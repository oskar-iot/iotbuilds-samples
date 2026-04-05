#include "wio_cellular_http_client.h"

#include <Arduino.h>
#include <WioCellular.h>

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

namespace {

constexpr auto kSearchAccessTechnology = WioCellularNetwork::SearchAccessTechnology::LTEM;
constexpr const char* kLtemBand = WioCellularNetwork::ALL_LTEM_BAND;

std::string BuildSoracomPdpAuthCommand(int pdpContextId, const char* user, const char* pass) {
  return "AT+CGAUTH=" + std::to_string(pdpContextId) + ",1,\"" + user + "\",\"" + pass + "\"";
}

}  // namespace

WioCellularHttpClient::WioCellularHttpClient(WioCellularConfig config) : config_(config) {}

void WioCellularHttpClient::begin() {
  WioCellular.begin();
  WioNetwork.config.searchAccessTechnology = kSearchAccessTechnology;
  WioNetwork.config.ltemBand = kLtemBand;
  WioNetwork.config.pdpContextId = config_.pdpContextId;
  WioNetwork.config.apn = config_.apn;
}

bool WioCellularHttpClient::connect() {
  Serial.println("Step: powerOn");
  const auto powerOnResult = WioCellular.powerOn(config_.powerOnTimeoutMs);
  if (powerOnResult != WioCellularResult::Ok) {
    Serial.printf("ERROR: powerOn failed: %s\n", WioCellularResultToString(powerOnResult));
    return false;
  }

  Serial.println("Step: WioNetwork.begin");
  WioNetwork.begin();

  Serial.println("Step: AT+CGAUTH");
  const auto cgauthResult = WioCellular.executeCommand(
      BuildSoracomPdpAuthCommand(config_.pdpContextId, config_.soracomUser, config_.soracomPass),
      config_.commandTimeoutMs);
  if (cgauthResult != WioCellularResult::Ok) {
    Serial.printf("ERROR: AT+CGAUTH failed: %s\n", WioCellularResultToString(cgauthResult));
    return false;
  }

  Serial.printf("Step: wait network (%dms)\n", config_.networkTimeoutMs);
  if (!waitForCommunicationAvailableWithProgress(config_.networkTimeoutMs)) {
    Serial.println("ERROR: network unavailable");
    return false;
  }

  Serial.println("Network connected");
  return true;
}

void WioCellularHttpClient::poll(int timeoutMs) {
  WioCellular.doWork(timeoutMs);
}

bool WioCellularHttpClient::postJson(const char* host, int port, const char* path, const char* payload) {
  WioCellularTcpClient2<WioCellularModule> client{WioCellular};
  if (!client.open(WioNetwork.config.pdpContextId, host, port)) {
    Serial.printf("ERROR: open failed %s\n", WioCellularResultToString(client.getLastResult()));
    return false;
  }
  if (!client.waitForConnect(config_.httpConnectTimeoutMs)) {
    Serial.printf("ERROR: connect failed %s\n", WioCellularResultToString(client.getLastResult()));
    return false;
  }

  auto request = std::make_unique<char[]>(config_.httpBufferSize);
  const int requestLength = std::snprintf(
      request.get(),
      config_.httpBufferSize,
      "POST %s HTTP/1.1\r\n"
      "Host: %s\r\n"
      "User-Agent: wio-bg770a/phase1\r\n"
      "Content-Type: application/json\r\n"
      "Connection: close\r\n"
      "Content-Length: %u\r\n"
      "\r\n"
      "%s",
      path,
      host,
      static_cast<unsigned>(std::strlen(payload)),
      payload);
  if (requestLength < 0 || requestLength >= static_cast<int>(config_.httpBufferSize)) {
    Serial.println("ERROR: HTTP request buffer too small");
    return false;
  }

  if (!client.send(request.get(), static_cast<size_t>(requestLength))) {
    Serial.printf("ERROR: send failed %s\n", WioCellularResultToString(client.getLastResult()));
    return false;
  }

  static uint8_t response[WioCellular.RECEIVE_SOCKET_SIZE_MAX];
  size_t responseSize = 0;
  if (!client.receive(response, sizeof(response) - 1, &responseSize, config_.httpReceiveTimeoutMs)) {
    Serial.printf("ERROR: receive failed %s\n", WioCellularResultToString(client.getLastResult()));
    return false;
  }

  response[responseSize] = '\0';
  Serial.printf("HTTP response:\n%s\n", reinterpret_cast<const char*>(response));
  return true;
}

bool WioCellularHttpClient::waitForCommunicationAvailableWithProgress(int timeoutMs) {
  const auto waitStart = millis();
  auto lastLog = waitStart;

  while (timeoutMs < 0 || millis() - waitStart < static_cast<uint32_t>(timeoutMs)) {
    WioCellular.doWork(100);
    if (WioNetwork.canCommunicate()) {
      return true;
    }

    const auto now = millis();
    if (now - lastLog >= 1000 * 10) {
      const auto state = WioNetwork.getNetworkState();
      Serial.printf("Waiting network... elapsed=%lu/%dms state=%s\n",
                    static_cast<unsigned long>(now - waitStart),
                    timeoutMs,
                    WioNetwork.networkStateToString(state));
      lastLog = now;
    }
  }

  return false;
}
