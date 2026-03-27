#pragma once

#include <Arduino.h>

struct WifiConnectionResult {
  bool connected = false;
  String ssid = "-";
  String ip = "-";
  int32_t rssi = 0;
};

struct HttpFetchResult {
  bool success = false;
  int http_status_code = 0;
  String payload = "";
  String error = "";
};

WifiConnectionResult connectWifiHardware(const char* ssid, const char* password);
WifiConnectionResult readWifiMetricsHardware(const String& connected_ssid);
HttpFetchResult fetchHttpGet(const char* url, const char* ca_cert);
