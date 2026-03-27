#include "app_logic.h"

namespace {

// 画面幅に収まるように長いレスポンス文字列を丸める。
String truncateText(const String& text, size_t max_length) {
  if (text.length() <= max_length) {
    return text;
  }
  return text.substring(0, max_length) + "...";
}

}  // namespace

// 起動時の既定状態を返す。
AppState createInitialState() {
  return AppState{};
}

// 表示用 uptime は秒単位で保持する。
void updateUptime(AppState& state, uint32_t uptime_ms) {
  state.device.uptime_seconds = uptime_ms / 1000;
}

// バッテリー残量を状態へ反映する。
void updateBatteryLevel(AppState& state, int battery_level) {
  state.device.battery_level = battery_level;
}

// 起動完了時の表示状態を作る。
void setBootCompleted(AppState& state, uint32_t boot_count, int battery_level) {
  state.device.boot_count = boot_count;
  state.device.battery_level = battery_level;
  state.status_message = "Boot OK";
  state.status_tone = StatusTone::Success;
}

// ユーザー入力待ちの通常状態へ戻す。
void setIdleStatus(AppState& state) {
  state.status_message = "Waiting for touch";
  state.status_tone = StatusTone::Success;
}

// タッチ結果を状態へ反映し、ステータスメッセージも更新する。
void recordTouch(AppState& state, int x, int y, bool new_touch) {
  state.device.touch.valid = true;
  state.device.touch.x = x;
  state.device.touch.y = y;
  if (new_touch) {
    ++state.device.touch.count;
  }
  state.status_message = "Touch x=" + String(x) + " y=" + String(y);
  state.status_tone = StatusTone::Warning;
}

// Wi-Fi 認証情報が未設定の状態を表現する。
void setWifiSecretsMissing(AppState& state) {
  state.network.wifi_state = WifiState::NotConfigured;
  state.network.ssid = "-";
  state.network.ip = "-";
  state.network.rssi = 0;
}

// Wi-Fi 接続中の状態を表現する。
void setWifiConnecting(AppState& state, const String& ssid) {
  state.network.wifi_state = WifiState::Connecting;
  state.network.ssid = ssid;
  state.network.ip = "-";
  state.network.rssi = 0;
}

// Wi-Fi 接続成功時のネットワーク状態を反映する。
void setWifiConnected(AppState& state, const String& ssid, const String& ip, int32_t rssi) {
  state.network.wifi_state = WifiState::Connected;
  state.network.ssid = ssid;
  state.network.ip = ip;
  state.network.rssi = rssi;
}

// Wi-Fi 接続失敗、または切断を状態へ反映する。
void setWifiFailed(AppState& state) {
  state.network.wifi_state = WifiState::Failed;
  state.network.ip = "-";
  state.network.rssi = 0;
}

// Web取得を実行できなかった状態を反映する。
void setWebFetchSkipped(AppState& state) {
  state.network.web_fetch_state = WebFetchState::Skipped;
  state.network.http_status_code = 0;
  state.network.last_updated_at = "-";
  state.network.web_payload = "No Wi-Fi connection";
}

// Web取得成功結果を画面表示向けに丸めて保存する。
void setWebFetchResult(AppState& state, int http_status_code, const String& updated_at,
                       const String& payload) {
  state.network.web_fetch_state = WebFetchState::Success;
  state.network.http_status_code = http_status_code;
  ++state.network.refresh_count;
  state.network.last_updated_at = updated_at;
  state.network.web_payload = summarizePayload(payload, 44);
}

// Web取得失敗理由を画面表示向けに保存する。
void setWebFetchFailed(AppState& state, const String& reason) {
  state.network.web_fetch_state = WebFetchState::Failed;
  state.network.http_status_code = 0;
  ++state.network.refresh_count;
  state.network.last_updated_at = "-";
  state.network.web_payload = summarizePayload(reason, 44);
}

// 改行を除去しつつ、画面に収まる長さへ整形する。
String summarizePayload(const String& payload, size_t max_length) {
  String normalized = payload;
  normalized.replace("\n", " ");
  normalized.replace("\r", " ");
  return truncateText(normalized, max_length);
}
