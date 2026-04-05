#pragma once

#include <cstddef>

struct WioCellularConfig {
  const char* apn;
  int pdpContextId;
  const char* soracomUser;
  const char* soracomPass;
  int powerOnTimeoutMs;
  int networkTimeoutMs;
  int commandTimeoutMs;
  int httpConnectTimeoutMs;
  int httpReceiveTimeoutMs;
  size_t httpBufferSize;
};

class WioCellularHttpClient {
 public:
  explicit WioCellularHttpClient(WioCellularConfig config);

  void begin();
  bool connect();
  void poll(int timeoutMs);
  bool postJson(const char* host, int port, const char* path, const char* payload);

 private:
  bool waitForCommunicationAvailableWithProgress(int timeoutMs);

  WioCellularConfig config_;
};
