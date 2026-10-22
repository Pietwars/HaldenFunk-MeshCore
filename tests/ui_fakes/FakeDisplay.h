#pragma once
// Host display for the T-Deck standalone pages: 320 x 240 RGB565 framebuffer. Records any drawing outside the
// screen (page bound test) and, with a GFX 5x7 font table (tools/standalone_preview.py), renders real pixels.
#include <helpers/ui/DisplayDriver.h>
#include <vector>
#include <string>

struct FakeDisplay : DisplayDriver {
  static constexpr int W = 320, H = 240;
  uint16_t fb[W * H];
  const unsigned char *font = nullptr;   // 256 x 5 bytes (Adafruit glcdfont), optional
  unsigned outOfBounds = 0, texts = 0;
  std::string lastText, allText;
  bool on = true;
  FakeDisplay() : DisplayDriver(W, H) { for (auto &p : fb) p = 0; }
  void px(int x, int y, uint16_t c) { if (x >= 0 && y >= 0 && x < W && y < H) fb[y * W + x] = c; }
  bool isOn() override { return on; }
  void turnOn() override { on = true; } void turnOff() override { on = false; } void clear() override { }
  void startFrame(ColorVal c) override { for (auto &p : fb) p = c; allText.clear(); }
  void setTextSize(int) override { } void setColor(ColorVal) override { }
  void setCursor(int, int) override { } void print(const char *) override { }
  void fillRect(int, int, int, int) override { } void drawRect(int, int, int, int) override { }
  void drawXbm(int, int, const uint8_t *, int, int) override { }
  uint16_t getTextWidth(const char *s) override { return uint16_t(strlen(s) * 6); }
  int nativeWidth() override { return W; }
  int nativeHeight() override { return H; }
  void check(int x, int y, int w, int h) { if (x < 0 || y < 0 || w < 0 || h < 0 || x + w > W || y + h > H) ++outOfBounds; }
  void nativeFill(int x, int y, int w, int h, uint16_t c) override {
    check(x, y, w, h);
    for (int j = 0; j < h; ++j) for (int i = 0; i < w; ++i) px(x + i, y + j, c);
  }
  void nativeRect(int x, int y, int w, int h, uint16_t c) override {
    check(x, y, w, h);
    for (int i = 0; i < w; ++i) { px(x + i, y, c); px(x + i, y + h - 1, c); }
    for (int j = 0; j < h; ++j) { px(x, y + j, c); px(x + w - 1, y + j, c); }
  }
  void nativeLine(int x0, int y0, int x1, int y1, uint16_t c) override {
    check(x0, y0, 1, 1); check(x1, y1, 1, 1);
    DisplayDriver::nativeLine(x0, y0, x1, y1, c);
  }
  void nativeDisc(int x, int y, int r, uint16_t c) override {
    check(x - r, y - r, 2 * r + 1, 2 * r + 1);
    for (int j = -r; j <= r; ++j) for (int i = -r; i <= r; ++i) if (i * i + j * j <= r * r + r) px(x + i, y + j, c);
  }
  void nativeImage(int x, int y, int w, int h, const uint16_t *rgb) override {
    check(x, y, w, h);
    for (int j = 0; j < h; ++j) for (int i = 0; i < w; ++i) px(x + i, y + j, rgb[j * w + i]);
  }
  void nativeText(int x, int y, int size, uint16_t c, const char *s) override {
    int n = int(strlen(s));
    check(x, y, n * 6 * size, 8 * size);
    ++texts; lastText = s; allText += s; allText += '\n';
    if (!font) return;
    for (int k = 0; k < n; ++k) {
      unsigned char ch = (unsigned char)s[k];
      for (int col = 0; col < 5; ++col) {
        unsigned char bits = font[ch * 5 + col];
        for (int row = 0; row < 8; ++row) if (bits & (1 << row))
          for (int a = 0; a < size; ++a) for (int b = 0; b < size; ++b) px(x + (k * 6 + col) * size + a, y + row * size + b, c);
      }
    }
  }
  void endFrame() override { }
  bool writePpm(const char *path) const {
    FILE *f = fopen(path, "wb"); if (!f) return false;
    fprintf(f, "P6\n%d %d\n255\n", W, H);
    for (int i = 0; i < W * H; ++i) {
      uint16_t p = fb[i];
      unsigned char rgb[3] = {(unsigned char)(((p >> 11) & 31) * 255 / 31), (unsigned char)(((p >> 5) & 63) * 255 / 63),
                              (unsigned char)((p & 31) * 255 / 31)};
      fwrite(rgb, 1, 3, f);
    }
    fclose(f); return true;
  }
};
