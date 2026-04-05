#pragma once

#include <cstdint>

enum class PowerProfileSleepMode {
  Psm,
  Idle,
  PowerOff,
};

int GetSendIntervalMs();
int GetPowerOnTimeoutMs();
int GetNetworkTimeoutMs();
int GetPsmPowerDownTimeoutMs();

int GetPowerProfileCount();
bool SetActivePowerProfileIndex(int index);
int GetActivePowerProfileIndex();
bool SetActivePowerProfileId(uint32_t profileId);
uint32_t GetPowerProfileId();
const char* GetPowerProfileName();
PowerProfileSleepMode GetPowerProfileSleepMode();
bool IsPowerProfileSleepModePowerOff();
bool IsPowerProfileSleepModeIdle();
const char* GetPowerProfileSleepModeName();
int GetPowerProfilePsmPeriodSec();
int GetPowerProfilePsmActiveSec();
