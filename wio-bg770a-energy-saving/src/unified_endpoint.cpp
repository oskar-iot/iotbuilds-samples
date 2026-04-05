#include <Arduino.h>
#include <WioCellular.h>

static constexpr const char* ENDPOINT_HOST = "echo.getpostman.com";
static constexpr int ENDPOINT_PORT = 80;
static constexpr const char* ENDPOINT_PATH = "/post";
static constexpr int CONNECT_TIMEOUT = 1000 * 10;   // [ms]
static constexpr int RECEIVE_TIMEOUT = 1000 * 10;   // [ms]
static constexpr size_t PAYLOAD_BUFFER_SIZE = 192;
static constexpr size_t REQUEST_BUFFER_SIZE = 512;

struct Telemetry {
  uint32_t profileId;
  uint32_t seq;
  uint32_t uptime;
  int rssi;
  int ber;
  int epsReg;
  int psState;
};

// 受信バイト列を可読な ASCII として出力（非表示文字は '.' に置換）。
static void PrintDataAsAscii(const void* data, size_t size) {
  const auto* p = static_cast<const uint8_t*>(data);
  for (size_t i = 0; i < size; ++i) {
    const uint8_t c = p[i];
    Serial.write((0x20 <= c && c <= 0x7E) ? c : '.');
  }
}

// 送信する端末状態を収集。
static Telemetry CollectTelemetry(uint32_t profileId, uint32_t seq) {
  Telemetry telemetry{};
  telemetry.profileId = profileId;
  telemetry.seq = seq;
  telemetry.uptime = static_cast<uint32_t>(millis() / 1000);
  telemetry.rssi = -1;
  telemetry.ber = -1;
  telemetry.epsReg = -1;
  telemetry.psState = -1;

  WioCellular.getSignalQuality(&telemetry.rssi, &telemetry.ber);
  WioCellular.getEpsNetworkRegistrationState(&telemetry.epsReg);
  WioCellular.getPacketDomainState(&telemetry.psState);
  return telemetry;
}

// Telemetry を JSON にシリアライズして、実データ長を返す。
static bool BuildPayload(const Telemetry& telemetry, char* payload, size_t payloadBufferSize, size_t* payloadSize) {
  const int written = snprintf(payload, payloadBufferSize,
                               "{\"type\":\"wio-bg770a\",\"profileId\":%lu,\"seq\":%lu,\"uptime\":%lu,"
                               "\"rssi\":%d,\"ber\":%d,\"epsReg\":%d,\"ps\":%d}",
                               static_cast<unsigned long>(telemetry.profileId),
                               static_cast<unsigned long>(telemetry.seq),
                               static_cast<unsigned long>(telemetry.uptime),
                               telemetry.rssi,
                               telemetry.ber,
                               telemetry.epsReg,
                               telemetry.psState);
  if (written < 0 || written >= static_cast<int>(payloadBufferSize)) {
    return false;
  }

  *payloadSize = static_cast<size_t>(written);  // 終端 '\0' は含まない実バイト数
  return true;
}

// 互換性のため関数名は維持しつつ、実際には汎用 HTTP endpoint へ POST する。
bool SendToUnifiedEndpoint(uint32_t profileId) {
  Serial.printf("HTTP endpoint: connect %s:%d\n", ENDPOINT_HOST, ENDPOINT_PORT);

  static uint32_t seq = 0;
  const Telemetry telemetry = CollectTelemetry(profileId, ++seq);

  char payload[PAYLOAD_BUFFER_SIZE];
  size_t payloadSize = 0;
  if (!BuildPayload(telemetry, payload, sizeof(payload), &payloadSize)) {
    Serial.println("ERROR: payload build failed");
    return false;
  }

  char request[REQUEST_BUFFER_SIZE];
  const int requestLength = snprintf(request,
                                     sizeof(request),
                                     "POST %s HTTP/1.1\r\n"
                                     "Host: %s\r\n"
                                     "User-Agent: wio-bg770a-energy-saving\r\n"
                                     "Content-Type: application/json\r\n"
                                     "Connection: close\r\n"
                                     "Content-Length: %u\r\n"
                                     "\r\n"
                                     "%s",
                                     ENDPOINT_PATH,
                                     ENDPOINT_HOST,
                                     static_cast<unsigned>(payloadSize),
                                     payload);
  if (requestLength < 0 || requestLength >= static_cast<int>(sizeof(request))) {
    Serial.println("ERROR: HTTP request buffer too small");
    return false;
  }

  WioCellularTcpClient2<WioCellularModule> client{WioCellular};
  if (!client.open(WioNetwork.config.pdpContextId, ENDPOINT_HOST, ENDPOINT_PORT)) {
    Serial.printf("ERROR: open failed %s\n", WioCellularResultToString(client.getLastResult()));
    return false;
  }
  if (!client.waitForConnect(CONNECT_TIMEOUT)) {
    Serial.printf("ERROR: connect failed %s\n", WioCellularResultToString(client.getLastResult()));
    return false;
  }

  Serial.print("HTTP endpoint TX: ");
  PrintDataAsAscii(request, static_cast<size_t>(requestLength));
  Serial.println();
  if (!client.send(request, static_cast<size_t>(requestLength))) {
    Serial.printf("ERROR: send failed %s\n", WioCellularResultToString(client.getLastResult()));
    return false;
  }

  static uint8_t recvData[WioCellular.RECEIVE_SOCKET_SIZE_MAX];
  size_t recvSize = 0;
  if (!client.receive(recvData, sizeof(recvData), &recvSize, RECEIVE_TIMEOUT)) {
    Serial.printf("ERROR: receive failed %s\n", WioCellularResultToString(client.getLastResult()));
    return false;
  }

  Serial.print("HTTP endpoint RX: ");
  PrintDataAsAscii(recvData, recvSize);
  Serial.println();
  return true;
}
