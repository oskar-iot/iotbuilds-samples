#pragma once

struct TouchEvent {
  bool was_pressed = false;
  int x = -1;
  int y = -1;
};

void initializeDevice();
void updateDevice();
TouchEvent readTouchEvent();
int readBatteryLevel();
int displayWidth();
int displayHeight();
