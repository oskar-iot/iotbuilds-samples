#pragma once

#include <Arduino.h>

enum class StatusTone {
  Info,
  Success,
  Warning,
  Error,
};

enum class WifiState {
  NotConfigured,
  Connecting,
  Connected,
  Failed,
};

enum class WebFetchState {
  Idle,
  Skipped,
  Success,
  Failed,
};

struct TouchState {
  bool valid = false;
  int x = -1;
  int y = -1;
  uint32_t count = 0;
};

struct DeviceState {
  uint32_t boot_count = 0;
  uint32_t uptime_seconds = 0;
  int battery_level = -1;
  TouchState touch;
};

struct NetworkState {
  WifiState wifi_state = WifiState::NotConfigured;
  String ssid = "-";
  String ip = "-";
  int32_t rssi = 0;
  WebFetchState web_fetch_state = WebFetchState::Idle;
  int http_status_code = 0;
  uint32_t refresh_count = 0;
  String last_updated_at = "-";
  String web_payload = "-";
};

struct AppState {
  String status_message = "Booting";
  StatusTone status_tone = StatusTone::Info;
  DeviceState device;
  NetworkState network;
};
