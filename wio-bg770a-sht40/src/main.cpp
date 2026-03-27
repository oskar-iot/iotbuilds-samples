#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <Wire.h>
#include <variant.h>

namespace {

constexpr uint32_t kSerialBaudRate = 115200;
constexpr uint8_t kSht4xAddress = 0x44;
constexpr uint8_t kSht4xMeasureMediumPrecision = 0xF6;
constexpr uint32_t kMeasurementDelayMs = 10;
constexpr uint32_t kLoopIntervalMs = 2000;

struct Sht4xReading {
  float temperatureC;
  float humidityPercent;
};

bool waitForSerial(uint32_t timeoutMs) {
  const auto start = millis();
  while (!Serial && millis() - start < timeoutMs) {
    delay(2);
  }
  return Serial;
}

uint8_t crc8(const uint8_t* data, size_t len) {
  uint8_t crc = 0xFF;
  for (size_t i = 0; i < len; ++i) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0x31) : static_cast<uint8_t>(crc << 1);
    }
  }
  return crc;
}

bool readSht4x(uint8_t address, Sht4xReading& reading) {
  Wire.beginTransmission(address);
  Wire.write(kSht4xMeasureMediumPrecision);
  if (Wire.endTransmission() != 0) {
    return false;
  }

  delay(kMeasurementDelayMs);

  constexpr size_t responseSize = 6;
  uint8_t response[responseSize] = {};
  const auto received = Wire.requestFrom(static_cast<int>(address), static_cast<int>(responseSize));
  if (received != static_cast<int>(responseSize)) {
    while (Wire.available()) {
      Wire.read();
    }
    return false;
  }

  for (size_t i = 0; i < responseSize; ++i) {
    if (!Wire.available()) {
      return false;
    }
    response[i] = static_cast<uint8_t>(Wire.read());
  }

  if (crc8(response, 2) != response[2] || crc8(response + 3, 2) != response[5]) {
    return false;
  }

  const uint16_t temperatureTicks = static_cast<uint16_t>(response[0] << 8) | response[1];
  const uint16_t humidityTicks = static_cast<uint16_t>(response[3] << 8) | response[4];

  reading.temperatureC = -45.0f + 175.0f * static_cast<float>(temperatureTicks) / 65535.0f;
  reading.humidityPercent = -6.0f + 125.0f * static_cast<float>(humidityTicks) / 65535.0f;
  if (reading.humidityPercent < 0.0f) {
    reading.humidityPercent = 0.0f;
  } else if (reading.humidityPercent > 100.0f) {
    reading.humidityPercent = 100.0f;
  }

  return true;
}

}  // namespace

void setup() {
  Serial.begin(kSerialBaudRate);
  waitForSerial(5000);
  Serial.println();
  Serial.println("Wio BG770A + SHT4x");

  digitalWrite(PIN_VGROVE_ENABLE, VGROVE_ENABLE_ON);
  pinMode(PIN_VGROVE_ENABLE, OUTPUT);
  delay(10);

  Wire.begin();
  Wire.setClock(100000);

  Serial.printf("SHT4x configured at 0x%02X (SCL=D4 SDA=D5)\n", kSht4xAddress);
}

void loop() {
  Sht4xReading reading{};
  if (!readSht4x(kSht4xAddress, reading)) {
    Serial.println("ERROR: failed to read SHT4x");
    delay(kLoopIntervalMs);
    return;
  }

  Serial.printf("temperature=%.2f C, humidity=%.2f %%RH\n", reading.temperatureC, reading.humidityPercent);
  delay(kLoopIntervalMs);
}
