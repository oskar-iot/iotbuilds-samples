#include <Arduino.h>

#include "app_logic.h"
#include "m5_device.h"
#include "network_adapter.h"
#include "open_meteo_ca.h"
#include "secrets.h"
#include "ui_renderer.h"
#include "weather_logic.h"

namespace {

constexpr char kWeatherUrl[] =
    "https://api.open-meteo.com/v1/forecast?latitude=35.6895&longitude=139.6917&current="
    "temperature_2m,weather_code&timezone=Asia%2FTokyo";

AppState app_state = createInitialState();
uint32_t boot_count = 0;

// シリアルログへ起動直後の情報を出す。
void logBootInfo(const AppState& state) {
  Serial.println();
  Serial.println("=== M5Stack CoreS3 Check ===");
  Serial.printf("Boot count: %lu\n", static_cast<unsigned long>(state.device.boot_count));
  Serial.printf("Display size: %d x %d\n", displayWidth(), displayHeight());
}

// M5.Power と millis() はハード依存の値なので、ここで AppState へ反映する。
void refreshDeviceState(AppState& state) {
  updateUptime(state, millis());
  updateBatteryLevel(state, readBatteryLevel());
}

bool connectWifi(AppState& state) {
  setWifiConnecting(state, kWifiSsid);
  renderScreen(state);

  const WifiConnectionResult result = connectWifiHardware(kWifiSsid, kWifiPassword);
  if (!result.connected) {
    setWifiFailed(state);
    Serial.println("Wi-Fi connection failed");
    return false;
  }

  setWifiConnected(state, result.ssid, result.ip, result.rssi);
  Serial.println("Wi-Fi connected");
  Serial.println("SSID: " + state.network.ssid);
  Serial.println("IP: " + state.network.ip);
  Serial.println("RSSI: " + String(state.network.rssi) + " dBm");
  return true;
}

// 接続済みの間は RSSI や IP を定期更新する。
void refreshWifiMetrics(AppState& state) {
  if (state.network.wifi_state != WifiState::Connected) {
    return;
  }

  const WifiConnectionResult result = readWifiMetricsHardware(state.network.ssid);
  if (!result.connected) {
    setWifiFailed(state);
    return;
  }

  setWifiConnected(state, result.ssid, result.ip, result.rssi);
}

void fetchWebInfo(AppState& state) {
  if (state.network.wifi_state != WifiState::Connected) {
    setWebFetchSkipped(state);
    return;
  }

  const HttpFetchResult result = fetchHttpGet(kWeatherUrl, kOpenMeteoCaCert);
  if (!result.success) {
    setWebFetchFailed(state, result.error);
    return;
  }

  String updated_at;
  String summary;
  if (!parseWeatherPayload(result.payload, updated_at, summary)) {
    setWebFetchFailed(state, "JSON parse failed");
    return;
  }

  setWebFetchResult(state, result.http_status_code, updated_at, summary);
  Serial.println("Web: HTTP " + String(result.http_status_code));
  Serial.println("Payload: " + state.network.web_payload);
}

}  // namespace

void setup() {
  initializeDevice();

  Serial.begin(115200);
  ++boot_count;

  app_state = createInitialState();
  setBootCompleted(app_state, boot_count, readBatteryLevel());
  refreshDeviceState(app_state);
  renderScreen(app_state);
  logBootInfo(app_state);

  connectWifi(app_state);
  refreshWifiMetrics(app_state);
  fetchWebInfo(app_state);
  setIdleStatus(app_state);
  renderScreen(app_state);
}

void loop() {
  updateDevice();
  refreshDeviceState(app_state);

  const TouchEvent touch = readTouchEvent();
  if (touch.was_pressed) {
    recordTouch(app_state, touch.x, touch.y, true);
    if (app_state.network.wifi_state == WifiState::Connected) {
      refreshWifiMetrics(app_state);
    } else {
      connectWifi(app_state);
    }
    fetchWebInfo(app_state);
    renderScreen(app_state);
  }

  delay(20);
}
