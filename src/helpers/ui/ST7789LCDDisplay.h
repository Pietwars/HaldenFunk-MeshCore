#pragma once

#include "DisplayDriver.h"
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <helpers/RefCountedDigitalPin.h>

#if defined(LILYGO_TDECK)
  // T-Deck: display (TFT_CS 12) and LoRa radio (NSS 9) share SCK 40 / MOSI 41 / MISO 38.
  // Both MUST use the same SPI controller instance, otherwise the GPIO matrix routes
  // the shared pins to whichever controller was attached last (display freezes after radio init).
  // Defined in variants/lilygo_tdeck/target.cpp.
  extern SPIClass tdeck_shared_spi;
  void tdeck_shared_spi_begin();
#endif

class ST7789LCDDisplay : public DisplayDriver {
  #if defined(HELTEC_LORA_V4_TFT) || defined(HELTEC_V4_R8_TFT)
    SPIClass displaySPI;
  #endif
  Adafruit_ST7789 display;
  bool _isOn;
  uint16_t _color;
  RefCountedDigitalPin* _peripher_power;

  bool i2c_probe(TwoWire& wire, uint8_t addr);
public:
#ifdef USE_PIN_TFT
  ST7789LCDDisplay(RefCountedDigitalPin* peripher_power=NULL) : DisplayDriver(128, 64), 
      display(PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_SDA, PIN_TFT_SCL, PIN_TFT_RST),
      _peripher_power(peripher_power)
  {
    _isOn = false;
  }
#elif defined(LILYGO_TDECK)
  ST7789LCDDisplay(RefCountedDigitalPin* peripher_power=NULL) : DisplayDriver(128, 64),
      display(&tdeck_shared_spi, PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST),
      _peripher_power(peripher_power)
  {
    _isOn = false;
  }
#elif defined(HELTEC_LORA_V4_TFT) || defined(HELTEC_V4_R8_TFT)
  ST7789LCDDisplay(RefCountedDigitalPin* peripher_power=NULL) : DisplayDriver(128, 64),
      displaySPI(HSPI),
      display(&displaySPI, PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST),
      _peripher_power(peripher_power)
  {
    _isOn = false;
  }
#else
  ST7789LCDDisplay(RefCountedDigitalPin* peripher_power=NULL) : DisplayDriver(128, 64), 
      display(&SPI, PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST),
      _peripher_power(peripher_power)
  {
    _isOn = false;
  }
#endif
  bool begin();

  bool isOn() override { return _isOn; }
  void turnOn() override;
  void turnOff() override;
  void clear() override;
  void startFrame(ColorVal bkg = UIColor::window_bkg) override;
  void setTextSize(int sz) override;
  void setColor(ColorVal c) override;
  void setCursor(int x, int y) override;
  void print(const char* str) override;
  void fillRect(int x, int y, int w, int h) override;
  void drawRect(int x, int y, int w, int h) override;
  void drawXbm(int x, int y, const uint8_t* bits, int w, int h) override;
  uint16_t getTextWidth(const char* str) override;
  void endFrame() override;

  int  nativeWidth() override  { return display.width(); }
  int  nativeHeight() override { return display.height(); }
  void nativeFill(int x, int y, int w, int h, uint16_t c) override { display.fillRect(x, y, w, h, c); }
  void nativeRect(int x, int y, int w, int h, uint16_t c) override { display.drawRect(x, y, w, h, c); }
  void nativeText(int x, int y, int size, uint16_t c, const char* s) override {
    display.setTextSize(size); display.setTextColor(c); display.setCursor(x, y); display.print(s);
  }
  void nativeImage(int x, int y, int w, int h, const uint16_t* rgb565) override {
    display.drawRGBBitmap(x, y, rgb565, w, h);
  }
  void nativeLine(int x0, int y0, int x1, int y1, uint16_t c) override { display.drawLine(x0, y0, x1, y1, c); }
  void nativeDisc(int x, int y, int r, uint16_t c) override { display.fillCircle(x, y, r, c); }
};
