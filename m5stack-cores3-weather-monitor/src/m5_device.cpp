#include <M5Unified.h>

#include "m5_device.h"

void initializeDevice() {
  auto cfg = M5.config();
  M5.begin(cfg);
}

void updateDevice() {
  M5.update();
}

TouchEvent readTouchEvent() {
  const auto touch = M5.Touch.getDetail();
  TouchEvent event;
  event.was_pressed = touch.wasPressed();
  event.x = touch.x;
  event.y = touch.y;
  return event;
}

int readBatteryLevel() {
  return M5.Power.getBatteryLevel();
}

int displayWidth() {
  return M5.Display.width();
}

int displayHeight() {
  return M5.Display.height();
}
