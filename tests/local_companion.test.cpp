// Host test (Issue #35, step 2): web-free local companion client shared by web portal and T-Deck standalone.
// Startup sequence, local sending with 5 s pause and strictly increasing stamps, history records of own sends,
// room-server login with the guest password, app sends in the history, timeouts.
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../examples/companion_radio/LocalCompanion.h"
uint32_t fake_millis = 1000;
FakeSerial Serial;
FakeFs SPIFFS;

static size_t contactFrame(uint8_t *p, uint8_t first, uint8_t type, const char *name, int32_t lat = 0, int32_t lon = 0) {
  memset(p, 0, 148); p[0] = 3;
  for (int i = 0; i < 32; ++i) p[1 + i] = uint8_t(first + i);
  p[33] = type; memcpy(p + 100, name, strlen(name));
  memcpy(p + 136, &lat, 4); memcpy(p + 140, &lon, 4);
  return 148;
}
static void startup(LocalCompanion &c) {
  uint8_t f[MAX_FRAME_SIZE], ok[1] = {0};
  for (int step = 0; step < 12; ++step) {
    size_t n = c.checkRecvFrame(f); assert(n);
    if (f[0] == 31) {                              // channel query: slot 0 "Oeffentlich", others empty
      uint8_t ch[50] = {18, f[1]};
      if (f[1] == 0) memcpy(ch + 2, "Oeffentlich", 11);
      if (f[1] == 0) c.writeFrame(ch, 50); else { uint8_t err[2] = {1, 0}; c.writeFrame(err, 2); }
    } else if (f[0] == 4) {                        // contacts: chat, room, repeater, then end
      uint8_t p[148];
      c.writeFrame(p, contactFrame(p, 0x10, 1, "Anna", 50551500, 7186500));
      c.writeFrame(p, contactFrame(p, 0x20, 3, "Brett Eisleben", 51527900, 11548000));
      c.writeFrame(p, contactFrame(p, 0x30, 2, "Repeater"));
      uint8_t end[5] = {4}; c.writeFrame(end, 5);
    } else c.writeFrame(ok, 1);
  }
}
static unsigned countKind(const LocalCompanion &c, uint8_t kind, disaster::HistoryRecord *last = nullptr) {
  unsigned n = 0; uint32_t best = 0;
  for (size_t i = 0; i < disaster::HISTORY_SLOTS; ++i) {
    disaster::HistoryRecord r;
    if (c.readHistory(i, r) && r.data[0] == kind) { ++n; if (last && r.seq > best) { best = r.seq; *last = r; } }
  }
  return n;
}

struct TestCompanion : LocalCompanion { using LocalCompanion::serviceRoom; using LocalCompanion::beginHistory; };

int main() {
  TestCompanion c; char st[96]; uint8_t f[MAX_FRAME_SIZE]; uint32_t stamp = 1792400000;
  assert(!c.localSend("c0", "x", stamp, st, sizeof(st)) && strstr(st, "beschaeftigt"));
  c.enable(); startup(c); assert(c.ready());
  assert(c.channelHex(0).length() == 68 && c.channelHex(1).length() == 0);
  assert(c.contactHex(0).substring(66, 68) == "01" && c.contactHex(1).substring(66, 68) == "03");
  // Channel send: frame 3 to slot 0, history f3 after the core's OK, device status updated.
  assert(c.localSend("c0", "Hallo", stamp, st, sizeof(st)) && strstr(st, "Vorgemerkt"));
  size_t n = c.checkRecvFrame(f);
  assert(n == 12 && f[0] == 3 && f[2] == 0 && !memcmp(f + 7, "Hallo", 5));
  uint32_t sent; memcpy(&sent, f + 3, 4); assert(sent == stamp);
  uint8_t ok[1] = {0}; c.writeFrame(ok, 1);
  disaster::HistoryRecord r;
  assert(countKind(c, 0xf3, &r) == 1 && r.len == 12 && !memcmp(r.data + 7, "Hallo", 5));
  assert(strstr(c.localStatus(), "Gesendet"));
  // 5 s pause shared with the web sender; stamps increase strictly.
  assert(!c.localSend("c0", "Zwei", stamp, st, sizeof(st)) && strstr(st, "Sendepause"));
  assert(c.pauseLeftMs() == 5000);
  fake_millis += 4999; assert(!c.localSend("c0", "Zwei", stamp, st, sizeof(st)));
  fake_millis += 1;
  assert(c.localSend("c0", "Zwei", stamp, st, sizeof(st)));
  n = c.checkRecvFrame(f); memcpy(&sent, f + 3, 4); assert(sent == stamp + 1);
  uint8_t err[2] = {1, 0}; c.writeFrame(err, 2);
  assert(countKind(c, 0xf3) == 1 && strstr(c.localStatus(), "Sendefehler"));
  // Direct message to a chat contact: f4 record keeps the ACK tag; 0x82 confirmation is kept as well.
  fake_millis += 5000;
  assert(c.localSend("d101112131415", "Hi Anna", stamp, st, sizeof(st)));
  n = c.checkRecvFrame(f); assert(f[0] == 2 && f[7] == 0x10 && f[12] == 0x15 && n == 13 + 7);
  uint8_t sentResp[10] = {6, 0, 0xaa, 0xbb, 0xcc, 0xdd, 0, 0, 0, 0}; c.writeFrame(sentResp, 10);
  assert(countKind(c, 0xf4, &r) == 1 && r.data[13] == 0xaa && r.data[16] == 0xdd && !memcmp(r.data + 17, "Hi Anna", 7));
  uint8_t conf[9] = {0x82, 0xaa, 0xbb, 0xcc, 0xdd, 1, 0, 0, 0}; c.writeFrame(conf, 9);
  assert(countKind(c, 0x82) == 1);
  // Targets: unknown slot, unknown contact, repeater, invalid ids, clock, size.
  fake_millis += 5000;
  assert(!c.localSend("c3", "x", stamp, st, sizeof(st)) && strstr(st, "nicht eingerichtet"));
  assert(!c.localSend("d999999999999", "x", stamp, st, sizeof(st)) && strstr(st, "mehrdeutig"));
  assert(!c.localSend("d303132333435", "x", stamp, st, sizeof(st)) && strstr(st, "Kein Chat-Kontakt"));
  assert(!c.localSend("x1", "x", stamp, st, sizeof(st)) && strstr(st, "Ungueltiges"));
  assert(!c.localSend("c0", "x", 12345, st, sizeof(st)) && strstr(st, "Uhr fehlt"));
  char big[102]; memset(big, 'x', 101); big[101] = 0;
  assert(!c.localSend("c0", big, stamp, st, sizeof(st)) && strstr(st, "100"));
  assert(!c.localSend("c0", "", stamp, st, sizeof(st)));
  // Brett: writing needs a login with write rights; login = clock frame, then guest-password frame.
  assert(!c.localSend("d202122232425", "Aushang", stamp, st, sizeof(st)) && strstr(st, "Brett"));
  assert(!c.localRoomLogin("d101112131415", stamp, st, sizeof(st)));      // chat contact is no room
  assert(c.localRoomLogin("d202122232425", stamp, st, sizeof(st)) && c.roomPendingNow());
  n = c.checkRecvFrame(f); assert(n == 5 && f[0] == 6); c.writeFrame(ok, 1);
  n = c.checkRecvFrame(f); assert(n == 38 && f[0] == 26 && f[1] == 0x20 && !memcmp(f + 33, "hello", 5));
  c.writeFrame(ok, 1);
  uint8_t login[13] = {0x85, 0, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0, 0, 0, 0, 2};
  c.writeFrame(login, 13);
  assert(!c.roomPendingNow() && c.roomRightsNow() == 2 && c.roomTargetId() == "d202122232425");
  assert(c.localSend("d202122232425", "Aushang", stamp, st, sizeof(st)));
  n = c.checkRecvFrame(f); assert(f[0] == 2 && f[7] == 0x20); c.writeFrame(sentResp, 10);
  // Room login without answer expires after 45 s (read-only state is never assumed).
  fake_millis += 5000;
  assert(c.localRoomLogin("d202122232425", stamp, st, sizeof(st)));
  c.checkRecvFrame(f); c.writeFrame(ok, 1); c.checkRecvFrame(f); c.writeFrame(ok, 1);
  fake_millis += 45001; c.writeFrame(ok, 0);
  assert(c.roomPendingNow()); // serviced by the frontend loop, see below
  c.serviceRoom(); assert(!c.roomPendingNow() && c.roomRightsNow() == 0);
  // Incoming channel message and app sends are stored like in the web build.
  uint8_t in[20] = {17, 0, 0, 0, 0, 0xff, 0}; memcpy(in + 7, &stamp, 4); memcpy(in + 11, "Anna: Hi", 8);
  c.writeFrame(in, 19); assert(countKind(c, 17) == 1);
  uint8_t app[12] = {3, 0, 0}; memcpy(app + 3, &stamp, 4); memcpy(app + 7, "Hallo", 5);
  c.recordAppSend(app, 12, nullptr); assert(countKind(c, 0xf3) == 2);
  // An unanswered send is never repeated automatically.
  fake_millis += 5000;
  assert(c.localSend("c0", "Spaet", stamp, st, sizeof(st)));
  c.checkRecvFrame(f); fake_millis += 15001; assert(c.checkRecvFrame(f) == 1 && f[0] == 10);
  assert(strstr(c.localStatus(), "Antwort unklar"));
  // History survives a restart: sequence numbers continue.
  uint32_t seq = c.historySeq();
  TestCompanion again; again.beginHistory(); assert(again.historySeq() == seq);
  puts("local_companion.test.cpp: startup, local send pause/stamps/history, Brett login, app sends passed");
}
