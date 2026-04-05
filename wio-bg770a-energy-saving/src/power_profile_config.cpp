#include "power_profile_config.hpp"

#include <cstddef>

namespace {

struct PowerProfile {
  uint32_t id;
  const char* name;
  PowerProfileSleepMode sleepMode;
  int psmPeriodSec;
  int psmActiveSec;
};

////////////////////////////////////////////////////////////////////////////////
// EDIT HERE: 電力測定で変更する設定

static constexpr int SEND_INTERVAL_MS = 1000 * 60 * 5;       // [ms]
static constexpr int POWER_ON_TIMEOUT_MS = 1000 * 20;        // [ms]
static constexpr int NETWORK_TIMEOUT_MS = 1000 * 60 * 3;     // [ms]
static constexpr int PSM_POWER_DOWN_TIMEOUT_MS = 1000 * 60;  // [ms]

// 電力測定用プロファイル一覧:
// profile id は消費電力の大きい順:
// 1: Idle(モジュール起動維持)
// 2: Power Off
// 3: PSM
static constexpr PowerProfile POWER_PROFILES[] = {
    {3, "psm", PowerProfileSleepMode::Psm, 60 * 30, 10},
    {1, "idle", PowerProfileSleepMode::Idle, 0, 0},
    {2, "poweroff", PowerProfileSleepMode::PowerOff, 0, 0},
};

int activeProfileIndex = 0;

////////////////////////////////////////////////////////////////////////////////

const PowerProfile& ActiveProfile() {
  constexpr size_t profileCount = sizeof(POWER_PROFILES) / sizeof(POWER_PROFILES[0]);
  return POWER_PROFILES[activeProfileIndex >= 0 && activeProfileIndex < static_cast<int>(profileCount) ? activeProfileIndex : 0];
}

}  // namespace

int GetSendIntervalMs() {
  return SEND_INTERVAL_MS;
}

int GetPowerOnTimeoutMs() {
  return POWER_ON_TIMEOUT_MS;
}

int GetNetworkTimeoutMs() {
  return NETWORK_TIMEOUT_MS;
}

int GetPsmPowerDownTimeoutMs() {
  return PSM_POWER_DOWN_TIMEOUT_MS;
}

int GetPowerProfileCount() {
  return static_cast<int>(sizeof(POWER_PROFILES) / sizeof(POWER_PROFILES[0]));
}

bool SetActivePowerProfileIndex(int index) {
  if (index < 0 || index >= GetPowerProfileCount()) {
    return false;
  }
  activeProfileIndex = index;
  return true;
}

int GetActivePowerProfileIndex() {
  return activeProfileIndex;
}

bool SetActivePowerProfileId(uint32_t profileId) {
  for (int i = 0; i < GetPowerProfileCount(); ++i) {
    if (POWER_PROFILES[i].id == profileId) {
      activeProfileIndex = i;
      return true;
    }
  }
  return false;
}

uint32_t GetPowerProfileId() {
  return ActiveProfile().id;
}

const char* GetPowerProfileName() {
  return ActiveProfile().name;
}

PowerProfileSleepMode GetPowerProfileSleepMode() {
  return ActiveProfile().sleepMode;
}

bool IsPowerProfileSleepModePowerOff() {
  return ActiveProfile().sleepMode == PowerProfileSleepMode::PowerOff;
}

bool IsPowerProfileSleepModeIdle() {
  return ActiveProfile().sleepMode == PowerProfileSleepMode::Idle;
}

const char* GetPowerProfileSleepModeName() {
  switch (GetPowerProfileSleepMode()) {
    case PowerProfileSleepMode::Psm:
      return "PSM";
    case PowerProfileSleepMode::Idle:
      return "IDLE";
    case PowerProfileSleepMode::PowerOff:
      return "POWER_OFF";
  }
  return "UNKNOWN";
}

int GetPowerProfilePsmPeriodSec() {
  return ActiveProfile().psmPeriodSec;
}

int GetPowerProfilePsmActiveSec() {
  return ActiveProfile().psmActiveSec;
}
