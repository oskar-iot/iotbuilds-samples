#include "funnel_publisher_app.h"

#include <Arduino.h>
#include <ArduinoJson.h>

#include <memory>

FunnelPublisherApp::FunnelPublisherApp(WioCellularHttpClient& httpClient, FunnelPublisherConfig config)
    : httpClient_(httpClient), config_(config), initialized_(false), lastSendMs_(0) {}

bool FunnelPublisherApp::initialize() {
  if (initialized_) {
    return true;
  }

  if (!httpClient_.connect()) {
    return false;
  }

  initialized_ = true;
  return true;
}

void FunnelPublisherApp::loop() {
  httpClient_.poll(100);

  if (!initialized_) {
    delay(1000);
    return;
  }

  const auto now = millis();
  if (lastSendMs_ != 0 && now - lastSendMs_ < config_.sendIntervalMs) {
    delay(100);
    return;
  }
  lastSendMs_ = now;

  auto payload = std::make_unique<char[]>(config_.jsonBufferSize);
  if (!buildPayload(payload.get(), config_.jsonBufferSize)) {
    Serial.println("ERROR: payload build failed");
    delay(1000);
    return;
  }

  Serial.printf("POST %s%s\n", config_.host, config_.path);
  Serial.printf("Payload: %s\n", payload.get());
  if (!httpClient_.postJson(config_.host, config_.port, config_.path, payload.get())) {
    Serial.println("ERROR: HTTP POST failed");
  }
}

bool FunnelPublisherApp::buildPayload(char* payload, size_t payloadSize) const {
  JsonDocument doc;
  doc["temperature"] = 15.0;
  doc["humidity"] = 50.0;

  const size_t written = serializeJson(doc, payload, payloadSize);
  if (written == 0 || written >= payloadSize) {
    return false;
  }
  return true;
}
