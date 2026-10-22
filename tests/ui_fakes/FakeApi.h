#pragma once
// Demo device for host tests and the preview harness of the T-Deck standalone pages (Issue #35).
// Data is invented (no real keys, no PINs).
#include "../../examples/companion_radio/ui-brand/DeviceApi.h"
#include <vector>
#include <string>
#include <stdio.h>

struct FakeApi : dev::Api {
  std::vector<dev::Target> targets;
  std::vector<disaster::HistoryRecord> hist = std::vector<disaster::HistoryRecord>(disaster::HISTORY_SLOTS);
  uint32_t seq = 0, clock = 1792922400u;   // 2026-10-26 10:00 local
  bool isReady = true, sendOk = true, loginOk = true;
  dev::Gps g; dev::Room rm{"", 0, false};
  int map = 0, sends = 0, logins = 0, adverts = 0, refreshes = 0, saves = 0;
  const char *firmwareLabel = "v1.18.0-dar0.9.5-standalone-test";
  std::string lastId, lastText; char st[96] = "Gesendet; Zustellung offen";
  FakeApi() { g.detected = true; g.receiverOn = true; g.sats = 7; }
  void add(const char *id, const char *name, dev::Kind k, double lat = 0, double lon = 0) {
    dev::Target t{}; snprintf(t.id, sizeof(t.id), "%s", id); snprintf(t.name, sizeof(t.name), "%s", name);
    t.kind = k; t.hasPos = lat != 0; t.lat = lat; t.lon = lon; targets.push_back(t);
  }
  void record(const std::vector<uint8_t> &frame) {
    disaster::HistoryRecord r{}; r.seq = ++seq; r.len = uint16_t(frame.size());
    memcpy(r.data, frame.data(), frame.size()); r.crc = disaster::checksum(r);
    hist[(r.seq - 1) % disaster::HISTORY_SLOTS] = r;
  }
  static void put32(std::vector<uint8_t> &f, uint32_t v) { for (int i = 0; i < 4; ++i) f.push_back(uint8_t(v >> (8 * i))); }
  static void hex6(std::vector<uint8_t> &f, const char *id) { for (int i = 0; i < 6; ++i) { unsigned b; sscanf(id + 1 + 2 * i, "%2x", &b); f.push_back(uint8_t(b)); } }
  void channelIn(int ch, const char *text, uint32_t t) {     // frame 17 (v3 channel message)
    std::vector<uint8_t> f = {17, 0, 0, 0, uint8_t(ch), 0, 0}; put32(f, t);
    f.insert(f.end(), text, text + strlen(text)); record(f);
  }
  void directIn(const char *id, const char *text, uint32_t t) {   // frame 16
    std::vector<uint8_t> f = {16, 0, 0, 0}; hex6(f, id); f.push_back(0); f.push_back(0); put32(f, t);
    f.insert(f.end(), text, text + strlen(text)); record(f);
  }
  void roomIn(const char *room, const char *author8, const char *text, uint32_t t) {   // frame 16, txt_type 2
    std::vector<uint8_t> f = {16, 0, 0, 0}; hex6(f, room); f.push_back(0); f.push_back(2); put32(f, t);
    for (int i = 0; i < 4; ++i) { unsigned b; sscanf(author8 + 2 * i, "%2x", &b); f.push_back(uint8_t(b)); }
    f.insert(f.end(), text, text + strlen(text)); record(f);
  }
  void channelOwn(int ch, const char *text, uint32_t t) {
    std::vector<uint8_t> f = {0xf3, 0, uint8_t(ch)}; put32(f, t); f.insert(f.end(), text, text + strlen(text)); record(f);
  }
  void directOwn(const char *id, const char *text, uint32_t t, uint32_t tag, bool delivered) {
    std::vector<uint8_t> f = {0xf4, 0, 0}; put32(f, t); hex6(f, id); put32(f, tag);
    f.insert(f.end(), text, text + strlen(text)); record(f);
    if (delivered) {
      std::vector<uint8_t> a = {0x82}; put32(a, tag); record(a);
    }
  }
  bool ready() const override { return isReady; }
  unsigned targetCount() const override { return unsigned(targets.size()); }
  bool target(unsigned i, dev::Target &o) const override {
    if (i >= targets.size()) return false;
    o = targets[i]; return true;
  }
  uint32_t historySeq() const override { return seq; }
  bool historyRecord(size_t slot, disaster::HistoryRecord &r) const override {
    if (slot >= hist.size() || !disaster::valid(hist[slot])) return false;
    r = hist[slot]; return true;
  }
  uint32_t now() const override { return clock; }
  bool send(const char *id, const char *text, char *status, size_t size) override {
    ++sends; lastId = id; lastText = text;
    if (sendOk) {
      if (id[0] == 'c') channelOwn(id[1] - '0', text, clock);
      else directOwn(id, text, clock, 0x1000 + sends, false);
    }
    snprintf(status, size, "%s", sendOk ? "Gesendet; Zustellung offen" : "Senden pausiert: noch 5 s"); return sendOk;
  }
  const char *sendStatus() const override { return st; }
  bool roomLogin(const char *id, char *status, size_t size) override {
    ++logins; snprintf(rm.id, sizeof(rm.id), "%s", id); rm.pending = loginOk; rm.rights = 0;
    snprintf(status, size, "%s", loginOk ? "Anmeldung gesendet" : "Funkkern besch\xc3\xa4" "ftigt"); return loginOk;
  }
  dev::Room room() const override { return rm; }
  dev::Gps gps() const override { return g; }
  void setGpsReceiver(bool on) override { g.receiverOn = on; if (!on) g.fresh = false; }
  void setGpsShare(bool on) override { g.share = on; }
  bool advert(char *status, size_t size) override { ++adverts; snprintf(status, size, "%s", g.share && g.fresh ? "Bekanntgabe mit Standort gesendet" : "Bekanntgabe gesendet"); return true; }
  bool refresh() override { ++refreshes; return true; }
  void info(dev::Info &o) const override {
    snprintf(o.name, sizeof(o.name), "DAR-TDeck-3f2a"); o.firmware = firmwareLabel;
    o.channels = 2; o.contacts = unsigned(targets.size()) - 2; o.history = seq; o.bleConnected = true; o.defaultPin = true;
    o.clockOK = true; o.heapFree = 142 * 1024; o.heapMin = 118 * 1024; o.sdCard = true; o.sdConfig = true; o.sdLog = true;
    o.txFlood = 12; o.txDirect = 3; o.rxFlood = 57; o.rxDirect = 9;
  }
  bool saveConfig(char *status, size_t size) override { ++saves; snprintf(status, size, "Einstellungen auf SD gespeichert"); return true; }
  int mapIndex() const override { return map; }
  void setMapIndex(int i) override { map = i; }

  // Demo network around Bad Neuenahr-Ahrweiler.
  void demo() {
    add("c0", "\xc3\x96" "ffentlich", dev::Kind::Channel);
    add("c1", "Ahrtal-Hilfe", dev::Kind::Channel);
    add("d0a1b2c3d4e5f", "Anna (Dernau)", dev::Kind::Chat, 50.5300, 7.0400);
    add("d1122334455aa", "Feuerwehr-Lager", dev::Kind::Chat, 50.5432, 7.1130);
    add("d99887766aabb", "Repeater Ahrweiler", dev::Kind::Repeater, 50.5395, 7.0955);
    add("dcafe0012ab34", "Brett Ahrweiler", dev::Kind::Room, 50.5410, 7.0990);
    add("dbeef7788cd90", "Brett Sinzig", dev::Kind::Room, 50.5440, 7.2470);
    uint32_t t = clock - 3600;
    channelIn(0, "Max: Wasser an der Ahrtorstra\xc3\x9f" "e wieder klar, Ausgabe ab 14 Uhr", t);
    channelOwn(0, "Danke, wir kommen mit 20 Kanistern vorbei.", t + 300);
    channelIn(1, "Lea: HILFE Keller unter Wasser 50.5299,7.0408", t + 600);
    directIn("d0a1b2c3d4e5f", "Brauchst du noch Sands\xc3\xa4" "cke?", t + 900);
    directOwn("d0a1b2c3d4e5f", "Ja, 10 St\xc3\xbc" "ck w\xc3\xa4ren super.", t + 1000, 0x55aa, true);
    roomIn("dcafe0012ab34", "0a1b2c3d", "Suche Notstromaggregat f\xc3\xbcr K\xc3\xbchltruhe", t + 1200);
    roomIn("dcafe0012ab34", "11223344", "Biete Bautrockner, Abholung Marktplatz", t + 1500);
    channelIn(0, "Ben: Apotheke am Markt hat ge\xc3\xb6" "ffnet", t + 2400);
  }
};
