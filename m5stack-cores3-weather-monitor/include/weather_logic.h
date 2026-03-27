#pragma once

#include <Arduino.h>

bool parseWeatherPayload(const String& payload, String& updated_at, String& summary);
