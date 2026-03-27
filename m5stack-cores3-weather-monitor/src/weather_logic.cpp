#include <ArduinoJson.h>

#include "weather_logic.h"

namespace {

String formatJstTimestamp(const String& iso8601_text) {
  if (iso8601_text.length() < 16) {
    return "-";
  }

  String formatted = iso8601_text;
  formatted.replace("T", " ");
  return formatted + " JST";
}

String weatherCodeText(const String& weather_code) {
  if (weather_code == "0") {
    return "Clear";
  }
  if (weather_code == "1" || weather_code == "2" || weather_code == "3") {
    return "Cloudy";
  }
  if (weather_code == "45" || weather_code == "48") {
    return "Fog";
  }
  if (weather_code == "51" || weather_code == "53" || weather_code == "55" ||
      weather_code == "56" || weather_code == "57") {
    return "Drizzle";
  }
  if (weather_code == "61" || weather_code == "63" || weather_code == "65" ||
      weather_code == "66" || weather_code == "67" || weather_code == "80" ||
      weather_code == "81" || weather_code == "82") {
    return "Rain";
  }
  if (weather_code == "71" || weather_code == "73" || weather_code == "75" ||
      weather_code == "77" || weather_code == "85" || weather_code == "86") {
    return "Snow";
  }
  if (weather_code == "95" || weather_code == "96" || weather_code == "99") {
    return "Thunder";
  }
  return "Code " + weather_code;
}

}  // namespace

bool parseWeatherPayload(const String& payload, String& updated_at, String& summary) {
  JsonDocument doc;
  const DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    return false;
  }

  const JsonVariant current = doc["current"];
  if (current.isNull()) {
    return false;
  }

  const char* time = current["time"];
  const JsonVariant temperature = current["temperature_2m"];
  const JsonVariant weather_code = current["weather_code"];
  if (time == nullptr || temperature.isNull() || weather_code.isNull()) {
    return false;
  }

  updated_at = formatJstTimestamp(String(time));
  summary = "Tokyo " + String(temperature.as<float>(), 1) + "C " +
            weatherCodeText(String(weather_code.as<int>()));
  return true;
}
