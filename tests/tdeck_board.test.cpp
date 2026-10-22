#include <cassert>
#include <Arduino.h>
#include <Wire.h>
#include "../variants/lilygo_tdeck/TDeckBoard.h"
FakeWire Wire;
int main() {
  TDeckBoard board;
  board.begin();
  assert(Wire.started && Wire.sda==18 && Wire.scl==8);
  // ESP32 Wire.begin on a running bus is a no-op, not a repair mechanism.
  Wire.begin(8,9);
  assert(Wire.sda==18 && Wire.scl==8);
}
