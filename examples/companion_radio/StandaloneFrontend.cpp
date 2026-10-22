// Frontend of the LilyGO T-Deck Plus standalone build (DISASTER_STANDALONE, Issue #35; see Frontend.h).
// No WLAN, no web server: the local companion client (LocalCompanion.h) drives the display pages
// (ui-brand/StandaloneUI) through dev::Api, next to the MeshCore app over PIN Bluetooth. Adds the SD settings
// file and event log (no PINs, no keys, no message texts in the log) and "Standort teilen" (GpsShare.h).
#include "Frontend.h"
#ifdef DISASTER_STANDALONE

#include <Arduino.h>
#include <stdarg.h>
#include <SPIFFS.h>
#include <Preferences.h>
#include <helpers/MultiSerialInterface.h>
#include <helpers/esp32/SerialBLEInterface.h>
#include "MyMesh.h"
#include "OpenAccess.h"   // DISASTER_BLE_PIN
#include "LocalCompanion.h"
#include "GpsShare.h"
#include "DeviceConfig.h"
#include "ui-brand/DeviceApi.h"
#include "ui-brand/StandaloneUI.h"
#ifdef DISASTER_SD
#include <SD.h>
#endif

extern SerialBLEInterface bluetooth_interface;   // main.cpp

namespace {
LocalCompanion companion;
gpsshare::Share gps_share;
Preferences nvs;                 // "hf-sa": share toggle, map (survive without SD card)
bool share_on = false;
int map_index = 0;
bool log_on = true, sd_config = false;
char sd_note[64] = "";
uint32_t next_gps = 0, next_targets = 0, next_heap = 0;
bool last_fix = false;
constexpr uint32_t FRESH_MS = 15000;

// ------------------------------------------------------------------ SD card: config + log
#ifdef DISASTER_SD
const char *CONFIG_PATH = "/disaster/config.txt", *LOG_PATH = "/disaster/log.txt", *LOG_OLD = "/disaster/log.old.txt";
constexpr size_t LOG_LIMIT = 64 * 1024;
#endif
bool sdReady() {
#ifdef DISASTER_SD
  return companion.archiveReady();
#else
  return false;
#endif
}
void logEvent(const char *fmt, ...) {
  char line[120]; va_list ap; va_start(ap, fmt); vsnprintf(line, sizeof(line), fmt, ap); va_end(ap);
  Serial.printf("[STANDALONE] %s\n", line);
#ifdef DISASTER_SD
  if (!log_on || !sdReady()) return;
  File f = SD.open(LOG_PATH, FILE_APPEND);
  if (!f) return;
  size_t size = f.size();
  if (size > LOG_LIMIT) {
    f.close();
    if (SD.exists(LOG_OLD)) SD.remove(LOG_OLD);
    SD.rename(LOG_PATH, LOG_OLD);
    f = SD.open(LOG_PATH, FILE_APPEND);
    if (!f) return;
  }
  char when[16]; theme::stampText(rtc_clock.getCurrentTime(), when, sizeof(when));
  f.printf("%s  %s\n", when, line);
  f.close();
#endif
}

void loadConfig() {
#ifdef DISASTER_SD
  if (!sdReady()) { snprintf(sd_note, sizeof(sd_note), "keine Karte / nicht lesbar"); return; }
  File f = SD.open(CONFIG_PATH, FILE_READ);
  if (!f) { snprintf(sd_note, sizeof(sd_note), "noch keine config.txt"); return; }
  static char buf[devcfg::MAX_BYTES + 1];
  size_t size = f.size(), n = 0;
  if (size <= devcfg::MAX_BYTES) n = f.read((uint8_t *)buf, size);
  f.close();
  devcfg::Config c;
  if (size > devcfg::MAX_BYTES || n != size || !devcfg::parse(buf, n, c)) { snprintf(sd_note, sizeof(sd_note), "config.txt zu gross/defekt"); return; }
  sd_config = true;
  if (c.hasLog) log_on = c.log;
  if (c.hasShare) { share_on = c.share; nvs.putBool("share", share_on); }
  if (c.hasMap) { int i = sui::mapIndexForKey(c.map); if (i >= 0) { map_index = i; nvs.putInt("map", i); } }
  if (c.secrets) snprintf(sd_note, sizeof(sd_note), "%u geheime Zeile(n) ignoriert", c.secrets);
  else if (c.rejected || c.unknown) snprintf(sd_note, sizeof(sd_note), "%u Zeile(n) ignoriert", c.rejected + c.unknown);
#endif
}
char config_name[devcfg::NAME_BYTES + 1] = "";

void readConfigName() {
#ifdef DISASTER_SD
  if (!sdReady()) return;
  File f = SD.open(CONFIG_PATH, FILE_READ);
  if (!f) return;
  static char buf[devcfg::MAX_BYTES + 1];
  size_t size = f.size(), n = size <= devcfg::MAX_BYTES ? f.read((uint8_t *)buf, size) : 0;
  f.close();
  devcfg::Config c;
  if (n == size && devcfg::parse(buf, n, c) && c.hasName && strcmp(c.name, the_mesh.getNodePrefs()->node_name))
    snprintf(config_name, sizeof(config_name), "%s", c.name);
#endif
}

bool saveConfig(char *status, size_t size) {
#ifdef DISASTER_SD
  if (!sdReady()) { snprintf(status, size, "Keine SD-Karte"); return false; }
  devcfg::Config c;
  snprintf(c.name, sizeof(c.name), "%s", the_mesh.getNodePrefs()->node_name);
  c.share = share_on; c.log = log_on;
  const char *k = sui::mapKey(map_index); snprintf(c.map, sizeof(c.map), "%s", k ? k : "mansfelder-land");
  char out[512]; size_t n = devcfg::format(c, out, sizeof(out));
  File f = SD.open(CONFIG_PATH, FILE_WRITE);
  bool ok = n && f && f.write((const uint8_t *)out, n) == n;
  if (f) f.close();
  sd_config = ok;
  snprintf(status, size, "%s", ok ? "Einstellungen auf SD gespeichert" : "SD: Schreiben fehlgeschlagen");
  logEvent("Einstellungen gespeichert: %s", ok ? "ok" : "Fehler");
  return ok;
#else
  snprintf(status, size, "Ohne SD-Unterstuetzung gebaut"); return false;
#endif
}

// ------------------------------------------------------------------ GPS
LocationProvider *gpsProvider() { return sensors.getLocationProvider(); }
gpsshare::Fix currentFix() {
  LocationProvider *l = gpsProvider();
  gpsshare::Fix f;
  if (!l || !the_mesh.getNodePrefs()->gps_enabled) return f;
  f.valid = l->isValid();
  f.fresh = f.valid && l->hasFreshFix(FRESH_MS);
  f.lat = l->getLatitude() / 1e6; f.lon = l->getLongitude() / 1e6;
  return f;
}
void prefsLocation(double &lat, double &lon, uint8_t &policy) {
  gpsshare::Loc node(sensors.node_lat, sensors.node_lon, the_mesh.advertLocPolicy());
  gpsshare::Loc keep = gps_share.persist(node, currentFix());
  lat = keep.lat; lon = keep.lon; policy = keep.policy;
}
void serviceGps() {
  gpsshare::Fix fix = currentFix();
  gpsshare::Loc node(sensors.node_lat, sensors.node_lon, the_mesh.advertLocPolicy());
  gpsshare::Loc use = gps_share.update(node, share_on, fix);
  sensors.node_lat = use.lat; sensors.node_lon = use.lon; the_mesh.setAdvertLocPolicy(use.policy);
  if (fix.fresh != last_fix) { last_fix = fix.fresh; logEvent("GPS: %s", fix.fresh ? "frischer Fix" : "kein frischer Fix"); }
}

// ------------------------------------------------------------------ dev::Api on top of the companion
dev::Target cache[8 + 64];
unsigned cached = 0;
int hexByte(const String &h, unsigned i) {
  if (h.length() < 2 * i + 2) return -1;
  int a = disaster::hexNibble(h[2 * i]), b = disaster::hexNibble(h[2 * i + 1]);
  return a < 0 || b < 0 ? -1 : a * 16 + b;
}
void hexText(const String &h, unsigned from, unsigned to, char *out, size_t cap) {
  size_t n = 0;
  for (unsigned i = from; i < to && n + 1 < cap; ++i) { int b = hexByte(h, i); if (b <= 0) break; out[n++] = char(b); }
  // never end inside a UTF-8 sequence (names are cut at 32 bytes by the core)
  size_t k = n; while (k && (uint8_t(out[k - 1]) & 0xC0) == 0x80) --k;
  if (k && uint8_t(out[k - 1]) >= 0xC0) {
    uint8_t lead = uint8_t(out[k - 1]); size_t need = lead >= 0xF0 ? 4 : lead >= 0xE0 ? 3 : 2;
    if (n - (k - 1) < need) n = k - 1;
  }
  out[n] = 0;
}
int32_t hexI32(const String &h, unsigned at) {
  uint32_t v = 0; for (int i = 3; i >= 0; --i) { int b = hexByte(h, at + i); if (b < 0) return 0; v = (v << 8) | uint32_t(b); }
  return int32_t(v);
}
void rebuildTargets() {
  unsigned n = 0;
  for (unsigned i = 0; i < 8; ++i) {
    const String &h = companion.channelHex(i);
    if (!h.length()) continue;
    dev::Target &t = cache[n++]; memset(&t, 0, sizeof(t));
    snprintf(t.id, sizeof(t.id), "c%u", i); t.kind = dev::Kind::Channel;
    hexText(h, 2, 34, t.name, sizeof(t.name));
    if (!t.name[0]) snprintf(t.name, sizeof(t.name), "Kanal %u", i);
  }
  for (unsigned i = 0; i < 64; ++i) {
    const String &h = companion.contactHex(i);
    if (h.length() < 264) continue;
    dev::Target &t = cache[n++]; memset(&t, 0, sizeof(t));
    t.id[0] = 'd'; for (int k = 0; k < 12; ++k) t.id[1 + k] = char(tolower(h[2 + k]));
    int type = hexByte(h, 33);
    t.kind = type == 1 ? dev::Kind::Chat : type == 2 ? dev::Kind::Repeater : type == 3 ? dev::Kind::Room : dev::Kind::Sensor;
    hexText(h, 100, 132, t.name, sizeof(t.name));
    if (!t.name[0]) snprintf(t.name, sizeof(t.name), "%.6s", t.id + 1);
    if (h.length() >= 288) {
      int32_t la = hexI32(h, 136), lo = hexI32(h, 140);
      if (la || lo) { t.hasPos = true; t.lat = la / 1e6; t.lon = lo / 1e6; }
    }
  }
  cached = n;
}

class StandaloneApi : public dev::Api {
  mutable File hist;
public:
  bool ready() const override { return companion.ready(); }
  unsigned targetCount() const override { return cached; }
  bool target(unsigned i, dev::Target &out) const override { if (i >= cached) return false; out = cache[i]; return true; }
  uint32_t historySeq() const override { return companion.historySeq(); }
  void historyScan(bool start) const override { if (hist) hist.close(); if (start) hist = companion.openHistory(); }
  bool historyRecord(size_t slot, disaster::HistoryRecord &r) const override {
    return hist ? LocalCompanion::readHistory(hist, slot, r) : companion.readHistory(slot, r);
  }
  uint32_t now() const override { return rtc_clock.getCurrentTime(); }
  bool send(const char *id, const char *text, char *status, size_t size) override {
    bool ok = companion.localSend(id, text, now(), status, size);
    logEvent("Senden an %s (%u Byte): %s", id[0] == 'c' ? "Kanal" : "Kontakt", unsigned(strlen(text)), ok ? "vorgemerkt" : status);
    return ok;
  }
  const char *sendStatus() const override { return companion.localStatus(); }
  bool roomLogin(const char *id, char *status, size_t size) override {
    bool ok = companion.localRoomLogin(id, now(), status, size);
    logEvent("Brett-Anmeldung: %s", ok ? "gesendet" : status);
    return ok;
  }
  dev::Room room() const override {
    dev::Room r; snprintf(r.id, sizeof(r.id), "%s", companion.roomTargetId().c_str());
    r.rights = companion.roomRightsNow(); r.pending = companion.roomPendingNow(); return r;
  }
  dev::Gps gps() const override {
    dev::Gps g; LocationProvider *l = gpsProvider();
    g.detected = sensors.isGPSDetected(); g.receiverOn = the_mesh.getNodePrefs()->gps_enabled; g.share = share_on;
    if (l && g.receiverOn) {
      g.sats = l->satellitesCount();
      g.fresh = l->isValid() && l->hasFreshFix(FRESH_MS);
      g.lat = l->getLatitude() / 1e6; g.lon = l->getLongitude() / 1e6;
    }
    return g;
  }
  void setGpsReceiver(bool on) override {
    NodePrefs *p = the_mesh.getNodePrefs();
    if (on && !sensors.isGPSDetected()) return;
    p->gps_enabled = on ? 1 : 0; the_mesh.applyGpsPrefs(); the_mesh.savePrefs();
    logEvent("GPS-Empfaenger %s", on ? "an" : "aus");
  }
  void setGpsShare(bool on) override {
    share_on = on; nvs.putBool("share", on); serviceGps();
    logEvent("Standort teilen %s", on ? "an" : "aus");
  }
  bool advert(char *status, size_t size) override {
    serviceGps();
    bool withPos = the_mesh.advertLocPolicy() != ADVERT_LOC_NONE;
    bool ok = the_mesh.floodAdvert();
    snprintf(status, size, "%s", !ok ? "Bekanntgabe fehlgeschlagen" : withPos ? (gps_share.sharingFix() ? "Bekanntgabe mit GPS-Standort gesendet" : "Bekanntgabe mit Standort gesendet") : "Bekanntgabe ohne Standort gesendet");
    logEvent("%s", status);
    return ok;
  }
  bool refresh() override { return companion.localRefresh(); }
  void info(dev::Info &o) const override {
    NodePrefs *p = the_mesh.getNodePrefs();
    snprintf(o.name, sizeof(o.name), "%s", p->node_name); o.firmware = FIRMWARE_VERSION;
    unsigned ch = 0, co = 0; for (unsigned i = 0; i < cached; ++i) (cache[i].kind == dev::Kind::Channel ? ch : co)++;
    o.channels = ch; o.contacts = co; o.history = companion.historySeq(); o.historyOK = companion.historyOK();
    o.bleConnected = bluetooth_interface.isConnected(); o.defaultPin = the_mesh.getBLEPin() == DISASTER_BLE_PIN;
    o.clockOK = disaster::validStamp(rtc_clock.getCurrentTime());
    o.heapFree = ESP.getFreeHeap(); o.heapMin = ESP.getMinFreeHeap();
    o.sdCard = sdReady(); o.sdArchive = sdReady(); o.sdConfig = sd_config; o.sdLog = log_on && sdReady();
    snprintf(o.sdNote, sizeof(o.sdNote), "%s", sd_note);
    o.txFlood = the_mesh.getNumSentFlood(); o.txDirect = the_mesh.getNumSentDirect();
    o.rxFlood = the_mesh.getNumRecvFlood(); o.rxDirect = the_mesh.getNumRecvDirect();
  }
  bool saveConfig(char *status, size_t size) override { return ::saveConfig(status, size); }
  int mapIndex() const override { return map_index; }
  void setMapIndex(int i) override { map_index = i; nvs.putInt("map", i); }
} standalone_api;
}

dev::Api &dev::api() { return standalone_api; }

bool frontend::mountFilesystem() {
  SPIFFS.begin(true);   // format on first boot (same as the plain companion); history + outbox live here
  return true;
}

void frontend::begin(MultiSerialInterface &interfaces) {
  nvs.begin("hf-sa", false);
  share_on = nvs.getBool("share", false);       // off until the user switches it on
  map_index = nvs.getInt("map", 0);
  companion.beginLocal();                        // history, outbox, SD archive (mounts the card)
  interfaces.addInterface(InterfaceType::Portal, &companion);
  the_mesh.setLocalOrigin("T-Deck");
  the_mesh.setAppSentObserver([](const uint8_t *cmd, size_t len, const uint8_t *tag) { companion.recordAppSend(cmd, len, tag); });
  gps_share.begin(gpsshare::Loc(sensors.node_lat, sensors.node_lon, the_mesh.advertLocPolicy()));
  the_mesh.setPrefsLocationHook(prefsLocation);
  loadConfig();
  readConfigName();
  logEvent("Start %s, Heap %u, SD %s", FIRMWARE_VERSION, (unsigned)ESP.getFreeHeap(), sdReady() ? "ja" : "nein");
}

void frontend::sensorsReady() {}

void frontend::loop() {
  uint32_t now = millis();
  companion.serviceLocal();
  if (int32_t(now - next_targets) >= 0) { next_targets = now + 1000; rebuildTargets(); }
  if (int32_t(now - next_gps) >= 0) { next_gps = now + 1000; serviceGps(); }
  if (config_name[0] && companion.ready() && companion.localSetName(config_name)) {
    logEvent("Knotenname aus config.txt gesetzt");
    config_name[0] = 0;
  }
  if (int32_t(now - next_heap) >= 0) {
    next_heap = now + 30000;
    uint32_t f = ESP.getFreeHeap();
    Serial.printf("[HEAP] frei %u | Minimum %u | groesster Block %u\n", (unsigned)f, (unsigned)ESP.getMinFreeHeap(), (unsigned)ESP.getMaxAllocHeap());
    if (f < 40 * 1024) logEvent("Wenig freier Speicher: %u", (unsigned)f);
  }
}
#endif
