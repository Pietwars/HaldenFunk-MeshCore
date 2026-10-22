#pragma once
// HaldenFunk design system for the T-Deck Plus standalone display (Issue #35): colours, wording and
// structure of the web UI (web/index.html), adapted to 320 x 240, keyboard and trackball.
// Text helpers: UTF-8 -> CP437 (GFX built-in font, 6 x 8 px per character at size 1) and word wrapping.
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

namespace theme {
constexpr uint16_t rgb(uint32_t c) {
  return uint16_t((((c >> 16) & 0xFF) >> 3) << 11 | (((c >> 8) & 0xFF) >> 2) << 5 | ((c & 0xFF) >> 3));
}
// :root of web/index.html
constexpr uint16_t BG = rgb(0x24282b), TEXT = rgb(0xf6f2eb), MUTED = rgb(0xb2ada6), LINE = rgb(0x65564b);
constexpr uint16_t MESH = rgb(0xb87333), SAGE = rgb(0xd59a62), WINE = rgb(0x985f45), CARD = rgb(0x353a3e);
constexpr uint16_t SELF = rgb(0x41352d), BUTTON2 = rgb(0x353a3e), WARN_BG = rgb(0x493126), PEND = rgb(0xf0b64d);
constexpr uint16_t BAD = rgb(0xe98b82), INFO = rgb(0x7eb6c2), BOARD = rgb(0xd58a47), POST = rgb(0x4b555a);
constexpr uint16_t PIN_BG = rgb(0x3a2f26), MAP_BG = rgb(0x20272b), PHARMACY = rgb(0xe98b82), WATER = rgb(0x58bfd0);
constexpr uint16_t NAV_BG = rgb(0x353a3e), INK = rgb(0x20170f);   // dark text on mesh-green buttons (#102)
constexpr int W = 320, H = 240, CW = 6, CH = 8;                    // display and character cell (size 1)

// HaldenFunk standalone: five touch tabs, plus MeshCore back/unread/battery strip.
constexpr int TAB_H = 36, STATUS_H = 28, NAV_H = TAB_H + STATUS_H;
constexpr int BATT_W = 52;

// UTF-8 -> CP437 for the GFX font. German umlauts and sharp s map to their CP437 glyphs, other non-ASCII
// characters become '?'. Never splits a multi-byte character. Returns output length.
inline size_t toCp437(const char *in, char *out, size_t cap, size_t max_chars = (size_t)-1) {
  size_t n = 0;
  if (!cap) return 0;
  for (size_t i = 0; in && in[i] && n + 1 < cap && n < max_chars; ++i) {
    uint8_t c = uint8_t(in[i]);
    if (c < 0x80) { out[n++] = c >= 32 || (c >= 0x18 && c <= 0x1B) ? char(c) : ' '; continue; }   // 0x18-0x1B: arrows
    if ((c & 0xE0) == 0xC0 && in[i + 1]) {
      uint8_t d = uint8_t(in[++i]); char m = '?';
      if (c == 0xC3) switch (d) {
        case 0xA4: m = char(0x84); break; case 0xB6: m = char(0x94); break; case 0xBC: m = char(0x81); break;
        case 0x84: m = char(0x8E); break; case 0x96: m = char(0x99); break; case 0x9C: m = char(0x9A); break;
        case 0x9F: m = char(0xE1); break; case 0xA9: m = char(0x82); break; case 0xA8: m = char(0x8A); break;
        default: break;
      } else if (c == 0xC2 && d == 0xB0) m = char(0xF8);    // degree sign
      else if (c == 0xC2 && d == 0xB7) m = char(0xFA);      // middle dot
      out[n++] = m; continue;
    }
    // 3/4-byte sequences: dashes, quotes, ellipsis, arrows, square; everything else (emoji) one '?'.
    if ((c & 0xC0) != 0x80) {
      uint8_t b1 = c == 0xE2 && in[i + 1] ? uint8_t(in[i + 1]) : 0, b2 = b1 && in[i + 2] ? uint8_t(in[i + 2]) : 0;
      if (b1 == 0x80 && b2 == 0xA6) {                        // ellipsis -> "..."
        for (int k = 0; k < 3 && n + 1 < cap && n < max_chars; ++k) out[n++] = '.';
      } else {
        char m = '?';
        if (b1 == 0x80 && (b2 == 0x93 || b2 == 0x94)) m = '-';
        else if (b1 == 0x80 && (b2 == 0x9C || b2 == 0x9D || b2 == 0x9E)) m = '"';
        else if (b1 == 0x80 && (b2 == 0x98 || b2 == 0x99 || b2 == 0x9A)) m = '\'';
        else if (b1 == 0x86 && b2 >= 0x90 && b2 <= 0x93) { static const char arrow[4] = {0x1B, 0x18, 0x1A, 0x19}; m = arrow[b2 - 0x90]; }
        else if (b1 == 0x96 && b2 == 0xA0) m = char(0xFE);
        out[n++] = m;
      }
    }
    while (in[i + 1] && (uint8_t(in[i + 1]) & 0xC0) == 0x80) ++i;
  }
  out[n] = 0;
  return n;
}
// Word wrap of a CP437 string into lines of at most `cols` characters. Calls fn(line, len) per line and
// returns the number of lines (fn may be null to count only). Long words are cut hard.
template <class Fn> int wrap(const char *s, int cols, Fn fn, int max_lines = 1000) {
  int lines = 0; size_t len = strlen(s), pos = 0;
  if (cols < 1) return 0;
  while (pos < len && lines < max_lines) {
    while (pos < len && s[pos] == ' ') ++pos;
    if (pos >= len) break;
    size_t take = len - pos;
    if (take > size_t(cols)) {
      take = cols;
      size_t cut = take;
      while (cut > 0 && s[pos + cut] != ' ' && s[pos + cut - 1] != ' ') --cut;
      if (cut > size_t(cols) / 3) take = cut;
    }
    size_t shown = take;
    while (shown && s[pos + shown - 1] == ' ') --shown;
    fn(s + pos, shown);
    ++lines; pos += take;
  }
  return lines;
}
inline int countLines(const char *s, int cols) { return wrap(s, cols, [](const char *, size_t) {}); }
// Calendar date from days since 1970-01-01 (Howard Hinnant's civil_from_days).
inline void civil(int64_t days, int &y, unsigned &m, unsigned &d) {
  int64_t z = days + 719468, era = z / 146097; unsigned doe = unsigned(z - era * 146097);
  unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365, doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  unsigned mp = (5 * doy + 2) / 153; d = doy - (153 * mp + 2) / 5 + 1; m = mp < 10 ? mp + 3 : mp - 9;
  y = int(yoe + era * 400 + (m <= 2));
}
// Germany: CET/CEST (EU rule: last Sunday of March / October, 01:00 UTC).
inline int berlinOffsetHours(uint32_t t) {
  int y; unsigned m, d; civil(int64_t(t) / 86400, y, m, d);
  auto lastSunday = [](int year, unsigned month) -> int64_t {   // days since epoch of the last Sunday
    int64_t yy = year - (month <= 2), era = yy / 400; unsigned yoe = unsigned(yy - era * 400);
    unsigned mp = month > 2 ? month - 3 : month + 9, doy = (153 * mp + 2) / 5 + 30;  // day 31
    int64_t days = era * 146097 + yoe * 365 + yoe / 4 - yoe / 100 + doy - 719468;
    int64_t wd = (days + 4) % 7;                                  // 1970-01-01 was a Thursday
    return days - wd;
  };
  int64_t start = lastSunday(y, 3) * 86400 + 3600, end = lastSunday(y, 10) * 86400 + 3600;
  return int64_t(t) >= start && int64_t(t) < end ? 2 : 1;
}
// Format a unix time as "TT.MM. hh:mm" in German local time.
inline void stampText(uint32_t t, char *out, size_t cap) {
  if (t < 1700000000u) { if (cap) snprintf(out, cap, "--.--. --:--"); return; }
  int64_t s = int64_t(t) + berlinOffsetHours(t) * 3600;
  int y; unsigned m, d; civil(s / 86400, y, m, d); int secs = int(s % 86400);
  snprintf(out, cap, "%02u.%02u. %02d:%02d", d, m, secs / 3600, (secs / 60) % 60);
}
}
