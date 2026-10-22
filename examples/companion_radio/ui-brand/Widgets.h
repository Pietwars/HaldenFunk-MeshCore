#pragma once
// Drawing helpers of the T-Deck Plus standalone display (Issue #35), web look (Theme.h) on 320 x 240.
#include <helpers/ui/DisplayDriver.h>
#include "Theme.h"

namespace ui {
// UTF-8 text in the GFX font, cut after max_chars characters and always at the right screen edge.
inline void text(DisplayDriver *d, int x, int y, uint16_t c, const char *utf8, int max_chars = 53, int size = 1) {
  char buf[160];
  int room = (theme::W - x) / (theme::CW * size);
  if (max_chars > room) max_chars = room;
  if (max_chars < 0) max_chars = 0;
  theme::toCp437(utf8, buf, sizeof(buf), size_t(max_chars));
  d->nativeText(x, y, size, c, buf);
}
// Text that is already CP437 (or plain ASCII).
inline void raw(DisplayDriver *d, int x, int y, uint16_t c, const char *cp, size_t n, int size = 1) {
  char buf[160]; if (n >= sizeof(buf)) n = sizeof(buf) - 1;
  memcpy(buf, cp, n); buf[n] = 0; d->nativeText(x, y, size, c, buf);
}
inline int textWidth(const char *utf8, int size = 1) {
  char buf[160]; return int(theme::toCp437(utf8, buf, sizeof(buf))) * theme::CW * size;
}
inline void rightText(DisplayDriver *d, int xr, int y, uint16_t c, const char *utf8) { text(d, xr - textWidth(utf8), y, c, utf8); }
inline void centerText(DisplayDriver *d, int cx, int y, uint16_t c, const char *utf8, int size = 1) {
  text(d, cx - textWidth(utf8, size) / 2, y, c, utf8, 53, size);
}
// Web buttons: .b (mesh green, dark text) and .b2 (#24384a, light text); focus = orange outline like the
// T-Deck quick page of 0.9.5-test.
constexpr uint16_t FOCUS = 0xfce0;
inline void button(DisplayDriver *d, int x, int y, int w, int h, const char *label, bool focused, bool primary = false) {
  d->nativeFill(x, y, w, h, primary ? theme::MESH : theme::BUTTON2);
  d->nativeRect(x, y, w, h, focused ? FOCUS : (primary ? theme::MESH : theme::LINE));
  if (focused) d->nativeRect(x + 1, y + 1, w - 2, h - 2, FOCUS);
  centerText(d, x + w / 2, y + (h - theme::CH) / 2, primary ? theme::INK : theme::TEXT, label);
}
inline void badge(DisplayDriver *d, int xr, int y, unsigned n, uint16_t bg = theme::WINE) {
  char t[8]; snprintf(t, sizeof(t), "%u", n > 99 ? 99 : n);
  int w = int(strlen(t)) * theme::CW + 4;
  d->nativeFill(xr - w, y, w, 9, bg); d->nativeText(xr - w + 2, y + 1, 1, theme::TEXT, t);
}
// Battery level shown at the right end of every status line (set by the UI from UITask's battery reading;
// -1 = unknown). One value for all pages, including the Schnellmeldung page that draws its own status line.
inline int &navBattery() { static int pct = -1; return pct; }
// Battery field: outline icon filled by level (green from 40 %, amber from 15 %, else red) and "87%".
inline void navBatteryField(DisplayDriver *d) {
  const int x0 = theme::W - theme::BATT_W, pct = navBattery();
  d->nativeFill(x0, theme::TAB_H, theme::BATT_W, theme::STATUS_H - 1, theme::NAV_BG);
  d->nativeFill(x0, theme::TAB_H + 2, 1, theme::STATUS_H - 5, theme::LINE);
  const int ix = x0 + 4, iy = theme::TAB_H + 10, iw = 13, ih = 7;
  d->nativeRect(ix, iy, iw, ih, theme::TEXT);
  d->nativeFill(ix + iw, iy + 2, 2, ih - 4, theme::TEXT);
  uint16_t c = pct >= 40 ? theme::MESH : pct >= 15 ? theme::PEND : theme::BAD;
  if (pct > 0) { int fw = (iw - 4) * (pct > 100 ? 100 : pct) / 100; if (fw < 1) fw = 1; d->nativeFill(ix + 2, iy + 2, fw, ih - 4, c); }
  char t[8]; if (pct >= 0) snprintf(t, sizeof(t), "%d%%", pct > 100 ? 100 : pct); else snprintf(t, sizeof(t), "--%%");
  d->nativeText(theme::W - 2 - int(strlen(t)) * theme::CW, theme::TAB_H + 10, 1,
                pct >= 0 && pct < 15 ? theme::BAD : theme::TEXT, t);
}
// Original standalone tab bar plus MeshCore status line: "← Title" left (tap = one level back; orange
// frame when the back action has the focus), unread total as a wine badge, battery right.
constexpr int BACK_ZONE_W = 80;
inline void statusLine(DisplayDriver *d, const char *title_utf8, bool back, unsigned unread, bool back_focus = false) {
  d->nativeFill(0, 0, theme::W, theme::NAV_H, theme::NAV_BG);
  const char *tabs[] = { "CHAT", "MELDUNG", "BRETT", "KARTE", "GERAET" };
  int selected =
      !strcmp(title_utf8, "Chat")      ? 0
      : !strcmp(title_utf8, "Meldung") ? 1
      : !strcmp(title_utf8, "Brett")   ? 2
      : !strcmp(title_utf8, "Karte")   ? 3
      : !strcmp(title_utf8, "Vorlage") || !strcmp(title_utf8, "Ort waehlen") ||
              !strcmp(title_utf8, "Ziel waehlen")
          ? 1
      : !strcmp(title_utf8, "Info") || !strcmp(title_utf8, "Standort") || !strncmp(title_utf8, "Ger", 3) ? 4
                                                                                                         : -1;
  for (int i = 0; i < 5; ++i) {
    d->nativeFill(2 + i * 64, 2, 60, 32, i == selected ? theme::MESH : theme::BUTTON2);
    centerText(d, 32 + i * 64, 14, i == selected ? theme::INK : theme::TEXT, tabs[i]);
  }
  d->nativeFill(0, theme::NAV_H - 1, theme::W, 1, theme::LINE);
  if (back) {
    button(d, 2, theme::TAB_H + 2, BACK_ZONE_W - 4, theme::STATUS_H - 4, "< Zurueck", back_focus);
    text(d, 88, theme::TAB_H + 10, theme::TEXT, title_utf8, 20);
  } else
    text(d, 8, theme::TAB_H + 10, theme::MESH, title_utf8, 25);
  if (unread) {
    char t[12]; snprintf(t, sizeof(t), "%u neu", unread > 99 ? 99 : unread);
    int bw = int(strlen(t)) * theme::CW + 4, bx = theme::W - theme::BATT_W - 4 - bw;
    d->nativeFill(bx, theme::TAB_H + 9, bw, 10, theme::WINE);
    d->nativeText(bx + 2, theme::TAB_H + 10, 1, theme::TEXT, t);
  }
  navBatteryField(d);
}
inline void hint(DisplayDriver *d, const char *utf8, uint16_t c = theme::MUTED) {
  d->nativeFill(0, theme::H - 9, theme::W, 9, theme::BG);
  text(d, 2, theme::H - 8, c, utf8);
}
}
