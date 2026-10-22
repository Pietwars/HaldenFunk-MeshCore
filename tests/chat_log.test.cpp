// Chat history of the T-Deck Plus standalone display (Issue #35): same record rules as decodeRecord() of the
// web UI, delivery marks, unread counters, sender split and positions in texts.
#include <assert.h>
#include <stdio.h>
#include "FakeApi.h"
#include "../examples/companion_radio/ui-brand/ChatLog.h"

int main() {
  FakeApi a; a.demo();
  chat::Log log; assert(log.begin());
  auto read = [&](size_t s, disaster::HistoryRecord &r) { return a.historyRecord(s, r); };
  assert(log.rebuild(a.seq, read) == 8 && !log.needsRebuild(a.seq));
  log.setBaseline(log.newest());
  const chat::Entry *own = log.last("d0a1b2c3d4e5f");
  assert(own && own->own && own->delivered && !strcmp(own->text, "Ja, 10 St\xc3\xbc" "ck w\xc3\xa4ren super."));
  unsigned posts = 0; log.forEach("dcafe0012ab34", true, [&](const chat::Entry &e) { ++posts; assert(e.room); });
  assert(posts == 2);
  unsigned direct = 0; log.forEach("dcafe0012ab34", false, [&](const chat::Entry &) { ++direct; });
  assert(direct == 0);                                             // room posts are not chat lines
  // unread: only incoming lines after the baseline
  a.channelOwn(0, "eigene", a.clock); a.channelIn(0, "Max: neu", a.clock); a.channelIn(0, "Max: neu 2", a.clock);
  log.rebuild(a.seq, read);
  assert(log.unread("c0") == 2 && log.incomingAfter(log.newest() - 3) == 2);
  log.markRead("c0"); assert(log.unread("c0") == 0);
  // the ring keeps 96 records; damaged slots are skipped
  for (int i = 0; i < 120; ++i) a.channelIn(1, "Lea: x", a.clock + i);
  a.hist[5].crc ^= 1;
  assert(log.rebuild(a.seq, read) == disaster::HISTORY_SLOTS - 1);
  for (size_t i = 1; i < log.size(); ++i) assert(log.at(i - 1).seq < log.at(i).seq);
  // decode: invalid channel slot, too short frames
  disaster::HistoryRecord r{}; r.seq = 1; uint8_t bad[] = {0xf3, 0, 9, 0, 0, 0, 0, 'x'};
  memcpy(r.data, bad, sizeof(bad)); r.len = sizeof(bad); chat::Entry e; assert(!chat::decode(r, e));
  uint8_t shortf[] = {16, 0, 0}; memcpy(r.data, shortf, 3); r.len = 3; assert(!chat::decode(r, e));
  // the longest possible channel text (frame limit) is kept whole
  std::vector<uint8_t> f = {17, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  for (int i = 0; i < 82; ++i) { f.push_back(0xc3); f.push_back(0xa4); }
  assert(f.size() <= disaster::FRAME_BYTES);
  memcpy(r.data, f.data(), f.size()); r.len = uint16_t(f.size());
  assert(chat::decode(r, e) && strlen(e.text) == 164 && uint8_t(e.text[163]) == 0xa4);
  // sender split and positions
  char who[33]; const char *body;
  assert(chat::splitSender("Lea: Hallo: Welt", who, sizeof(who), &body) && !strcmp(who, "Lea") && !strcmp(body, "Hallo: Welt"));
  assert(!chat::splitSender("ohne Absender", who, sizeof(who), &body) && !strcmp(body, "ohne Absender"));
  double la, lo;
  assert(chat::findPos("!DP1|50.5432,7.1130|1792", la, lo) && la > 50.54 && lo < 7.12);
  assert(chat::findPos("HILFE Keller 50.5299, 7.0408 bitte", la, lo));
  assert(chat::findPos("Berlin 52.520,13.405", la, lo));           // Germany: inside
  assert(!chat::findPos("kurz 52.52,13.40", la, lo));               // web: at least 3 decimals
  assert(!chat::findPos("Paris 48.85,2.35", la, lo));                // outside Germany
  assert(!chat::findPos("Version 1.18.0", la, lo));
  puts("chat_log: ok");
  return 0;
}
