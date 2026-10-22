#pragma once
#include "Theme.h"

// One geometry contract for rendering, hit testing and the host regression tests.
namespace touchui {
struct Rect {
  int x, y, w, h;
  bool contains(int px, int py) const { return px >= x && py >= y && px < x + w && py < y + h; }
};
constexpr int GAP = 4, ROW = 38, CONTENT = theme::NAV_H + 4, BOTTOM = 228;
constexpr int VISIBLE_ROWS = (BOTTOM - CONTENT) / ROW;
constexpr int COMPOSE_Y = 188, COMPOSE_H = 40, SEND_X = 230;
inline Rect row(int n) {
  return { 4, CONTENT + n * ROW, 312, ROW - GAP };
}
inline Rect tile(int n) {
  return { 4 + (n % 2) * 158, CONTENT + (n / 2) * 52, 154, 48 };
}
inline Rect send() {
  return { SEND_X, COMPOSE_Y, 86, COMPOSE_H };
}
inline Rect mapControl(int n) {
  return { 4 + n * 105, 194, 101, 34 };
}
inline Rect quickControl(int n) {
  switch (n) {
  case 0:
    return { 4, 68, 312, 30 }; // template picker
  case 1:
    return { 4, 102, 256, 38 }; // physical keyboard text field
  case 2:
    return { 264, 102, 52, 38 }; // clear
  case 3:
    return { 4, 144, 154, 32 }; // place picker
  case 4:
    return { 162, 144, 154, 32 }; // recipient picker
  case 5:
    return { 4, 180, 154, 38 }; // message
  default:
    return { 162, 180, 154, 38 }; // position only
  }
}
} // namespace touchui
