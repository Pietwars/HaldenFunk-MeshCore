// SPDX-License-Identifier: MIT
#pragma once
// Web-free building blocks for local sending (Issue #35): shared by the web portal (DisasterPortal.h) and the
// T-Deck Plus standalone display (StandaloneFrontend.cpp) through LocalCompanion.h. No Arduino dependency,
// so host tests can check the rules directly.
#include <stdint.h>
#include <stddef.h>
#include <string.h>

namespace disaster {
constexpr size_t LOCAL_TEXT_LIMIT = 100;       // UTF-8 bytes, same limit as /api/send
constexpr uint32_t SEND_PAUSE_MS = 5000;       // pause between two local sends (web and device)
constexpr uint32_t STAMP_MIN = 1700000000u, STAMP_MAX = 4102444800u;

inline bool validStamp(uint32_t stamp) { return stamp >= STAMP_MIN && stamp <= STAMP_MAX; }
// Companion timestamps of local sends must increase strictly (same rule as the web sender).
inline uint32_t nextStamp(uint32_t stamp, uint32_t last) { return stamp <= last ? last + 1 : stamp; }
inline bool sendPaused(bool sentOnce, uint32_t lastSend, uint32_t now) {
  return sentOnce && uint32_t(now - lastSend) < SEND_PAUSE_MS;
}
inline uint32_t pauseLeftMs(bool sentOnce, uint32_t lastSend, uint32_t now) {
  return sendPaused(sentOnce, lastSend, now) ? SEND_PAUSE_MS - uint32_t(now - lastSend) : 0;
}
inline int hexNibble(char c) {
  return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
}
// Local targets: "c0".."c7" = channel slot, "d" + 12 hex digits = contact (first 6 key bytes).
inline bool channelTarget(const char *t) { return t && strlen(t) == 2 && t[0] == 'c' && t[1] >= '0' && t[1] <= '7'; }
inline bool contactTarget(const char *t) {
  if (!t || strlen(t) != 13 || t[0] != 'd') return false;
  for (int i = 1; i < 13; ++i) if (hexNibble(t[i]) < 0) return false;
  return true;
}
// Restricted plaintext Companion frame builder (CMD_SEND_TXT_MSG 2 / CMD_SEND_CHANNEL_TXT_MSG 3):
// no arbitrary command or CLI access. Returns 0 if target, clock, text or capacity are invalid.
inline size_t textFrame(const char *target, const char *text, uint32_t stamp, uint8_t *out, size_t capacity) {
  if (!target || !text) return 0;
  size_t n = strlen(text), header = 0;
  if (!n || n > LOCAL_TEXT_LIMIT || !validStamp(stamp)) return 0;
  if (channelTarget(target)) header = 7;
  else if (contactTarget(target)) header = 13;
  if (!header || header + n > capacity) return 0;
  memset(out, 0, header);
  out[0] = header == 7 ? 3 : 2;
  memcpy(out + 3, &stamp, 4);
  if (header == 7) out[2] = uint8_t(target[1] - '0');
  else for (int i = 0; i < 6; ++i) out[7 + i] = uint8_t(hexNibble(target[1 + 2 * i]) << 4 | hexNibble(target[2 + 2 * i]));
  memcpy(out + header, text, n);
  return header + n;
}
}
