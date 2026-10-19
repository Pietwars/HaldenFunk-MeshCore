// SPDX-License-Identifier: MIT
#pragma once
// Offline-Warteschlange mit zwei Lesern (0.9.3): Weboberflaeche (Portal) und MeshCore-App (Bluetooth).
// Vorher holte das Portal jede Nachricht per CMD_SYNC_NEXT_MESSAGE ab und entfernte sie damit fuer alle;
// eine spaeter verbundene App bekam sie nicht mehr. Jetzt merkt sich jeder Eintrag, wer ihn schon hat,
// und wird erst entfernt, wenn beide Leser ihn abgeholt haben.
#include <stdint.h>
#include <string.h>

namespace disaster {

constexpr uint8_t READER_PORTAL = 1, READER_APP = 2, READERS_ALL = READER_PORTAL | READER_APP;

// F: { uint8_t len; uint8_t buf[]; uint8_t seen; bool isChannelMsg() const; }
template <typename F> int sqFind(const F *q, int n, uint8_t who) {
  for (int i = 0; i < n; i++) if (!(q[i].seen & who)) return i;
  return -1;
}
template <typename F> int sqPending(const F *q, int n, uint8_t who) {
  int c = 0;
  for (int i = 0; i < n; i++) if (!(q[i].seen & who)) c++;
  return c;
}
template <typename F> void sqRemove(F *q, int &n, int pos) {
  for (int i = pos; i < n - 1; i++) q[i] = q[i + 1];
  n--;
}
// Naechsten Eintrag fuer `who` holen (0 = keiner). Entfernt ihn, sobald beide Leser ihn haben.
template <typename F> int sqTake(F *q, int &n, uint8_t who, uint8_t *out) {
  int i = sqFind(q, n, who);
  if (i < 0) return 0;
  int len = q[i].len;
  memcpy(out, q[i].buf, len);
  q[i].seen |= who;
  if ((q[i].seen & READERS_ALL) == READERS_ALL) sqRemove(q, n, i);
  return len;
}
// Volle Warteschlange: Platz schaffen. Reihenfolge: aelteste Kanalnachricht, die das Portal schon hat;
// (nur mit Portal) aelteste beliebige Nachricht, die das Portal schon hat; aelteste Kanalnachricht
// (MeshCore-Standard). Liefert false, wenn nichts entfernt werden darf (neue Nachricht wird verworfen).
template <typename F> bool sqMakeRoom(F *q, int &n, bool portal) {
  for (int i = 0; i < n; i++) if ((q[i].seen & READER_PORTAL) && q[i].isChannelMsg()) { sqRemove(q, n, i); return true; }
  if (portal) for (int i = 0; i < n; i++) if (q[i].seen & READER_PORTAL) { sqRemove(q, n, i); return true; }
  for (int i = 0; i < n; i++) if (q[i].isChannelMsg()) { sqRemove(q, n, i); return true; }
  return false;
}
template <typename F> bool sqAdd(F *q, int &n, int cap, const uint8_t *frame, int len, uint8_t seen, bool portal) {
  if (len <= 0 || len > (int)sizeof(q[0].buf)) return false;
  if (n >= cap && !sqMakeRoom(q, n, portal)) return false;
  q[n].len = (uint8_t)len;
  memcpy(q[n].buf, frame, len);
  q[n].seen = seen;
  n++;
  return true;
}

} // namespace disaster
