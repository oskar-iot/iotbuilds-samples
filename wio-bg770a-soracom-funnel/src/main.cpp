#include <Arduino.h>
#include <Adafruit_TinyUSB.h>

#include <csignal>

#include "funnel_publisher_app.h"
#include "wio_cellular_http_client.h"

static constexpr const char* APN = "soracom.io";
static constexpr int PDP_CONTEXT_ID = 1;
static constexpr const char* SORACOM_USER = "sora";
static constexpr const char* SORACOM_PASS = "sora";
static constexpr const char* FUNNEL_HOST = "funnel.soracom.io";
static constexpr int FUNNEL_PORT = 80;
static constexpr const char* FUNNEL_PATH = "/";
static constexpr uint32_t SEND_INTERVAL_MS = 1000 * 10;
static constexpr int POWER_ON_TIMEOUT_MS = 1000 * 20;
static constexpr int NETWORK_TIMEOUT_MS = 1000 * 60 * 3;
static constexpr int COMMAND_TIMEOUT_MS = 300;
static constexpr int HTTP_CONNECT_TIMEOUT_MS = 1000 * 10;
static constexpr int HTTP_RECEIVE_TIMEOUT_MS = 1000 * 10;
static constexpr size_t JSON_BUFFER_SIZE = 128;
static constexpr size_t HTTP_BUFFER_SIZE = 512;

namespace {

WioCellularConfig CreateCellularConfig() {
  return WioCellularConfig{
      APN,
      PDP_CONTEXT_ID,
      SORACOM_USER,
      SORACOM_PASS,
      POWER_ON_TIMEOUT_MS,
      NETWORK_TIMEOUT_MS,
      COMMAND_TIMEOUT_MS,
      HTTP_CONNECT_TIMEOUT_MS,
      HTTP_RECEIVE_TIMEOUT_MS,
      HTTP_BUFFER_SIZE,
  };
}

FunnelPublisherConfig CreatePublisherConfig() {
  return FunnelPublisherConfig{
      FUNNEL_HOST,
      FUNNEL_PORT,
      FUNNEL_PATH,
      SEND_INTERVAL_MS,
      JSON_BUFFER_SIZE,
  };
}

WioCellularHttpClient cellularClient(CreateCellularConfig());
FunnelPublisherApp app(cellularClient, CreatePublisherConfig());

}  // namespace

static void abortHandler(int sig) {
  Serial.printf("ABORT: Signal %d received\n", sig);
  yield();

  vTaskSuspendAll();
  while (true) {
    ledOn(LED_BUILTIN);
    nrfx_coredep_delay_us(100000);
    ledOff(LED_BUILTIN);
    nrfx_coredep_delay_us(100000);
  }
}

void setup() {
  signal(SIGABRT, abortHandler);
  Serial.begin(115200);
  const auto start = millis();
  while (!Serial && millis() - start < 5000) {
    delay(2);
  }

  Serial.println();
  Serial.println("Startup Wio BG770A -> SORACOM Funnel HTTP sender");

  cellularClient.begin();
}

void loop() {
  static bool ready = false;

  if (!ready) {
    digitalWrite(LED_BUILTIN, HIGH);
    ready = app.initialize();
    if (!ready) {
      abort();
    }
    digitalWrite(LED_BUILTIN, LOW);
  }
  app.loop();
}
