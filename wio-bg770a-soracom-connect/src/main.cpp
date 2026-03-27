#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <csignal>
#include <WioCellular.h>

#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <limits>
#include <string>
#include <vector>

static constexpr auto SEARCH_ACCESS_TECHNOLOGY = WioCellularNetwork::SearchAccessTechnology::LTEM;
static constexpr const char* LTEM_BAND = WioCellularNetwork::ALL_LTEM_BAND;
static constexpr const char* APN = "soracom.io";
static constexpr int PDP_CONTEXT_ID = 1;
static constexpr const char* SORACOM_USER = "sora";
static constexpr const char* SORACOM_PASS = "sora";
static constexpr int POWER_ON_TIMEOUT_MS = 1000 * 20;
static constexpr int NETWORK_TIMEOUT_MS = 1000 * 60 * 3;
static constexpr int CGAUTH_TIMEOUT_MS = 300;
static constexpr int COPS_TIMEOUT_MS = 300;
static constexpr int QCSQ_TIMEOUT_MS = 300;
static constexpr uint32_t STATUS_LOG_INTERVAL_MS = 1000 * 10;
static constexpr int UNKNOWN_METRIC = std::numeric_limits<int>::min();

struct ServingCellInfo {
  std::string rat = "-";
  std::string band = "-";
  std::string channel = "-";
  bool available = false;
};

struct CopsInfo {
  std::string oper = "-";
  bool available = false;
};

struct QcsqInfo {
  std::string rat = "-";
  /*
   * LTE-M 無線品質指標の違い
   * - RSSI[dBm]: 受信電力総量。信号 + 干渉 + 雑音を含む
   * - RSRP[dBm]: 参照信号の受信強度。カバレッジ把握の主指標
   * - RSRQ[dB]: 参照信号の品質。混雑/干渉の影響を見やすい
   * - SINR[dB]: 信号対干渉雑音比。復調しやすさ/実効品質の主指標
   *
   * 目安としては RSRP/SINR を主に見て、RSRQ で劣化要因を補助判定する。
   * RSSI 単独では品質判断しない。
   */
  int rssi = UNKNOWN_METRIC;     // dBm
  int rsrp = UNKNOWN_METRIC;     // dBm
  int sinrRaw = UNKNOWN_METRIC;  // 0..250 (0.2dB step, -20dB offset)
  int rsrq = UNKNOWN_METRIC;     // dB
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

static int ParseIntOrDefault(const std::string& value, int defaultValue = -1) {
  const std::string trimmed = Trim(value);
  if (trimmed.empty()) {
    return defaultValue;
  }

  char* end = nullptr;
  const long parsed = std::strtol(trimmed.c_str(), &end, 10);
  if (end == trimmed.c_str() || *end != '\0') {
    return defaultValue;
  }
  return static_cast<int>(parsed);
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

// +QNWINFO から現在のRAT/Band/Channelを取得。
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

// +COPS? から operator を取得。AT+COPS=3,2 適用時は MCC+MNC (例: 44020)。
static CopsInfo GetCopsInfo() {
  CopsInfo info;

  const auto result = WioCellular.queryCommand(
      "AT+COPS?",
      [&info](const std::string& response) -> bool {
        if (response.compare(0, 7, "+COPS: ") != 0) {
          return false;
        }

        const auto fields = SplitCsvRespectingQuotes(response.substr(7));
        if (fields.size() >= 3) {
          info.oper = StripQuotes(fields[2]);
        }
        info.available = true;
        return true;
      },
      COPS_TIMEOUT_MS);

  if (result != WioCellularResult::Ok) {
    info.available = false;
  }
  return info;
}

// +QCSQ から無線品質を取得。
// BG770Aの eMTC 応答は "eMTC",<rssi>,<rsrp>,<sinr>,<rsrq>。
static QcsqInfo GetQcsqInfo() {
  QcsqInfo info;

  const auto result = WioCellular.queryCommand(
      "AT+QCSQ",
      [&info](const std::string& response) -> bool {
        if (response.compare(0, 7, "+QCSQ: ") != 0) {
          return false;
        }

        const auto fields = SplitCsvRespectingQuotes(response.substr(7));
        if (fields.size() >= 1) {
          info.rat = StripQuotes(fields[0]);
        }
        if (fields.size() >= 2) {
          info.rssi = ParseIntOrDefault(fields[1], UNKNOWN_METRIC);
        }
        if (fields.size() >= 3) {
          info.rsrp = ParseIntOrDefault(fields[2], UNKNOWN_METRIC);
        }
        if (fields.size() >= 4) {
          info.sinrRaw = ParseIntOrDefault(fields[3], UNKNOWN_METRIC);
        }
        if (fields.size() >= 5) {
          info.rsrq = ParseIntOrDefault(fields[4], UNKNOWN_METRIC);
        }
        info.available = true;
        return true;
      },
      QCSQ_TIMEOUT_MS);

  if (result != WioCellularResult::Ok) {
    info.available = false;
  }
  return info;
}

static void FormatIntOrDash(int value, char* buffer, size_t bufferSize) {
  if (bufferSize == 0) {
    return;
  }
  if (value == UNKNOWN_METRIC) {
    std::snprintf(buffer, bufferSize, "-");
    return;
  }
  std::snprintf(buffer, bufferSize, "%d", value);
}

static void FormatSinrDbOrDash(int sinrRaw, char* buffer, size_t bufferSize) {
  if (bufferSize == 0) {
    return;
  }
  if (sinrRaw == UNKNOWN_METRIC) {
    std::snprintf(buffer, bufferSize, "-");
    return;
  }

  // Quectel QCSQ SINR raw(0..250) -> dB: raw/5 - 20
  const int sinrDbX10 = sinrRaw * 2 - 200;
  const bool negative = sinrDbX10 < 0;
  const int absValue = negative ? -sinrDbX10 : sinrDbX10;
  std::snprintf(buffer, bufferSize, "%s%d.%d", negative ? "-" : "", absValue / 10, absValue % 10);
}

static std::string BuildSoracomPdpAuthCommand() {
  return "AT+CGAUTH=" + std::to_string(PDP_CONTEXT_ID) + ",1,\"" + SORACOM_USER + "\",\"" + SORACOM_PASS + "\"";
}

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

static void PrintRadioStatus() {
  // Radioログの読み方は README.md の「ログ指標リファレンス」を参照。
  int epsReg = -1;
  WioCellular.getEpsNetworkRegistrationState(&epsReg);

  int psState = -1;
  WioCellular.getPacketDomainState(&psState);

  const ServingCellInfo servingCell = GetServingCellInfo();
  const CopsInfo cops = GetCopsInfo();
  const QcsqInfo qcsq = GetQcsqInfo();
  const auto networkState = WioNetwork.getNetworkState();
  const char* operName = (cops.available && !cops.oper.empty()) ? cops.oper.c_str() : "-";
  const char* rat = servingCell.available ? servingCell.rat.c_str() : (qcsq.available ? qcsq.rat.c_str() : "-");
  const char* band = servingCell.available ? servingCell.band.c_str() : "-";
  const char* channel = servingCell.available ? servingCell.channel.c_str() : "-";
  char rssiDbm[16];
  char rsrpDbm[16];
  char rsrqDb[16];
  char sinrDb[16];
  const int rssi = qcsq.available ? qcsq.rssi : UNKNOWN_METRIC;
  const int rsrp = qcsq.available ? qcsq.rsrp : UNKNOWN_METRIC;
  const int rsrq = qcsq.available ? qcsq.rsrq : UNKNOWN_METRIC;
  const int sinrRaw = qcsq.available ? qcsq.sinrRaw : UNKNOWN_METRIC;
  FormatIntOrDash(rssi, rssiDbm, sizeof(rssiDbm));
  FormatIntOrDash(rsrp, rsrpDbm, sizeof(rsrpDbm));
  FormatIntOrDash(rsrq, rsrqDb, sizeof(rsrqDb));
  FormatSinrDbOrDash(sinrRaw, sinrDb, sizeof(sinrDb));

  Serial.printf("Radio t=%lus net=%s epsReg=%d ps=%d oper=%s rat=%s band=%s ch=%s rssi=%s rsrp=%s rsrq=%s sinrDb=%s\n",
                static_cast<unsigned long>(millis() / 1000),
                WioNetwork.networkStateToString(networkState),
                epsReg,
                psState,
                operName,
                rat,
                band,
                channel,
                rssiDbm,
                rsrpDbm,
                rsrqDb,
                sinrDb);
}

static void abortHandler(int sig) {
  Serial.printf("ABORT: Signal %d received\n", sig);
  yield();

  vTaskSuspendAll();
  while (true) {
    ledOn(LED_BUILTIN);
    nrfx_coredep_delay_us(100000);
    ledOff(LED_BUILTIN);
    nrfx_coredep_delay_us(100000);
  }
}

void setup() {
  signal(SIGABRT, abortHandler);
  Serial.begin(115200);
  {
    const auto start = millis();
    while (!Serial && millis() - start < 5000) {
      delay(2);
    }
  }
  Serial.println();
  Serial.println("Startup SORACOM attach check");

  WioCellular.begin();
  WioNetwork.config.searchAccessTechnology = SEARCH_ACCESS_TECHNOLOGY;
  WioNetwork.config.ltemBand = LTEM_BAND;
  WioNetwork.config.pdpContextId = PDP_CONTEXT_ID;
  WioNetwork.config.apn = APN;

  Serial.printf("Carrier profile: soracom (APN=%s authType=1)\n", APN);
}

void loop() {
  static bool initialized = false;
  static bool attached = false;
  static uint32_t lastStatusLogMs = 0;

  if (!initialized) {
    initialized = true;
    digitalWrite(LED_BUILTIN, HIGH);

    Serial.println("Step: powerOn");
    const auto powerOnResult = WioCellular.powerOn(POWER_ON_TIMEOUT_MS);
    if (powerOnResult != WioCellularResult::Ok) {
      Serial.printf("ERROR: powerOn failed: %s\n", WioCellularResultToString(powerOnResult));
      abort();
    }

    Serial.println("Step: WioNetwork.begin");
    WioNetwork.begin();

    Serial.println("Step: AT+CGAUTH");
    const auto cgauthResult = WioCellular.executeCommand(BuildSoracomPdpAuthCommand(), CGAUTH_TIMEOUT_MS);
    if (cgauthResult != WioCellularResult::Ok) {
      Serial.printf("ERROR: AT+CGAUTH failed: %s\n", WioCellularResultToString(cgauthResult));
      abort();
    }

    Serial.println("Step: AT+COPS=3,2");
    const auto copsResult = WioCellular.executeCommand("AT+COPS=3,2", COPS_TIMEOUT_MS);
    if (copsResult != WioCellularResult::Ok) {
      Serial.printf("WARN: AT+COPS=3,2 failed: %s (continue)\n", WioCellularResultToString(copsResult));
    }

    Serial.printf("Step: wait network (%dms)\n", NETWORK_TIMEOUT_MS);
    attached = WaitForCommunicationAvailableWithProgress(NETWORK_TIMEOUT_MS, 1000 * 10);
    if (attached) {
      Serial.printf("Attach OK: periodic status every %lus\n",
                    static_cast<unsigned long>(STATUS_LOG_INTERVAL_MS / 1000));
      PrintRadioStatus();
      lastStatusLogMs = millis();
    } else {
      Serial.println("Network unavailable");
    }

    digitalWrite(LED_BUILTIN, LOW);
  }

  WioCellular.doWork(100);

  if (!attached) {
    delay(1000);
    return;
  }

  const auto now = millis();
  if (now - lastStatusLogMs >= STATUS_LOG_INTERVAL_MS) {
    PrintRadioStatus();
    lastStatusLogMs = now;
  }

  delay(100);
}
