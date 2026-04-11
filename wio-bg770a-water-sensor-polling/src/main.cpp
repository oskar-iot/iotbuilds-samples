#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <variant.h>

namespace {

constexpr uint32_t kSerialBaudRate = 115200;
constexpr uint32_t kLoopIntervalMs = 1000;
constexpr int kAnalogWetThreshold = 700;
constexpr uint8_t kAnalogSensorPin = A4;   // Grove - Analog (P1)
constexpr uint8_t kDigitalSensorPin = D28; // Same Grove - Analog (P1)

bool waitForSerial(uint32_t timeoutMs) {
  const auto start = millis();
  while (!Serial && millis() - start < timeoutMs) {
    delay(2);
  }
  return Serial;
}

bool isWetFromAnalog(int analogValue) {
  return analogValue < kAnalogWetThreshold;
}

}  // namespace

void setup() {
  Serial.begin(kSerialBaudRate);
  waitForSerial(5000);
  Serial.println();
  Serial.println("Wio BG770A + Grove Water Sensor");

  digitalWrite(PIN_VGROVE_ENABLE, VGROVE_ENABLE_ON);
  pinMode(PIN_VGROVE_ENABLE, OUTPUT);
  delay(10);

  pinMode(kAnalogSensorPin, INPUT);
  pinMode(kDigitalSensorPin, INPUT);
  Serial.printf(
      "Grove Analog P1: analog=A4(%u) digital=D28(%u), wet_threshold=%d\n",
      kAnalogSensorPin,
      kDigitalSensorPin,
      kAnalogWetThreshold);
  Serial.println("This connector supports both analogRead() and digitalRead()");
  Serial.println("Lower analog value means wetter");
}

void loop() {
  const int analogValue = analogRead(kAnalogSensorPin);
  const int digitalValue = digitalRead(kDigitalSensorPin);
  Serial.printf(
      "analog=%d digital=%d threshold=%d state=%s\n",
      analogValue,
      digitalValue,
      kAnalogWetThreshold,
      isWetFromAnalog(analogValue) ? "WET" : "DRY");
  delay(kLoopIntervalMs);
}
