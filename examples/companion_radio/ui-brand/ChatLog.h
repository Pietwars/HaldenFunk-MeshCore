#pragma once
// Chat history for the T-Deck Plus standalone display (Issue #35). Decodes the same history records as the web
// UI (decodeRecord in web/index.html): received channel/direct/room messages (17/16, older 8/7), own sends
// (f3 channel, f2/f4 direct with ACK tag, also sends from the MeshCore app) and delivery confirmations (0x82).
// Keeps unread counters per conversation (incoming only, since start of the device).
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../HistoryRecord.h"

namespace chat {
constexpr size_t TEXT_BYTES = 168;
struct Entry {
  uint32_t seq = 0, stamp = 0;
  char conv[14] = "";          // "c<slot>" or "d<12 hex>" like the web UI
  bool own = false, room = false, delivered = false;
  uint8_t tag[4] = {0, 0, 0, 0};
  char author[9] = "";         // room posts: first 4 bytes of the author key as hex
  char text[TEXT_BYTES + 1] = "";
};
inline uint32_t u32(const uint8_t *p) { uint32_t v; memcpy(&v, p, 4); return v; }
inline void hex(const uint8_t *p, size_t n, char *out) {
  static const char d[] = "0123456789abcdef";
  for (size_t i = 0; i < n; ++i) { out[2 * i] = d[p[i] >> 4]; out[2 * i + 1] = d[p[i] & 15]; }
  out[2 * n] = 0;
}
inline void copyText(const uint8_t *p, size_t n, char *out) {
  size_t k = 0;
  while (k < n && k < TEXT_BYTES && p[k]) ++k;
  // cut at the limit: never end inside a UTF-8 sequence
  if (k == TEXT_BYTES && k < n && p[k]) while (k && (p[k] & 0xC0) == 0x80) --k;
  memcpy(out, p, k); out[k] = 0;
}
// Same rules as decodeRecord() of the web UI. Returns false for records that are no chat line.
inline bool decode(const disaster::HistoryRecord &r, Entry &e) {
  const uint8_t *p = r.data; size_t n = r.len; size_t at = 0;
  e = Entry(); e.seq = r.seq;
  if (!n) return false;
  uint8_t c = p[0];
  if (c == 17 && n >= 11) { at = 11; e.stamp = u32(p + 7); snprintf(e.conv, sizeof(e.conv), "c%u", p[4]); }
  else if (c == 16 && n >= 16) {
    at = 16; e.stamp = u32(p + 12); e.conv[0] = 'd'; hex(p + 4, 6, e.conv + 1);
    if (p[11] == 2) { if (n < 20) return false; at = 20; e.room = true; hex(p + 16, 4, e.author); }
  }
  else if (c == 8 && n >= 8) { at = 8; e.stamp = u32(p + 4); snprintf(e.conv, sizeof(e.conv), "c%u", p[1]); }
  else if (c == 7 && n >= 13) { at = 13; e.stamp = u32(p + 9); e.conv[0] = 'd'; hex(p + 1, 6, e.conv + 1); if (p[8] == 2) return false; }
  else if (c == 0xf3 && n >= 7) { at = 7; e.stamp = u32(p + 3); snprintf(e.conv, sizeof(e.conv), "c%u", p[2]); e.own = true; }
  else if ((c == 0xf2 && n >= 13) || (c == 0xf4 && n >= 17)) {
    at = c == 0xf4 ? 17 : 13; e.stamp = u32(p + 3); e.conv[0] = 'd'; hex(p + 7, 6, e.conv + 1); e.own = true;
    if (c == 0xf4) memcpy(e.tag, p + 13, 4);
  }
  else return false;
  if (e.conv[0] == 'c' && (e.conv[1] < '0' || e.conv[1] > '7' || e.conv[2])) return false;
  copyText(p + at, n - at, e.text);
  return true;
}
// Channel lines carry "Name: text" (web: /^(.{1,32}?): /). Splits sender and body for display.
inline bool splitSender(const char *text, char *name, size_t cap, const char **body) {
  const char *k = strstr(text, ": ");
  if (!k || k == text || size_t(k - text) > 32) { *body = text; if (cap) name[0] = 0; return false; }
  size_t n = size_t(k - text); if (n >= cap) n = cap - 1;
  memcpy(name, text, n); name[n] = 0; *body = k + 2; return true;
}
// First "lat,lon" pair in a text (web notePos: -?\d{1,2}\.\d{3,} , -?\d{1,3}\.\d{3,}), limited to Germany.
inline bool findPos(const char *t, double &lat, double &lon) {
  for (const char *s = t; *s; ++s) {
    const char *a = s; if (*a == '-') ++a;
    if (!(*a >= '0' && *a <= '9')) continue;
    if (s > t && ((s[-1] >= '0' && s[-1] <= '9') || s[-1] == '.')) continue;
    const char *q = a; int d1 = 0; while (*q >= '0' && *q <= '9') { ++q; ++d1; }
    if (d1 < 1 || d1 > 2 || *q != '.') continue;
    ++q; int f1 = 0; while (*q >= '0' && *q <= '9') { ++q; ++f1; }
    if (f1 < 3) continue;
    const char *m = q; while (*m == ' ') ++m;
    if (*m != ',') continue;
    ++m; while (*m == ' ') ++m;
    const char *b = m; if (*b == '-') ++b;
    const char *w = b; int d2 = 0; while (*w >= '0' && *w <= '9') { ++w; ++d2; }
    if (d2 < 1 || d2 > 3 || *w != '.') continue;
    ++w; int f2 = 0; while (*w >= '0' && *w <= '9') { ++w; ++f2; }
    if (f2 < 3) continue;
    double la = strtod(s, nullptr), lo = strtod(m, nullptr);
    if (la < 47 || la > 56 || lo < 5 || lo > 16) continue;
    lat = la; lon = lo; return true;
  }
  return false;
}

// All decodable records in sequence order (HISTORY_SLOTS ring), plus unread bookkeeping.
class Log {
public:
  static constexpr size_t CAP = disaster::HISTORY_SLOTS;
  static constexpr size_t SEEN = 80;
private:
  Entry *items = nullptr; size_t count = 0;
  struct Seen { char conv[14]; uint32_t seq; } seen[SEEN];
  size_t seenCount = 0; uint32_t baseline = 0, builtSeq = 0xFFFFFFFFu;
  uint32_t seenSeq(const char *conv) const {
    for (size_t i = 0; i < seenCount; ++i) if (!strcmp(seen[i].conv, conv)) return seen[i].seq;
    return baseline;
  }
public:
  Log() {}
  ~Log() { delete[] items; }
  Log(const Log &) = delete; Log &operator=(const Log &) = delete;
  bool begin() { if (!items) items = new Entry[CAP]; return items != nullptr; }
  // Everything that exists at start counts as read; only later incoming lines are unread.
  void setBaseline(uint32_t seq) { baseline = seq; seenCount = 0; }
  bool needsRebuild(uint32_t seq) const { return seq != builtSeq; }
  template <class Reader> size_t rebuild(uint32_t seq, Reader read) {   // read(slot, record) -> bool
    count = 0; builtSeq = seq;
    if (!items) return 0;
    uint32_t ack[CAP]; size_t acks = 0;
    for (size_t slot = 0; slot < CAP; ++slot) {
      disaster::HistoryRecord r;
      if (!read(slot, r)) continue;
      if (r.data[0] == 0x82 && r.len >= 5) {         // delivery confirmation for an f4 record
        if (acks < CAP) ack[acks++] = u32(r.data + 1);
        continue;
      }
      Entry e; if (!decode(r, e)) continue;
      size_t i = count++;
      while (i && items[i - 1].seq > e.seq) { items[i] = items[i - 1]; --i; }
      items[i] = e;
    }
    for (size_t a = 0; a < acks; ++a)
      for (size_t i = 0; i < count; ++i) if (items[i].own && ack[a] && u32(items[i].tag) == ack[a]) items[i].delivered = true;
    return count;
  }
  size_t size() const { return count; }
  const Entry &at(size_t i) const { return items[i]; }
  uint32_t newest() const { return count ? items[count - 1].seq : 0; }
  // Entries of one conversation; room=true: Brett posts (room posts and own sends to that room).
  template <class Fn> void forEach(const char *conv, bool room, Fn fn) const {
    for (size_t i = 0; i < count; ++i) {
      const Entry &e = items[i];
      if (strcmp(e.conv, conv)) continue;
      if (room ? !(e.room || e.own) : e.room) continue;
      fn(e);
    }
  }
  unsigned unread(const char *conv, bool room = false) const {
    uint32_t s = seenSeq(conv); unsigned n = 0;
    forEach(conv, room, [&](const Entry &e) { if (!e.own && e.seq > s) ++n; });
    return n;
  }
  const Entry *last(const char *conv, bool room = false) const {
    const Entry *l = nullptr; forEach(conv, room, [&](const Entry &e) { l = &e; }); return l;
  }
  void markRead(const char *conv) {
    uint32_t s = newest() > baseline ? newest() : baseline;
    for (size_t i = 0; i < seenCount; ++i) if (!strcmp(seen[i].conv, conv)) { seen[i].seq = s; return; }
    if (seenCount < SEEN) { snprintf(seen[seenCount].conv, sizeof(seen[seenCount].conv), "%s", conv); seen[seenCount++].seq = s; }
  }
  // Incoming lines after `since` (for the wake-up / unread indicator).
  unsigned incomingAfter(uint32_t since) const {
    unsigned n = 0; for (size_t i = 0; i < count; ++i) if (!items[i].own && items[i].seq > since) ++n; return n;
  }
};
}
