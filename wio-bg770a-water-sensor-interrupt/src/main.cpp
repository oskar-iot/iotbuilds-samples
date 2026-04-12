#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <variant.h>

namespace {

constexpr uint32_t kSerialBaudRate = 115200;
constexpr uint32_t kIdleDelayMs = 20;
constexpr uint8_t kDigitalSensorPin = D28; // Grove P1 digital pin

volatile bool gDigitalStateChanged = false;

bool waitForSerial(uint32_t timeoutMs) {
  const auto start = millis();
  while (!Serial && millis() - start < timeoutMs) {
    delay(2);
  }
  return Serial;
}

const char* toLevelName(int digitalValue) {
  return digitalValue == LOW ? "LOW" : "HIGH";
}

void onDigitalStateChanged() {
  gDigitalStateChanged = true;
}

void printSensorState(const char* reason, int digitalValue) {
  Serial.printf("[%s] digital=%s(%d)\n", reason, toLevelName(digitalValue),
                digitalValue);
}

}  // namespace

void setup() {
  Serial.begin(kSerialBaudRate);
  waitForSerial(5000);
  Serial.println();
  Serial.println("Wio BG770A Water Sensor Interrupt Sample");

  digitalWrite(PIN_VGROVE_ENABLE, VGROVE_ENABLE_ON);
  pinMode(PIN_VGROVE_ENABLE, OUTPUT);
  delay(10);

  pinMode(kDigitalSensorPin, INPUT);
  Serial.printf("Grove P1 digital input: D28(%u)\n", kDigitalSensorPin);
  Serial.println("Interrupt mode: detect digital HIGH/LOW changes");

  attachInterrupt(digitalPinToInterrupt(kDigitalSensorPin), onDigitalStateChanged, CHANGE);
  printSensorState("startup", digitalRead(kDigitalSensorPin));
}

void loop() {
  static int lastReportedState = digitalRead(kDigitalSensorPin);

  bool stateChanged = false;

  noInterrupts();
  stateChanged = gDigitalStateChanged;
  interrupts();

  if (stateChanged) {
    noInterrupts();
    gDigitalStateChanged = false;
    interrupts();

    const int currentState = digitalRead(kDigitalSensorPin);

    if (currentState != lastReportedState) {
      lastReportedState = currentState;
      printSensorState("digital-change", currentState);
    }
  }

  delay(kIdleDelayMs);
}
