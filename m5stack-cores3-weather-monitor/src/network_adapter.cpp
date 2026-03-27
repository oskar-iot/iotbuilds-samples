#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

#include "network_adapter.h"

WifiConnectionResult connectWifiHardware(const char* ssid, const char* password) {
  WifiConnectionResult result;
  result.connected = false;
  result.ssid = String(ssid);
  result.ip = "-";
  result.rssi = 0;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  const uint32_t start_ms = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start_ms < 15000) {
    delay(250);
  }

  if (WiFi.status() != WL_CONNECTED) {
    return result;
  }

  result.connected = true;
  result.ip = WiFi.localIP().toString();
  result.rssi = WiFi.RSSI();
  return result;
}

WifiConnectionResult readWifiMetricsHardware(const String& connected_ssid) {
  WifiConnectionResult result;
  result.connected = false;
  result.ssid = connected_ssid;
  result.ip = "-";
  result.rssi = 0;

  if (WiFi.status() != WL_CONNECTED) {
    return result;
  }

  result.connected = true;
  result.ip = WiFi.localIP().toString();
  result.rssi = WiFi.RSSI();
  return result;
}

HttpFetchResult fetchHttpGet(const char* url, const char* ca_cert) {
  HttpFetchResult result;

  WiFiClientSecure client;
  client.setCACert(ca_cert);

  HTTPClient http;
  if (!http.begin(client, url)) {
    result.error = "HTTP client init error";
    return result;
  }

  const int http_code = http.GET();
  if (http_code <= 0) {
    result.error = http.errorToString(http_code);
    http.end();
    return result;
  }

  result.success = true;
  result.http_status_code = http_code;
  result.payload = http.getString();
  http.end();
  return result;
}
