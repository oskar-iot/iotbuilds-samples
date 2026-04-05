#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include <csignal>
#include <WioCellular.h>

#include <cctype>
#include <string>
#include <vector>

#include "power_profile_config.hpp"

static constexpr auto SEARCH_ACCESS_TECHNOLOGY = WioCellularNetwork::SearchAccessTechnology::LTEM;
static constexpr const char* LTEM_BAND = WioCellularNetwork::ALL_LTEM_BAND;
static constexpr const char* APN = "soracom.io";
static constexpr int PDP_CONTEXT_ID = 1;
static constexpr const char* PDP_AUTH_USER = "sora";
static constexpr const char* PDP_AUTH_PASS = "sora";
static constexpr int PDP_AUTH_TYPE = 1;
static constexpr const char* MODE_STORE_PATH = "/power_profile.txt";
static constexpr uint32_t MODE_BUTTON_HOLD_MS = 1500;
static constexpr uint32_t MODE_BOOT_WINDOW_MS = 2000;
static constexpr uint32_t MODE_INDICATION_PAUSE_MS = 700;
static constexpr uint32_t MODE_INDICATION_PREAMBLE_ON_MS = 2000;
static constexpr uint32_t MODE_INDICATION_PREAMBLE_GAP_MS = 700;
static constexpr uint32_t MODE_INDICATION_BLINK_ON_MS = 1000;
static constexpr uint32_t MODE_INDICATION_BLINK_OFF_MS = 1000;

using namespace Adafruit_LittleFS_Namespace;

struct ServingCellInfo {
  std::string rat = "-";
  std::string plmn = "-";
  std::string band = "-";
  std::string channel = "-";
  bool available = false;
};

static std::string Trim(const std::string& value) {
  size_t begin = 0;
  while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin]))) {
    ++begin;
  }
  size_t end = value.size();
  while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
    --end;
  }
  return value.substr(begin, end - begin);
}

static std::string StripQuotes(const std::string& value) {
  const std::string trimmed = Trim(value);
  if (trimmed.size() >= 2 && trimmed.front() == '"' && trimmed.back() == '"') {
    return trimmed.substr(1, trimmed.size() - 2);
  }
  return trimmed;
}

static std::vector<std::string> SplitCsvRespectingQuotes(const std::string& text) {
  std::vector<std::string> fields;
  std::string current;
  bool inQuotes = false;

  for (char c : text) {
    if (c == '"') {
      inQuotes = !inQuotes;
      current.push_back(c);
      continue;
    }
    if (!inQuotes && c == ',') {
      fields.push_back(Trim(current));
      current.clear();
      continue;
    }
    current.push_back(c);
  }
  fields.push_back(Trim(current));
  return fields;
}

static std::string BuildPdpAuthCommand() {
  return "AT+CGAUTH=" + std::to_string(PDP_CONTEXT_ID) + "," + std::to_string(PDP_AUTH_TYPE) + ",\"" + PDP_AUTH_USER +
         "\",\"" + PDP_AUTH_PASS + "\"";
}

static void BlinkLedCount(int count, uint32_t onMs = 120, uint32_t offMs = 180) {
  for (int i = 0; i < count; ++i) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(onMs);
    digitalWrite(LED_BUILTIN, LOW);
    delay(offMs);
  }
}

static void IndicateActivePowerProfileWithLed() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(MODE_INDICATION_PREAMBLE_ON_MS);
  digitalWrite(LED_BUILTIN, LOW);
  delay(MODE_INDICATION_PREAMBLE_GAP_MS);
  BlinkLedCount(static_cast<int>(GetPowerProfileId()), MODE_INDICATION_BLINK_ON_MS, MODE_INDICATION_BLINK_OFF_MS);
  delay(MODE_INDICATION_PAUSE_MS);
}

static bool SaveActivePowerProfileIndex() {
  if (InternalFS.exists(MODE_STORE_PATH)) {
    InternalFS.remove(MODE_STORE_PATH);
  }

  File file(InternalFS);
  if (!file.open(MODE_STORE_PATH, FILE_O_WRITE)) {
    return false;
  }

  const char digit = static_cast<char>('0' + static_cast<int>(GetPowerProfileId()));
  const auto written = file.write(&digit, 1);
  file.close();
  return written == 1;
}

static void LoadActivePowerProfileIndex() {
  if (!InternalFS.begin()) {
    return;
  }

  File file(InternalFS);
  if (!file.open(MODE_STORE_PATH, FILE_O_READ)) {
    return;
  }

  char digit = '\0';
  const auto readLen = file.read(&digit, 1);
  file.close();
  if (readLen == 1 && digit >= '0' && digit <= '9') {
    SetActivePowerProfileId(static_cast<uint32_t>(digit - '0'));
  }
}

static void HandlePowerProfileSelectionAtBoot() {
  const auto bootStart = millis();
  if (digitalRead(PIN_BUTTON1) != LOW) {
    return;
  }

  while (digitalRead(PIN_BUTTON1) == LOW && millis() - bootStart < MODE_BUTTON_HOLD_MS) {
    delay(10);
  }
  if (digitalRead(PIN_BUTTON1) != LOW) {
    return;
  }

  const uint32_t currentId = GetPowerProfileId();
  const uint32_t nextId = (currentId % static_cast<uint32_t>(GetPowerProfileCount())) + 1;
  if (!SetActivePowerProfileId(nextId)) {
    return;
  }

  SaveActivePowerProfileIndex();

  while (digitalRead(PIN_BUTTON1) == LOW) {
    delay(10);
  }
  while (millis() - bootStart < MODE_BOOT_WINDOW_MS) {
    delay(10);
  }
}

// +QNWINFO から現在のRAT/PLMN/Band/Channelを取得。
static ServingCellInfo GetServingCellInfo() {
  ServingCellInfo info;

  const auto result = WioCellular.queryCommand(
      "AT+QNWINFO",
      [&info](const std::string& response) -> bool {
        if (response.compare(0, 10, "+QNWINFO: ") != 0) {
          return false;
        }

        const auto fields = SplitCsvRespectingQuotes(response.substr(10));
        if (fields.size() >= 1) {
          info.rat = StripQuotes(fields[0]);
        }
        if (fields.size() >= 2) {
          info.plmn = StripQuotes(fields[1]);
        }
        if (fields.size() >= 3) {
          info.band = StripQuotes(fields[2]);
        }
        if (fields.size() >= 4) {
          info.channel = StripQuotes(fields[3]);
        }
        info.available = true;
        return true;
      },
      300);

  if (result != WioCellularResult::Ok) {
    info.available = false;
  }
  return info;
}

// 通信可能になるまで待機し、一定間隔で進捗ログを出す。
static bool WaitForCommunicationAvailableWithProgress(int timeoutMs, uint32_t logIntervalMs) {
  const auto waitStart = millis();
  auto lastLog = waitStart;

  while (timeoutMs < 0 || millis() - waitStart < static_cast<uint32_t>(timeoutMs)) {
    WioCellular.doWork(100);
    if (WioNetwork.canCommunicate()) {
      return true;
    }

    const auto now = millis();
    if (now - lastLog >= logIntervalMs) {
      const auto state = WioNetwork.getNetworkState();
      Serial.printf("Waiting network... elapsed=%lu/%dms state=%s\n",
                    static_cast<unsigned long>(now - waitStart),
                    timeoutMs,
                    WioNetwork.networkStateToString(state));
      lastLog = now;
    }
  }

  return false;
}

void PrintInfo();
void PrintStatus();
bool SendToUnifiedEndpoint(uint32_t profileId);

// nRF52 + FreeRTOS では delay() が vTaskDelay() を使い、tickless idle で深い待機に入る。
static void SleepMcuDeepMs(uint32_t durationMs) {
  delay(durationMs);
}

// SIGABRT を捕捉したときの非常停止ハンドラ。
// LED 点滅ループへ入れて、異常終了を目視できるようにする。
static void abortHandler(int sig) {
  Serial.printf("ABORT: Signal %d received\n", sig);
  yield();

  vTaskSuspendAll();  // FreeRTOS
  while (true) {
    ledOn(LED_BUILTIN);
    nrfx_coredep_delay_us(100000);  // Spin
    ledOff(LED_BUILTIN);
    nrfx_coredep_delay_us(100000);  // Spin
  }
}

void setup() {
  // abort() 発生時に abortHandler へ制御を渡す。
  signal(SIGABRT, abortHandler);
  Serial.begin(115200);
  {
    // USB シリアル接続待ち（最大 5 秒）。未接続でも先へ進む。
    const auto start = millis();
    while (!Serial && millis() - start < 5000) {
      delay(2);
    }
  }
  Serial.println();
  Serial.println();

  Serial.println("Startup");

  LoadActivePowerProfileIndex();
  HandlePowerProfileSelectionAtBoot();
  IndicateActivePowerProfileWithLed();

  // セルラーモジュール初期化。
  WioCellular.begin();

  WioNetwork.config.searchAccessTechnology = SEARCH_ACCESS_TECHNOLOGY;
  WioNetwork.config.ltemBand = LTEM_BAND;
  WioNetwork.config.pdpContextId = PDP_CONTEXT_ID;
  WioNetwork.config.apn = APN;

  Serial.printf("PDP context: APN=%s authType=%d\n", APN, PDP_AUTH_TYPE);
}

void loop() {
  static bool infoPrinted = false;
  static bool modemPrepared = false;
  const uint32_t profileId = GetPowerProfileId();
  const bool keepModemOn = IsPowerProfileSleepModeIdle();

  Serial.printf("Power profile: id=%lu name=%s mode=%s\n",
                static_cast<unsigned long>(profileId),
                GetPowerProfileName(),
                GetPowerProfileSleepModeName());

  if (!keepModemOn || !modemPrepared) {
    // 送信タイミングでのみモデムを起動。Idle は初回だけ初期化して以降は起動維持する。
    Serial.println("Step: powerOn");
    const auto powerOnResult = WioCellular.powerOn(GetPowerOnTimeoutMs());
    if (powerOnResult != WioCellularResult::Ok) {
      Serial.printf("ERROR: powerOn failed: %s\n", WioCellularResultToString(powerOnResult));
      abort();
    }
    Serial.println("Step: WioNetwork.begin");
    WioNetwork.begin();

    // PDP 認証情報を設定。
    Serial.println("Step: AT+CGAUTH");
    const auto cgauthResult = WioCellular.executeCommand(BuildPdpAuthCommand(), 300);
    if (cgauthResult != WioCellularResult::Ok) {
      Serial.printf("ERROR: AT+CGAUTH failed: %s\n", WioCellularResultToString(cgauthResult));
      abort();
    }

    Serial.println("Step: disable PSM");
    const auto psmUrcResult = WioCellular.setPsmEnteringIndicationUrc(true);
    if (psmUrcResult != WioCellularResult::Ok) {
      Serial.printf("ERROR: setPsmEnteringIndicationUrc failed: %s\n", WioCellularResultToString(psmUrcResult));
      abort();
    }

    const auto psmDisableResult = WioCellular.setPsm(0, GetPowerProfilePsmPeriodSec(), GetPowerProfilePsmActiveSec());
    if (psmDisableResult != WioCellularResult::Ok) {
      Serial.printf("ERROR: setPsm(disable) failed: %s\n", WioCellularResultToString(psmDisableResult));
      abort();
    }

    modemPrepared = true;
  }

  const int networkTimeoutMs = GetNetworkTimeoutMs();
  Serial.printf("Step: wait network (%dms)\n", networkTimeoutMs);
  if (WaitForCommunicationAvailableWithProgress(networkTimeoutMs, 1000 * 10)) {
    const ServingCellInfo servingCell = GetServingCellInfo();
    if (servingCell.available) {
      Serial.printf("Serving cell: rat=%s plmn=%s band=%s ch=%s\n",
                    servingCell.rat.c_str(),
                    servingCell.plmn.c_str(),
                    servingCell.band.c_str(),
                    servingCell.channel.c_str());
    } else {
      Serial.println("Serving cell: unavailable");
    }

    if (!infoPrinted) {
      PrintInfo();
      Serial.println();
      infoPrinted = true;
    }

    PrintStatus();
    if (!SendToUnifiedEndpoint(profileId)) {
      Serial.println("HTTP endpoint send failed");
    }
  } else {
    Serial.println("Network unavailable");
  }

  if (IsPowerProfileSleepModePowerOff()) {
    WioNetwork.end();
    if (WioCellular.powerOff() != WioCellularResult::Ok) abort();
    modemPrepared = false;
  } else if (IsPowerProfileSleepModeIdle()) {
    // Idle 比較ではモデムを在圏したまま維持し、次周期もそのまま利用する。
  } else {
    // 通信完了後に PSM へ移行を試みる。
    bool enteredPsm = false;
    if (WioNetwork.canCommunicate()) {
      if (WioCellular.setPsm(1, GetPowerProfilePsmPeriodSec(), GetPowerProfilePsmActiveSec()) != WioCellularResult::Ok) {
        abort();
      }

      const auto psmStart = millis();
      while (millis() - psmStart < static_cast<uint32_t>(GetPsmPowerDownTimeoutMs())) {
        WioCellular.doWork(10);  // Spin
        if (!WioCellular.getInterface().isActive()) {
          enteredPsm = true;
          break;
        }
      }
    }

    if (enteredPsm) {
      WioNetwork.end(false);
    } else {
      Serial.println("PSM entry timeout");
      WioNetwork.end(false);
    }
    modemPrepared = false;
  }
  SleepMcuDeepMs(static_cast<uint32_t>(GetSendIntervalMs()));
}
