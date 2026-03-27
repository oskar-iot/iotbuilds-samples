#include <M5Unified.h>

#include "m5_device.h"
#include "ui_renderer.h"

namespace {

constexpr uint32_t kBackgroundColor = 0x102030;
constexpr uint32_t kPanelColor = 0x1A1A1A;
constexpr uint32_t kAccentColor = 0x00D4A3;
constexpr uint32_t kWarningColor = 0xFFB000;
constexpr uint32_t kErrorColor = 0xFF5A5A;
constexpr int kPanelX = 12;
constexpr int kPanelWidth = 296;
constexpr int kTextX = 20;
constexpr int kTitleY = 8;
constexpr int kHeaderLineY = 28;
constexpr int kLineGap = 16;

// AppState の状態種別を、M5.Display で使う色へ変換する。
uint32_t statusToneColor(StatusTone tone) {
  switch (tone) {
    case StatusTone::Success:
      return kAccentColor;
    case StatusTone::Warning:
      return kWarningColor;
    case StatusTone::Error:
      return kErrorColor;
    case StatusTone::Info:
    default:
      return WHITE;
  }
}

// Wi-Fi 接続状態を画面表示用の短い文字列へ変換する。
String wifiStateText(WifiState state) {
  switch (state) {
    case WifiState::Connecting:
      return "connecting";
    case WifiState::Connected:
      return "connected";
    case WifiState::Failed:
      return "failed";
    case WifiState::NotConfigured:
    default:
      return "not configured";
  }
}

// M5Unified の描画APIで共通パネルを描く。
void drawPanel(int x, int y, int w, int h, const String& title) {
  M5.Display.fillRoundRect(x, y, w, h, 8, kPanelColor);
  M5.Display.setTextDatum(top_left);
  M5.Display.setFont(&fonts::Font2);
  M5.Display.setTextColor(TFT_CYAN, kPanelColor);
  M5.Display.drawString(title, x + 8, y + 6);
}

String ellipsizeText(const String& text, size_t max_length) {
  if (text.length() <= max_length) {
    return text;
  }
  return text.substring(0, max_length - 3) + "...";
}

// M5Unified の drawString を薄いラッパーにして座標指定を揃える。
void drawLine(int x, int y, const String& text, uint32_t color = WHITE) {
  M5.Display.setTextDatum(top_left);
  M5.Display.setFont(&fonts::Font2);
  M5.Display.setTextColor(color, kPanelColor);
  M5.Display.drawString(text, x, y);
}

}  // namespace

// AppState を唯一の入力として、画面全体を再描画する。
void renderScreen(const AppState& state) {
  M5.Display.fillScreen(kBackgroundColor);
  M5.Display.setTextDatum(top_left);
  M5.Display.setFont(&fonts::Font2);
  M5.Display.setTextColor(WHITE, kBackgroundColor);
  M5.Display.drawString("M5Stack CoreS3 Check", kPanelX, kTitleY);
  M5.Display.drawFastHLine(kPanelX, kHeaderLineY, displayWidth() - 24, kAccentColor);

  drawPanel(kPanelX, 36, kPanelWidth, 44, "Touch");
  String touch_text = "Last touch: -";
  if (state.device.touch.valid) {
    touch_text =
        "Last touch: x=" + String(state.device.touch.x) + " y=" + String(state.device.touch.y);
  }
  drawLine(kTextX, 56, ellipsizeText(touch_text, 30), statusToneColor(state.status_tone));

  drawPanel(kPanelX, 88, kPanelWidth, 144, "Network");
  const int line1_y = 108;
  drawLine(kTextX, line1_y, "Wi-Fi: " + wifiStateText(state.network.wifi_state));
  drawLine(kTextX, line1_y + kLineGap, "SSID: " + ellipsizeText(state.network.ssid, 22));
  drawLine(kTextX, line1_y + kLineGap * 2, "IP: " + ellipsizeText(state.network.ip, 24));
  String signal_text = "RSSI: -";
  if (state.network.wifi_state == WifiState::Connected) {
    signal_text = "RSSI: " + String(state.network.rssi) + " dBm";
  }
  drawLine(kTextX, line1_y + kLineGap * 3,
           "Refresh: #" + String(state.network.refresh_count));
  drawLine(kTextX, line1_y + kLineGap * 4, signal_text);
  drawLine(kTextX, line1_y + kLineGap * 5,
           "Updated: " + ellipsizeText(state.network.last_updated_at, 22));
  drawLine(kTextX, line1_y + kLineGap * 6, ellipsizeText(state.network.web_payload, 28),
           LIGHTGREY);
}
