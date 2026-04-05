#pragma once

#include <cstddef>
#include <cstdint>

#include "wio_cellular_http_client.h"

struct FunnelPublisherConfig {
  const char* host;
  int port;
  const char* path;
  uint32_t sendIntervalMs;
  size_t jsonBufferSize;
};

class FunnelPublisherApp {
 public:
  FunnelPublisherApp(WioCellularHttpClient& httpClient, FunnelPublisherConfig config);

  bool initialize();
  void loop();

 private:
  bool buildPayload(char* payload, size_t payloadSize) const;

  WioCellularHttpClient& httpClient_;
  FunnelPublisherConfig config_;
  bool initialized_;
  uint32_t lastSendMs_;
};
