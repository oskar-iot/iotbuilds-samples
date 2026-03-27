#pragma once

#include <Arduino.h>

#include "app_state.h"

AppState createInitialState();
void updateUptime(AppState& state, uint32_t uptime_ms);
void updateBatteryLevel(AppState& state, int battery_level);
void setBootCompleted(AppState& state, uint32_t boot_count, int battery_level);
void setIdleStatus(AppState& state);
void recordTouch(AppState& state, int x, int y, bool new_touch);
void setWifiSecretsMissing(AppState& state);
void setWifiConnecting(AppState& state, const String& ssid);
void setWifiConnected(AppState& state, const String& ssid, const String& ip, int32_t rssi);
void setWifiFailed(AppState& state);
void setWebFetchSkipped(AppState& state);
void setWebFetchResult(AppState& state, int http_status_code, const String& updated_at,
                       const String& payload);
void setWebFetchFailed(AppState& state, const String& reason);
String summarizePayload(const String& payload, size_t max_length);
