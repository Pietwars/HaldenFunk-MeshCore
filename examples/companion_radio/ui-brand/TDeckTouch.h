#pragma once
// Touch panel of the LilyGO T-Deck Plus (Goodix GT911 on the shared I2C bus SDA 18 / SCL 8), standalone build only.
// Polled (no interrupt pin): status register 0x814E, first point at 0x8150, status cleared after each read.
// Orientation as in LilyGO's T-Deck examples (max 320 x 240, swap XY, mirror Y): screen x = raw y,
// screen y = 240 - raw x. To be confirmed on hardware (docs/STANDALONE.md, hardware test list).
#include <Arduino.h>
#include <Wire.h>

namespace tdeck {
class Touch {
  uint8_t addr = 0;
  bool last = false;   // finger state of the last frame
  bool readReg(uint16_t reg, uint8_t *buf, uint8_t n) {
    Wire.beginTransmission(addr); Wire.write(uint8_t(reg >> 8)); Wire.write(uint8_t(reg & 0xFF));
    if (Wire.endTransmission() != 0) return false;
    if (Wire.requestFrom(addr, n) != n) return false;
    for (uint8_t i = 0; i < n; ++i) buf[i] = uint8_t(Wire.read());
    return true;
  }
  void clearStatus() {
    Wire.beginTransmission(addr); Wire.write(uint8_t(0x81)); Wire.write(uint8_t(0x4E)); Wire.write(uint8_t(0)); Wire.endTransmission();
  }
public:
  bool present() const { return addr != 0; }
  // GT911 answers on 0x5D or 0x14 (address latched at reset); the product id register reads "911".
  bool probe() {
    const uint8_t cand[2] = {0x5D, 0x14};
    for (uint8_t a : cand) {
      addr = a; uint8_t id[3];
      if (readReg(0x8140, id, 3) && id[0] == '9' && id[1] == '1' && id[2] == '1') {
        Serial.printf("[TDECK] Touch GT911 erkannt (I2C 0x%02X)\n", a);
        return true;
      }
    }
    addr = 0; return false;
  }
  // true while a finger is down; x/y in screen coordinates (landscape 320 x 240).
  bool read(int &x, int &y) {
    if (!addr) return false;
    uint8_t st;
    if (!readReg(0x814E, &st, 1)) { addr = 0; last = false; return false; }   // gone: probe again later
    if (!(st & 0x80)) return last;                              // no new frame yet: keep the last state
    uint8_t n = st & 0x0F, p[4];
    bool down = n > 0 && n <= 5 && readReg(0x8150, p, 4);
    clearStatus();
    if (down) {
      int rx = p[0] | (p[1] << 8), ry = p[2] | (p[3] << 8);
      x = ry; y = 240 - rx;
      if (x < 0) x = 0;
      if (x > 319) x = 319;
      if (y < 0) y = 0;
      if (y > 239) y = 239;
    }
    last = down;
    return down;
  }
};
}
