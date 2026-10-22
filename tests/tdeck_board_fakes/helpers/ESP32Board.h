#pragma once
#include <Wire.h>
struct ESP32Board {
  int startup_reason=0;
  // Match the production base class: it starts Wire with default pins.
  void begin() { Wire.begin(); }
  virtual uint16_t getBattMilliVolts() { return 0; }
};
