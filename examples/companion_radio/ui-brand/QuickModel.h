#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "QuickData.h"
#include "../LocalSend.h"

namespace quick {
constexpr size_t TEXT_LIMIT = 100; // same UTF-8 byte limit as the web sender, including location
constexpr int PRESET_COUNT = sizeof(PRESETS) / sizeof(PRESETS[0]);
constexpr int PLACE_COUNT = sizeof(PLACES) / sizeof(PLACES[0]);
enum Focus { Presets, Text, PlaceSelect, Target, Send, Position, Home, Clear, FOCUS_COUNT };
enum class Result { Ready, Empty, NoFix, TooLong, InvalidTime };

struct Fix {
  bool valid; double lat, lon;
  Fix(bool v = false, double la = 0, double lo = 0) : valid(v), lat(la), lon(lo) { }
};
inline bool validFix(const Fix& f) {
  return f.valid && isfinite(f.lat) && isfinite(f.lon) &&
         f.lat >= -90 && f.lat <= 90 && f.lon >= -180 && f.lon <= 180 &&
         (f.lat != 0 || f.lon != 0);
}

class Model {
public:
  char text[TEXT_LIMIT + 1] = {};
  int preset = 0;
  int place = 0; // 0=device GPS (default); -1=no location; 1..12=explicit manual place
  Focus focus = Presets;
  Model() { selectPreset(0); }
  void selectPreset(int index) {
    if (index < 0 || index >= PRESET_COUNT) return;
    preset = index;
    snprintf(text, sizeof(text), "[%s] %s", PRESETS[index].kind, PRESETS[index].text);
  }
  bool append(char key) {
    size_t n = strlen(text);
    if (key < 32 || key > 126 || n >= TEXT_LIMIT) return false;
    text[n] = key; text[n + 1] = 0; return true;
  }
  void backspace() {
    size_t n = strlen(text);
    if (!n) return;
    do { --n; } while (n && (static_cast<uint8_t>(text[n]) & 0xC0) == 0x80);
    text[n] = 0; // delete a complete UTF-8 character, including preset umlauts
  }
  void cyclePlace(int direction) {
    place += direction;
    if (place > PLACE_COUNT) place = -1;
    if (place < -1) place = PLACE_COUNT;
  }
  Fix selectedFix(const Fix& gps) const {
    if (place > 0 && place <= PLACE_COUNT)
      return {true, PLACES[place - 1].lat, PLACES[place - 1].lon};
    return gps;
  }
  Result compose(bool position, const Fix& gps, uint32_t stamp, char* out, size_t capacity) const {
    if (capacity) out[0] = 0;
    if (stamp < 1700000000u || stamp > 4102444800u) return Result::InvalidTime;
    if (place < -1 || place > PLACE_COUNT) return Result::NoFix;
    Fix fix = selectedFix(gps);
    if ((position || place >= 0) && !validFix(fix)) return Result::NoFix;
    char buffer[192];
    int length;
    if (position) {
      length = snprintf(buffer, sizeof(buffer), "!DP1|%.4f,%.4f|%lu", fix.lat, fix.lon, (unsigned long)stamp);
    } else {
      const char* start = text;
      while (*start == ' ') ++start;
      size_t n = strlen(start);
      while (n && start[n - 1] == ' ') --n;
      if (!n) return Result::Empty;
      if (place < 0) length = snprintf(buffer, sizeof(buffer), "%.*s", (int)n, start);
      else if (place == 0) length = snprintf(buffer, sizeof(buffer), "%.*s @ %.4f,%.4f", (int)n, start, fix.lat, fix.lon);
      else length = snprintf(buffer, sizeof(buffer), "%.*s @ %s %.4f,%.4f", (int)n, start, PLACES[place - 1].name, fix.lat, fix.lon);
    }
    if (length < 0 || length > (int)TEXT_LIMIT || (size_t)length >= capacity) return Result::TooLong;
    memcpy(out, buffer, length + 1);
    return Result::Ready;
  }
};

// Restricted plaintext Companion frame builder: the shared web-free one (LocalSend.h).
inline size_t frame(const char* target, const char* text, uint32_t stamp, uint8_t* out, size_t capacity) {
  return disaster::textFrame(target, text, stamp, out, capacity);
}
}
