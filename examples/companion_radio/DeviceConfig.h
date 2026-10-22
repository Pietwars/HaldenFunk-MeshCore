#pragma once
// Settings file of the T-Deck Plus standalone build on the SD card (Issue #35): /disaster/config.txt.
// Plain "key=value" lines, '#' comments. Only these keys exist:
//   name=<node name, 1..24 bytes>   gps_teilen=0|1   karte=<map key, e.g. mansfelder-land>   log=0|1
// No PINs, keys or passwords: lines whose key mentions one of them are rejected and never written.
// Pure logic, host tested (tests/device_config.test.cpp).
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

namespace devcfg {
constexpr size_t MAX_BYTES = 2048, NAME_BYTES = 24, MAP_KEY_BYTES = 31;   // name: CMD_SET_ADVERT_NAME limit
struct Config {
  bool hasName = false, hasShare = false, hasMap = false, hasLog = false;
  char name[NAME_BYTES + 1] = "";
  bool share = false, log = true;
  char map[MAP_KEY_BYTES + 1] = "";
  unsigned rejected = 0, unknown = 0, secrets = 0;   // ignored lines
};
inline bool secretKey(const char *k, size_t n) {
  static const char *const words[] = {"pin", "key", "schluessel", "schl\xc3\xbcssel", "passw", "secret", "token", "psk"};
  char low[40]; if (n >= sizeof(low)) n = sizeof(low) - 1;
  for (size_t i = 0; i < n; ++i) low[i] = char(tolower((unsigned char)k[i]));
  low[n] = 0;
  for (const char *w : words) if (strstr(low, w)) return true;
  return false;
}
inline bool flag(const char *v, size_t n, bool &out) {
  if (n == 1 && (v[0] == '0' || v[0] == '1')) { out = v[0] == '1'; return true; }
  if ((n == 2 && !strncmp(v, "an", 2)) || (n == 2 && !strncmp(v, "ja", 2))) { out = true; return true; }
  if ((n == 3 && !strncmp(v, "aus", 3)) || (n == 4 && !strncmp(v, "nein", 4))) { out = false; return true; }
  return false;
}
inline void trim(const char *&s, size_t &n) {
  while (n && (*s == ' ' || *s == '\t')) { ++s; --n; }
  while (n && (s[n - 1] == ' ' || s[n - 1] == '\t' || s[n - 1] == '\r')) --n;
}
// Parses `len` bytes. Returns false if the file is too large (nothing is applied then).
inline bool parse(const char *text, size_t len, Config &c) {
  c = Config();
  if (len > MAX_BYTES) return false;
  size_t pos = 0;
  while (pos < len) {
    size_t end = pos; while (end < len && text[end] != '\n') ++end;
    const char *l = text + pos; size_t n = end - pos; pos = end + 1;
    trim(l, n);
    if (!n || l[0] == '#') continue;
    const char *eq = (const char *)memchr(l, '=', n);
    if (!eq) { ++c.rejected; continue; }
    const char *k = l; size_t kn = size_t(eq - l); trim(k, kn);
    const char *v = eq + 1; size_t vn = size_t(l + n - v); trim(v, vn);
    if (secretKey(k, kn)) { ++c.secrets; continue; }
    if (kn == 4 && !strncmp(k, "name", 4)) {
      bool ok = vn >= 1 && vn <= NAME_BYTES;
      for (size_t i = 0; ok && i < vn; ++i) if ((unsigned char)v[i] < 32 || v[i] == 127) ok = false;
      if (!ok) { ++c.rejected; continue; }
      memcpy(c.name, v, vn); c.name[vn] = 0; c.hasName = true;
    } else if (kn == 10 && !strncmp(k, "gps_teilen", 10)) {
      if (flag(v, vn, c.share)) c.hasShare = true; else ++c.rejected;
    } else if (kn == 3 && !strncmp(k, "log", 3)) {
      if (flag(v, vn, c.log)) c.hasLog = true; else ++c.rejected;
    } else if (kn == 5 && !strncmp(k, "karte", 5)) {
      bool ok = vn >= 1 && vn <= MAP_KEY_BYTES;
      for (size_t i = 0; ok && i < vn; ++i) { unsigned char ch = (unsigned char)v[i]; if (!(islower(ch) || isdigit(ch) || ch == '-')) ok = false; }
      if (!ok) { ++c.rejected; continue; }
      memcpy(c.map, v, vn); c.map[vn] = 0; c.hasMap = true;
    } else ++c.unknown;
  }
  return true;
}
// Writes the file contents. Returns the length (0 if it does not fit).
inline size_t format(const Config &c, char *out, size_t cap) {
  int n = snprintf(out, cap,
    "# HaldenFunk T-Deck Plus Standalone - Einstellungen\n"
    "# Keine PINs, Schluessel oder Passwoerter: die Bluetooth-PIN bleibt im Geraet.\n"
    "name=%s\ngps_teilen=%d\nkarte=%s\nlog=%d\n", c.name, c.share ? 1 : 0, c.map, c.log ? 1 : 0);
  return n > 0 && size_t(n) < cap ? size_t(n) : 0;
}
}
