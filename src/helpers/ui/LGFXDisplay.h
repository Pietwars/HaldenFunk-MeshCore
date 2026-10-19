#pragma once

#include <helpers/ui/DisplayDriver.h>

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#ifndef UI_ZOOM
  #define UI_ZOOM 1
#endif
#ifndef LGFX_SPRITE_DEPTH
  #define LGFX_SPRITE_DEPTH 8   // 16 = volle Farben fuer Bilder (Sprite im PSRAM)
#endif
#ifndef UI_ZOOM_X
  #define UI_ZOOM_X UI_ZOOM
#endif
#ifndef UI_ZOOM_Y
  #define UI_ZOOM_Y UI_ZOOM
#endif

class LGFXDisplay : public DisplayDriver {
protected:
  LGFX_Device* display;
  LGFX_Sprite buffer;

  bool _isOn = false;
  int _color = TFT_WHITE;

public:
  LGFXDisplay(int w, int h, LGFX_Device &disp)
    : DisplayDriver(w/UI_ZOOM_X, h/UI_ZOOM_Y), display(&disp) {}
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
  virtual bool getTouch(int *x, int *y);

  int  nativeWidth() override  { return width(); }    // mit UI_ZOOM 1 sind logisch = echte Pixel
  int  nativeHeight() override { return height(); }
  void nativeFill(int x, int y, int w, int h, uint16_t c) override { buffer.fillRect(x, y, w, h, c); }
  void nativeRect(int x, int y, int w, int h, uint16_t c) override { buffer.drawRect(x, y, w, h, c); }
  // Native Texte in glatten LovyanGFX-Schriften statt der 5x7-Pixelschrift (auf 240x240 bei Groesse 1 kaum lesbar):
  // 1 -> FreeSans 9 pt, 2 -> FreeSansBold 12 pt, 3 -> FreeSansBold 18 pt, ab 4 -> FreeSansBold 24 pt
  const lgfx::IFont* nativeFont(int size) {
    switch (size) {
      case 1:  return &fonts::FreeSans9pt7b;
      case 2:  return &fonts::FreeSansBold12pt7b;
      case 3:  return &fonts::FreeSansBold18pt7b;
      default: return &fonts::FreeSansBold24pt7b;
    }
  }
  void nativeText(int x, int y, int size, uint16_t c, const char* s) override {
    buffer.setFont(nativeFont(size)); buffer.setTextSize(1); buffer.setTextDatum(lgfx::top_left);
    buffer.setTextColor(c); buffer.drawString(s, x, y);
    buffer.setFont(&fonts::Font0);
  }
  int nativeTextWidth(const char* s, int size) override {
    buffer.setFont(nativeFont(size)); buffer.setTextSize(1); int w = buffer.textWidth(s); buffer.setFont(&fonts::Font0); return w;
  }
  int nativeTextHeight(int size) override {
    buffer.setFont(nativeFont(size)); buffer.setTextSize(1); int h = buffer.fontHeight(); buffer.setFont(&fonts::Font0); return h;
  }
  void nativeImage(int x, int y, int w, int h, const uint16_t* rgb565) override {
    buffer.pushImage(x, y, w, h, (const lgfx::rgb565_t*)rgb565);
  }
};
