#pragma once
#include <Wire.h>

// GT911 on the existing shared I2C bus. T-Deck has INT on 16, no reset GPIO.
// Register layout follows Goodix GT911 and LilyGO's TouchDrvGT911 driver.
class TDeckTouch {
public:
  bool begin() {
    pinMode(16, INPUT);
    for (uint8_t address : {uint8_t(0x5d), uint8_t(0x14)}) {
      address_ = address;
      uint8_t id[4];
      if (read(0x8140, id, 4) && id[0]=='9' && id[1]=='1' && id[2]=='1') return true;
    }
    address_ = 0;
    return false;
  }
  // False means no fresh sample, not a finger release.
  bool sample(bool &down, int &x, int &y) {
    uint8_t status;
    if (!address_ || !read(0x814e, &status, 1) || !(status & 0x80)) return false;
    const uint8_t count = status & 0x0f;
    uint8_t point[8];
    const bool valid = count == 1 && read(0x814f, point, 8);
    Wire.beginTransmission(address_); Wire.write(0x81); Wire.write(0x4e); Wire.write(uint8_t(0));
    if (Wire.endTransmission() != 0) return false;
    if (count > 1) {down=true; x=y=-1; return true;} // cancel this gesture until release
    if (count == 1 && !valid) return false;
    down = count == 1;
    if (down) {
      x = point[1] | (point[2] << 8); y = point[3] | (point[4] << 8);
      if (x > 4095 || y > 4095) return false;
    }
    return true;
  }
private:
  uint8_t address_ = 0;
  bool read(uint16_t reg, uint8_t *data, uint8_t length) {
    Wire.beginTransmission(address_); Wire.write(reg >> 8); Wire.write(reg & 255);
    if (Wire.endTransmission(false) != 0 || Wire.requestFrom(address_, length) != length) return false;
    for (uint8_t i=0; i<length; ++i) data[i] = Wire.read();
    return true;
  }
};
