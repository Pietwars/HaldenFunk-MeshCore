// T-Deck Plus standalone display (Issue #35). See StandaloneUI.h.
#if defined(DISASTER_STANDALONE)
#include "StandaloneUI.h"
#include "Widgets.h"
#include <stdio.h>
#include <math.h>

namespace sui {
using namespace theme;
namespace {
// Main menu: the six entries of the web tabs, in the web order.
const Page MENU_PAGE[MENU_COUNT] = {CHAT_LIST, QUICK, BOARD_LIST, MAP, LOCATION, DEVICE};
const char *const MENU_LABEL[MENU_COUNT] = {"Chat", "Meldung", "Brett", "Karte", "Standort", "Ger\xc3\xa4t"};
const char *title(Page p) {
  switch (p) {
    case CHAT_LIST: case CHAT_VIEW: return "Chat";
    case QUICK: return "Meldung";
    case BOARD_LIST: case BOARD_VIEW: return "Brett";
    case MAP: return "Karte";
    case LOCATION: return "Standort";
    case DEVICE_INFO:
      return "Info";
    case DEVICE: return "Ger\xc3\xa4t";
    default:
      return "Men\xc3\xbc";
    }
}
constexpr int LIST_TOP = touchui::CONTENT, ROW_H = touchui::ROW, HINT_Y = H - 9;
constexpr int COMPOSE_Y = touchui::COMPOSE_Y, COMPOSE_H = touchui::COMPOSE_H;
constexpr int LIST_ROWS = touchui::VISIBLE_ROWS; // 10
constexpr int DEVICE_BUTTONS = 7;                // plus saved touch calibration
}

int mapIndexForKey(const char *key) {
  for (int i = 0; key && i < devmap::MAP_COUNT; ++i) if (!strcmp(devmap::MAPS[i].key, key)) return i;
  return -1;
}
const char *mapKey(int index) { return index >= 0 && index < devmap::MAP_COUNT ? devmap::MAPS[index].key : nullptr; }

bool UI::begin(dev::Api *a) {
  api = a;
  bool ok = log.begin();
  map.select(a ? a->mapIndex() : 0);
#ifdef SUI_HAS_QUICK
  quick.begin(a);
#endif
  dirty = FULL;
  return ok;
}

bool UI::quickText() const {
#ifdef SUI_HAS_QUICK
  return quick.textFocus();
#else
  return false;
#endif
}

void UI::setBattery(int pct, const char *line, const char *estimate) {
  bool texts = strcmp(battLine, line ? line : "") || strcmp(battEst, estimate ? estimate : "");
  bool level = pct != battPct;
  battPct = pct; ui::navBattery() = pct;
  snprintf(battLine, sizeof(battLine), "%s", line ? line : "");
  snprintf(battEst, sizeof(battEst), "%s", estimate ? estimate : "");
  if (page != BOOT && (level || (texts && page == DEVICE))) dirty = FULL;
}

void UI::setStatus(const char *s, uint32_t ms) {
  snprintf(status, sizeof(status), "%s", s);
  statusUntil = nowMs + ms;
}

unsigned UI::buildList(Page p, dev::Target *out, unsigned cap) const {
  unsigned n = 0; dev::Target t;
  if (!api) return 0;
  for (unsigned i = 0, c = api->targetCount(); i < c && n < cap; ++i) {
    if (!api->target(i, t)) continue;
    bool want = p == BOARD_LIST ? t.kind == dev::Kind::Room : (t.kind == dev::Kind::Channel || t.kind == dev::Kind::Chat);
    if (want) out[n++] = t;
  }
  return n;
}

void UI::open(Page p) {
  if (p == CHAT_VIEW || p == BOARD_VIEW) log.markRead(conv);
#ifdef SUI_HAS_QUICK
  if (p == QUICK && page != QUICK) { quick.setTarget(conv[0] && !convRoom ? conv : "c0"); quick.enter(); }
#endif
  if (p == MAP && page != MAP) {
    mapReturn = page == BOOT ? MENU : page; mapFollow = true;
    dev::Gps g = api->gps();
    if (g.receiverOn && g.fresh) { map.centerOn(g.lat,g.lon); followLat=g.lat; followLon=g.lon; }
    else map.centerOn(51.527900, 11.548000); // Lutherstadt Eisleben (Mansfelder Land): view only, never a GPS/advert position
  }
  ++touchRevision;
  page = p; scroll = 0; dirty = FULL; nhits = 0;   // touch targets belong to the drawn page
}

void UI::openConversation(const dev::Target &t, bool room) {
  if (strcmp(conv, t.id)) compose[0] = 0;   // a draft belongs to its conversation
  snprintf(conv, sizeof(conv), "%s", t.id);
  snprintf(convName, sizeof(convName), "%s %s", t.kind == dev::Kind::Channel ? "#" : room ? "Brett:" : "@", t.name);
  convRoom = room;
  open(room ? BOARD_VIEW : CHAT_VIEW);
}

bool UI::back() {
  if (page == QUICK) {
#ifdef SUI_HAS_QUICK
    if (quick.closePicker()) {
      dirty = FULL;
      return true;
    }
#endif
  }
  switch (page) {
  case DEVICE_INFO:
    open(DEVICE);
    return true;
  case BOOT:
    open(MENU);
    return true;
  case MENU:
    return false;
  case CHAT_VIEW:
    open(CHAT_LIST);
    return true;
  case BOARD_VIEW:
    open(BOARD_LIST);
    return true;
  case MAP:
    leaveMap();
    return true;
  default:
    open(MENU);
    return true;
  }
}

void UI::addHit(int x, int y, int w, int h, int idx) {
  if (nhits >= MAX_HITS) return;
  Hit &t = hits[nhits++]; t.x = int16_t(x); t.y = int16_t(y); t.w = int16_t(w); t.h = int16_t(h); t.idx = int16_t(idx);
  t.id[0] = 0;
}

bool UI::listMove(Page p, int dy, unsigned n) {
  if (!n) return false;
  int s = listSel[p] + (dy > 0 ? 1 : -1);
  if (s < 0) s = 0;
  if (s >= int(n)) s = int(n) - 1;
  if (s == listSel[p]) return false;
  listSel[p] = s;
  if (s < listTop[p]) listTop[p] = s;
  if (s >= listTop[p] + LIST_ROWS) listTop[p] = s - LIST_ROWS + 1;
  return true;
}

void UI::sendCompose() {
  char st[96] = "";
  if (!api) return;
  if (!compose[0]) {
    if (convRoom) { api->roomLogin(conv, st, sizeof(st)); setStatus(st, 15000); }
    else setStatus("Zuerst Text eingeben");
    dirty = FULL; return;
  }
  bool ok = api->send(conv, compose, st, sizeof(st));
  setStatus(st, ok ? 15000 : 8000);
  if (ok) { compose[0] = 0; scroll = 0; }
  dirty = FULL;
}

void UI::sendPosition() {
  if (!api) return;
  dev::Gps g = api->gps();
  if (!g.share) { setStatus("Erst \"Standort teilen\" einschalten"); return; }
  if (!g.receiverOn || !g.fresh) { setStatus("Kein frischer GPS-Fix \xe2\x80\x93 bitte warten"); return; }
  uint32_t t = api->now();
  if (t < 1700000000u) { setStatus("Uhr fehlt: GPS-Fix abwarten / App verbinden"); return; }
  char text[64], st[96] = "";
  snprintf(text, sizeof(text), "!DP1|%.4f,%.4f|%lu", g.lat, g.lon, (unsigned long)t);   // web: Position in Chat senden
  const char *to = conv[0] && !convRoom ? conv : "c0";
  api->send(to, text, st, sizeof(st));
  setStatus(st, 15000);
}

// ---------------------------------------------------------------------------------------------- input
bool UI::key(uint8_t k) {
  if (!api || !k) return false;
  if (page == BOOT) { leaveBoot(); return true; }   // the first key only leaves the logo
  if (k == '\n') k = K_ENTER;
  if (k == 127) k = K_BACKSPACE;
  if (k == K_BACK) return back();                   // ALT+Backspace (UITask) / Esc: one level back, everywhere
  Redraw before = dirty;
  if (page == CHAT_VIEW || page == BOARD_VIEW) {
    size_t n = strlen(compose);
    if (k == K_ENTER) sendCompose();
    else if (k == K_BACKSPACE) {
      if (!n) return back();                        // empty field: fallback for keyboards without ALT detection
      compose[n - 1] = 0; if (dirty < PART) dirty = PART;
    } else if (k >= 32 && k <= 126) {
      if (n < COMPOSE_BYTES) { compose[n] = char(k); compose[n + 1] = 0; if (dirty < PART) dirty = PART; }
      else { setStatus("Text voll: max. 100 Byte"); if (dirty < PART) dirty = PART; }
    }
    return dirty != before || dirty != NONE;
  }
#ifdef SUI_HAS_QUICK
  if (page == QUICK) {
    if (quick.key(k, api->now())) open(MENU);     // Backspace outside the text / "Zurück" action: menu
    dirty = FULL; return true;
  }
#endif
  if (k == K_BACKSPACE) return back();             // no text field here: plain Backspace also goes back
  if (k == 'd' || k == 'D') { requestDisplayOff(); return true; }   // display off (any input wakes it)
  if (page == MAP) {
    if (k == '+' || k == 'z' || k == 'Z') map.zoom(2);
    else if (k == '-' || k == 'x' || k == 'X') map.zoom(0.5);
    else if (k == 'a' || k == 'A') { mapFollow=false; map.fit(); }
    else if (k == 'i' || k == 'I') {
      dev::Gps g = api->gps();
      if (g.receiverOn && g.fresh) { mapFollow=true; map.centerOn(g.lat, g.lon); followLat=g.lat; followLon=g.lon; } else { setStatus("Kein frischer GPS-Fix"); }
    }
    else if (k == 'n' || k == 'N' || k == K_ENTER || k == ' ') { map.select(map.index() + 1); api->setMapIndex(map.index()); }
    else return false;
    dirty = FULL; return true;
  }
  if (k == K_ENTER || k == ' ') return click();
  return false;
}

// Trackball: moves the selection inside the page (never switches pages). In vertical lists left/right act like
// up/down; on the map it pans.
bool UI::move(int dx, int dy) {
  if (!api || (!dx && !dy)) return false;
  int step = dy ? dy : dx;
  switch (page) {
    case BOOT: leaveBoot(); return true;
    case MENU: {
      int s = menuSel + (step > 0 ? 1 : -1);
      if (s < 0) s = 0;
      if (s >= MENU_COUNT) s = MENU_COUNT - 1;
      if (s == menuSel) return false;
      menuSel = s; dirty = FULL; return true;
    }
    case CHAT_LIST: case BOARD_LIST: {
      dev::Target list[MAX_LIST]; unsigned n = buildList(page, list, MAX_LIST);
      if (listMove(page, step, n)) { dirty = FULL; return true; }
      return false;
    }
    case CHAT_VIEW: case BOARD_VIEW:
      if (dy) { scroll += dy < 0 ? 1 : -1; if (scroll < 0) scroll = 0; if (scroll > 95) scroll = 95; dirty = FULL; return true; }
      return false;
#ifdef SUI_HAS_QUICK
    case QUICK:
      if (quick.move(dx, dy)) { dirty = FULL; return true; }
      return false;
#endif
    case MAP:
      mapFollow=false; map.pan(dx * 32, dy * 32); dirty = FULL; return true;
    case LOCATION: case DEVICE: {
      int &sel = page == LOCATION ? locSel : devSel; int n = page == LOCATION ? 4 : DEVICE_BUTTONS;
      int s = sel + (step > 0 ? 1 : -1); if (s < 0) s = 0; if (s >= n) s = n - 1;
      if (s == sel) return false;
      sel = s;
      if (page == DEVICE) {
        if (devSel < devTop) devTop = devSel;
        if (devSel >= devTop + LIST_ROWS) devTop = devSel - LIST_ROWS + 1;
      }
      dirty = FULL;
      return true;
    }
    default: return false;
  }
}

bool UI::click() {
  if (!api) return false;
  char st[96] = "";
  switch (page) {
    case BOOT: leaveBoot(); return true;
    case MENU: open(MENU_PAGE[menuSel]); return true;
    case CHAT_LIST: case BOARD_LIST: {
      dev::Target list[MAX_LIST]; unsigned n = buildList(page, list, MAX_LIST);
      if (!n) { setStatus(page == BOARD_LIST ? "Kein Brett (Room-Server) bekannt" : "Noch keine Kan\xc3\xa4le geladen"); dirty = FULL; return true; }
      if (listSel[page] >= int(n)) listSel[page] = int(n) - 1;
      openConversation(list[listSel[page]], page == BOARD_LIST); return true;
    }
    case CHAT_VIEW: case BOARD_VIEW: sendCompose(); return true;
#ifdef SUI_HAS_QUICK
    case QUICK: if (quick.activate(api->now())) open(MENU); dirty = FULL; return true;
#endif
    case MAP: leaveMap(); return true;   // the trackball pans; its click is the "Zurück" button
    case LOCATION: {
      dev::Gps g = api->gps();
      if (locSel == 0) api->setGpsReceiver(!g.receiverOn);
      else if (locSel == 1) { api->setGpsShare(!g.share); setStatus(!g.share ? "Standort wird geteilt (Bekanntgabe, Meldungen)" : "Standort wird nicht mehr geteilt"); }
      else if (locSel == 2) sendPosition();
      else { api->advert(st, sizeof(st)); setStatus(st); }
      dirty = FULL; return true;
    }
    case DEVICE_INFO:
      return back();
    case DEVICE:
      if (devSel == 6) {
        open(DEVICE_INFO);
        return true;
      }
      if (devSel == 0) { setStatus(api->refresh() ? "Kontakte werden neu gelesen" : "Funkkern besch\xc3\xa4" "ftigt \xe2\x80\x93 sp\xc3\xa4ter erneut"); }
      else if (devSel == 1) { api->advert(st, sizeof(st)); setStatus(st); }
      else if (devSel == 2) { requestDisplayOff(); return true; }
      else if (devSel == 4) { touchCalibrationReq=true; return true; }
      else if (devSel == 5) { open(LOCATION); return true; }
      else { api->saveConfig(st, sizeof(st)); setStatus(st); }
      dirty = FULL; return true;
    default: return false;
  }
}

// Touch: status line = back; otherwise the targets recorded while the page was drawn (menu entries, list rows,
// buttons, compose box, the map's "Zurück"). A tap acts like selecting the target and clicking.
int UI::touchTarget(int x, int y) const {
  if (x < 0 || x >= W || y < 0 || y >= H) return -1;
  if (y < TAB_H) return x % 64 >= 2 && x % 64 < 62 && y >= 2 && y < 34 ? 100 + x / 64 : -1;
  if (y < NAV_H) return x < ui::BACK_ZONE_W ? 110 : -1;
  if (page == MAP) {
    for (int i = 0; i < 3; ++i)
      if (touchui::mapControl(i).contains(x, y)) return 120 + i;
    if (touchui::Rect{ 228, NAV_H + 4, 88, 32 }.contains(x, y)) return 123;
  }
#ifdef SUI_HAS_QUICK
  if (page == QUICK) return quick.touchTarget(x, y);
#endif
  for (int i = 0; i < nhits; ++i) {
    const Hit &h = hits[i];
    if (touchui::Rect{ h.x, h.y, h.w, h.h }.contains(x, y)) return h.idx;
  }
  return -1;
}

bool UI::tap(int x, int y) {
  if (!api || x < 0 || x >= W || y < 0 || y >= H) return false;
  if (page == BOOT) {
    leaveBoot();
    return true;
  }
  const int target = touchTarget(x, y);
  if (target < 0) return false;
  if (target >= 100 && target < 105) {
    const Page tabs[] = { CHAT_LIST, QUICK, BOARD_LIST, MAP, DEVICE };
    Page next = tabs[target - 100];
    if ((next == CHAT_LIST && page == CHAT_VIEW) || (next == BOARD_LIST && page == BOARD_VIEW)) return back();
    if (page != next) open(next);
    return true;
  }
  if (target == 110) return back();
  if (page == MAP) {
    if (target == 120) return key('-');
    if (target == 122) return key('+');
    if (target == 123) return key('n');
    return key('i');
  }
#ifdef SUI_HAS_QUICK
  if (page == QUICK) {
    quick.tap(x, y, api->now());
    dirty = FULL;
    return true;
  }
#endif
  if (page == CHAT_LIST || page == BOARD_LIST) {
    for (int i = 0; i < nhits; ++i)
      if (hits[i].idx == target) {
        dev::Target shown;
        if (dev::findTarget(*api, hits[i].id, shown)) {
          listSel[page] = target;
          openConversation(shown, page == BOARD_LIST);
        } else {
          setStatus("Kontakt nicht mehr verfuegbar / neu waehlen");
          dirty = FULL;
        }
        return true;
      }
    return false;
  }
  switch (page) {
  case MENU:
    menuSel = target;
    break;
  case CHAT_LIST:
  case BOARD_LIST:
    listSel[page] = target;
    break;
  case LOCATION:
    locSel = target;
    break;
  case DEVICE:
    devSel = target;
    break;
  default:
    break;
  }
  return click();
}

bool UI::swipeList(int dy) {
  const int rows = abs(dy) / ROW_H + 1, step = dy < 0 ? rows : -rows;
  if (page == CHAT_LIST || page == BOARD_LIST) {
    dev::Target list[MAX_LIST];
    int n = int(buildList(page, list, MAX_LIST));
    int &top = listTop[page], &sel = listSel[page];
    top += step;
    if (top > n - LIST_ROWS) top = n - LIST_ROWS;
    if (top < 0) top = 0;
    if (sel < top) sel = top;
    if (sel >= top + LIST_ROWS) sel = top + LIST_ROWS - 1;
  } else if (page == DEVICE) {
    devTop += step;
    if (devTop > DEVICE_BUTTONS - LIST_ROWS) devTop = DEVICE_BUTTONS - LIST_ROWS;
    if (devTop < 0) devTop = 0;
    if (devSel < devTop) devSel = devTop;
    if (devSel >= devTop + LIST_ROWS) devSel = devTop + LIST_ROWS - 1;
  } else if (page == CHAT_VIEW || page == BOARD_VIEW) {
    scroll += dy > 0 ? rows : -rows;
    if (scroll < 0) scroll = 0;
    if (scroll > 95) scroll = 95;
  } else
    return false;
  dirty = FULL;
  return true;
}

bool UI::gesture(int x, int y, int endX, int endY, bool travelled, uint32_t duration) {
  if (x < 0 || x >= W || endX < 0 || endX >= W || y < 0 || y >= H || endY < 0 || endY >= H) return false;
  const int dx = endX - x, dy = endY - y, ax = abs(dx), ay = abs(dy);
  if (page == BOOT) {
    if (ax <= 12 && ay <= 12 && !travelled && duration <= 700) {
      leaveBoot();
      return true;
    }
    return false;
  }
  // An edge swipe goes one level back. No horizontal page carousel over message actions.
  if (x < 20 && y >= NAV_H && endY >= NAV_H && dx > 64 && ax > ay * 2) return back();
  if (page == MAP && y >= NAV_H + 38 && y < 190 && endY >= NAV_H && endY < 190 && (ax > 20 || ay > 20)) {
    mapFollow = false;
    map.pan(-dx, -dy);
    dirty = FULL;
    return true;
  }
  if (y >= NAV_H && endY >= NAV_H && ay > 24 && ay > ax * 2) {
    // A drag beginning on SEND/other controls can only be cancelled, never sent.
    if ((page == CHAT_VIEW || page == BOARD_VIEW) && (y >= COMPOSE_Y || endY >= COMPOSE_Y)) return false;
#ifdef SUI_HAS_QUICK
    if (page == QUICK) {
      if (quick.swipe(dy)) {
        dirty = FULL;
        return true;
      }
      return false;
    }
#endif
    return swipeList(dy);
  }
  if (ax > 12 || ay > 12 || travelled || duration > 700) return false;
  int target = touchTarget(x, y);
  // Both ends belong to the same rendered action, including picker rows and SEND.
  return target >= 0 && target == touchTarget(endX, endY) && tap(x, y);
}

// ---------------------------------------------------------------------------------------------- state
bool UI::tick(uint32_t ms) {
  nowMs = ms;
  if (!api) return false;
  if (page == BOOT) {
    if (!bootUntil) bootUntil = ms + BOOT_MS ? ms + BOOT_MS : 1;
    else if (int32_t(ms - bootUntil) >= 0) leaveBoot();
  }
  if (status[0] && int32_t(ms - statusUntil) >= 0) { status[0] = 0; if (dirty < PART) dirty = PART; }
#ifdef SUI_HAS_QUICK
  if (page == QUICK && quick.tick(ms)) dirty = FULL;
  quick.unreadChat = unreadChat(); quick.unreadBoard = unreadBoard();
#endif
  if (int32_t(ms - nextPoll) < 0) return dirty != NONE;
  nextPoll = ms + 250;
  if (page == MAP && mapFollow) {
    dev::Gps g=api->gps();
    if (g.receiverOn && g.fresh && (g.lat!=followLat || g.lon!=followLon)) {
      map.centerOn(g.lat,g.lon); followLat=g.lat; followLon=g.lon; dirty=FULL;
    }
  }
  uint32_t seq = api->historySeq();
  if (api->ready() && log.needsRebuild(seq)) {
    bool first = log.size() == 0 && wakeSeq == 0;
    api->historyScan(true);
    log.rebuild(seq, [this](size_t slot, disaster::HistoryRecord &r) { return api->historyRecord(slot, r); });
    api->historyScan(false);
    if (first) { log.setBaseline(log.newest()); lastIncoming = log.newest(); wakeSeq = 1; }
    if (page == CHAT_VIEW || page == BOARD_VIEW) log.markRead(conv);
    if (page != MAP || log.incomingAfter(lastIncoming)) dirty = FULL;
  }
  uint32_t sig = api->targetCount() * 131u + (api->ready() ? 1 : 0);
  dev::Room r = api->room(); sig = sig * 31u + r.rights * 7u + (r.pending ? 3 : 0);
  if (sig != lastTargetSig) { lastTargetSig = sig; if (page != MAP) dirty = FULL; }
  return dirty != NONE;
}

bool UI::takeIncoming() {
  if (!log.size() || !log.incomingAfter(lastIncoming)) return false;
  lastIncoming = log.newest();
  return true;
}

unsigned UI::unreadChat() const {
  dev::Target list[MAX_LIST]; unsigned n = buildList(CHAT_LIST, list, MAX_LIST), u = 0;
  for (unsigned i = 0; i < n; ++i) u += log.unread(list[i].id, false);
  return u;
}
unsigned UI::unreadBoard() const {
  dev::Target list[MAX_LIST]; unsigned n = buildList(BOARD_LIST, list, MAX_LIST), u = 0;
  for (unsigned i = 0; i < n; ++i) u += log.unread(list[i].id, true);
  return u;
}

const char *UI::authorName(const chat::Entry &e, char *buf, size_t cap, const char **body) const {
  *body = e.text;
  if (e.own) { snprintf(buf, cap, "Du"); return buf; }
  if (e.conv[0] == 'c') { if (!chat::splitSender(e.text, buf, cap, body)) snprintf(buf, cap, "?"); return buf; }
  if (e.room) {
    dev::Target t;
    for (unsigned i = 0, n = api ? api->targetCount() : 0; i < n; ++i)
      if (api->target(i, t) && t.id[0] == 'd' && !strncmp(t.id + 1, e.author, 8)) { snprintf(buf, cap, "%s", t.name); return buf; }
    snprintf(buf, cap, "%s", e.author); return buf;
  }
  snprintf(buf, cap, "%s", convName[0] == '@' ? convName + 2 : convName);
  return buf;
}

// ---------------------------------------------------------------------------------------------- drawing
void UI::drawBoot(DisplayDriver *d) {
  d->startFrame(BG);
  if (splash) {
    uint16_t row[320];
    for (int y = 0; y < 213; ++y) {
      memcpy(row, splash + y * 640, sizeof(row));
      d->nativeImage(0, 13 + y, 320, 1, row);
    }
    d->endFrame(); return;
  }
  int y = logo ? (H - logoH - 34) / 2 : 70;
  if (y < 0) y = 0;
  if (logo) { d->nativeImage((W - logoW) / 2, y, logoW, logoH, logo); y += logoH + 8; }
  else { ui::centerText(d, W / 2, y, MESH, "HaldenFunk", 2); y += 24; }
  dev::Info in; api->info(in);
  char v[64]; snprintf(v, sizeof(v), "%s \xc2\xb7 ohne WLAN", in.firmware);
  ui::centerText(d, W / 2, y, MUTED, v); y += 11;
  ui::centerText(d, W / 2, y, BAD, "Kein zertifiziertes Notrufsystem \xc2\xb7 Lebensgefahr: 112");
  d->endFrame();
}

void UI::drawHint(DisplayDriver *d) {
  if (status[0]) { ui::hint(d, status, PEND); return; }
  const char *h = "";
  switch (page) {
  case MENU:
    h = "Antippen: oeffnen | Trackball / Enter auch moeglich";
    break;
  case CHAT_LIST:
  case BOARD_LIST:
    h = "Tippen: oeffnen | Wischen: blaettern | < Zurueck";
    break;
  case CHAT_VIEW:
    h = "Text: Tastatur | Senden: tippen / Enter | Wischen";
    break;
  case BOARD_VIEW:
    h = "Tippen: Senden / Beitreten | Wischen: Verlauf";
    break;
  case LOCATION:
  case DEVICE:
    h = "Tippen: ausfuehren | Wischen: weitere Aktionen";
    break;
  case DEVICE_INFO:
    h = "< Zurueck: Einstellungen | ALT+Del / Klick";
    break;
  default:
    break;
  }
  ui::hint(d, h);
}

void UI::drawMenu(DisplayDriver *d) {
  const char *sub[] = { "Kanaele / Kontakte", "Vorlage / Ort / Ziel", "Room-Server",
                        "Offline-Karten",     "GPS / Freigabe",       "Akku / Einstellungen" };
  for (int i = 0; i < MENU_COUNT; ++i) {
    auto r = touchui::tile(i);
    bool on = i == menuSel;
    ui::button(d, r.x, r.y, r.w, r.h, "", on);
    ui::text(d, r.x + 10, r.y + 7, on ? MESH : TEXT, MENU_LABEL[i], 12, 2);
    ui::text(d, r.x + 10, r.y + 31, MUTED, sub[i], 23);
    unsigned n = i == 0 ? unreadChat() : i == 2 ? unreadBoard() : 0;
    if (n) ui::badge(d, r.x + r.w - 4, r.y + 3, n);
    addHit(r.x, r.y, r.w, r.h, i);
  }
}

void UI::drawList(DisplayDriver *d, Page p) {
  dev::Target list[MAX_LIST]; unsigned n = buildList(p, list, MAX_LIST);
  if (!api->ready()) { ui::text(d, 6, LIST_TOP + 10, MUTED, "Funkkern startet \xe2\x80\x93 Kontakte werden gelesen \xe2\x80\xa6"); return; }
  if (!n) {
    ui::text(d, 6, LIST_TOP + 10, MUTED, p == BOARD_LIST ? "Kein Brett (Room-Server) in den Kontakten." : "Keine Kan\xc3\xa4le oder Kontakte.");
    ui::text(d, 6, LIST_TOP + 22, MUTED, p == BOARD_LIST ? "Room-Server erscheinen nach ihrer Bekanntgabe." : "Men\xc3\xbc \xe2\x86\x92 Ger\xc3\xa4t \xe2\x86\x92 Kontakte neu lesen.");
    return;
  }
  int &sel = listSel[p], &top = listTop[p];
  if (sel >= int(n)) sel = int(n) - 1;
  if (top > sel) top = sel;
  if (sel >= top + LIST_ROWS) top = sel - LIST_ROWS + 1;
  dev::Room room = api->room();
  for (int r = 0; r < LIST_ROWS && top + r < int(n); ++r) {
    const dev::Target &t = list[top + r];
    int y = LIST_TOP + r * ROW_H;
    bool on = top + r == sel;
    d->nativeFill(4, y, W - 8, ROW_H - 4, on ? SELF : CARD);
    if (on) d->nativeFill(0, y, 3, ROW_H - 1, MESH);
    addHit(4, y, W - 8, ROW_H - 4, top + r);
    snprintf(hits[nhits - 1].id, sizeof(hits[nhits - 1].id), "%s", t.id);
    char line[64];
    snprintf(line, sizeof(line), "%s %s", t.kind == dev::Kind::Channel ? "#" : p == BOARD_LIST ? "\xe2\x96\xa0" : "@", t.name);
    ui::text(d, 7, y + 5, p == BOARD_LIST ? BOARD : t.kind == dev::Kind::Channel ? MESH : TEXT, line, 23, 2);
    bool roomList = p == BOARD_LIST;
    unsigned u = log.unread(t.id, roomList);
    if (u) ui::badge(d, W - 4, y + 1, u);
    const chat::Entry *e = log.last(t.id, roomList);
    char who[34], prev[80]; const char *body = "";
    if (roomList) {
      bool here = !strcmp(room.id, t.id);
      snprintf(prev, sizeof(prev), "%s", here && room.pending ? "Anmeldung l\xc3\xa4uft \xe2\x80\xa6" : here && room.rights >= 2 ? "beigetreten" : "nicht beigetreten");
      if (e) { authorName(*e, who, sizeof(who), &body); size_t l = strlen(prev); snprintf(prev + l, sizeof(prev) - l, " \xc2\xb7 %s: %s", who, body); }
    } else if (e) {
      authorName(*e, who, sizeof(who), &body);
      snprintf(prev, sizeof(prev), "%s: %s", who, body);
    } else snprintf(prev, sizeof(prev), "%s", "noch keine Nachrichten");
    ui::text(d, 15, y + 22, MUTED, prev, 50);
  }
  if (int(n) > LIST_ROWS) {   // scroll bar
    int bh = (HINT_Y - LIST_TOP) * LIST_ROWS / int(n), by = LIST_TOP + (HINT_Y - LIST_TOP - bh) * top / (int(n) - LIST_ROWS);
    d->nativeFill(W - 2, by, 2, bh, LINE);
  }
}

void UI::drawCompose(DisplayDriver *d) {
  bool typingNow = compose[0] != 0;
  d->nativeFill(0, COMPOSE_Y - 1, W, COMPOSE_H + 2, BG);
  d->nativeFill(2, COMPOSE_Y, W - 4, COMPOSE_H, CARD);
  ui::button(d, touchui::SEND_X, COMPOSE_Y, 86, COMPOSE_H, convRoom && !compose[0] ? "Beitreten" : "Senden",
             false, true);
  d->nativeRect(4, COMPOSE_Y, 222, COMPOSE_H, typingNow ? ui::FOCUS : LINE);
  if (!nhits) addHit(touchui::SEND_X, COMPOSE_Y, 86, COMPOSE_H, 0); // tap = Enter (send / join)
  if (!typingNow)
    ui::text(d, 6, COMPOSE_Y + 4, MUTED, convRoom ? "Aushang tippen..." : "Nachricht tippen...");
  else {
    int lines = 0;
    // last two lines of the draft, cursor at the end
    int total = countLines(compose, 35);
    wrap(compose, 35, [&](const char *s, size_t n) {
      if (lines++ < total - 2) return;
      ui::raw(d, 6, COMPOSE_Y + 4 + (lines - 1 - (total > 2 ? total - 2 : 0)) * 9, TEXT, s, n);
    });
  }
  char cnt[12]; snprintf(cnt, sizeof(cnt), "%u/100", unsigned(strlen(compose)));
  d->nativeText(224 - int(strlen(cnt)) * CW - 2, COMPOSE_Y + COMPOSE_H - 8, 1,
                strlen(compose) >= COMPOSE_BYTES ? BAD : MUTED, cnt);
  drawHint(d);
}

void UI::drawConversation(DisplayDriver *d) {
  // header like the web .tgt row
  d->nativeFill(0, NAV_H, W, 11, CARD);
  ui::text(d, 4, NAV_H + 2, convRoom ? BOARD : MESH, convName, 34);
  if (convRoom) {
    dev::Room r = api->room();
    bool here = !strcmp(r.id, conv);
    const char *st = here && r.pending ? "Anmeldung l\xc3\xa4uft \xe2\x80\xa6" : here && r.rights >= 2 ? "beigetreten" : "Enter: Beitreten";
    ui::rightText(d, W - 4, NAV_H + 2, here && r.rights >= 2 ? MESH : PEND, st);
  } else ui::rightText(d, W - 4, NAV_H + 2, MUTED, "ALT+Del: Liste");
  const int top = NAV_H + 17, bottom = COMPOSE_Y - 2;
  const chat::Entry *items[chat::Log::CAP]; size_t n = 0;
  log.forEach(conv, convRoom, [&](const chat::Entry &e) { if (n < chat::Log::CAP) items[n++] = &e; });
  if (!n) ui::text(d, 6, top + 6, MUTED, convRoom ? "Noch keine Beitr\xc3\xa4ge. Nach dem Beitreten werden sie abgerufen." : "Noch keine Nachrichten in dieser Unterhaltung.", 52);
  if (scroll > int(n) - 1) scroll = n ? int(n) - 1 : 0;
  int y = bottom;
  char cp[200], who[34], meta[80], when[16];
  for (int i = int(n) - 1 - scroll; i >= 0 && y > top; --i) {
    const chat::Entry &e = *items[i];
    const char *body; authorName(e, who, sizeof(who), &body);
    toCp437(body, cp, sizeof(cp));
    const int cols = 45, x = e.own ? 40 : 4, w = W - 44;
    int lines = countLines(cp, cols); if (!lines) lines = 1;
    int h = 11 + lines * 9 + 2;
    int y0 = y - h;
    stampText(e.stamp, when, sizeof(when));
    snprintf(meta, sizeof(meta), "%s \xc2\xb7 %s%s", who, when,
             e.own ? (e.conv[0] == 'd' ? (e.delivered ? " \xc2\xb7 zugestellt" : " \xc2\xb7 gesendet") : " \xc2\xb7 gesendet") : "");
    // clip at the header: draw only what fits (the oldest visible bubble may be cut at the top)
    int cy = y0 < top ? top : y0;
    d->nativeFill(x, cy, w, y - 1 - cy, e.own ? SELF : CARD);
    if (y0 >= top) ui::text(d, x + 3, y0 + 2, e.own ? SAGE : MUTED, meta, 44);
    int ly = y0 + 11;
    wrap(cp, cols, [&](const char *s, size_t len) {
      if (ly >= top && ly + 8 <= bottom) ui::raw(d, x + 3, ly, TEXT, s, len);
      ly += 9;
    });
    y = y0 - 2;
  }
  if (scroll) ui::rightText(d, W - 4, bottom - 9, PEND, "\x19 neuere");
  drawCompose(d);
}

void UI::drawMap(DisplayDriver *d) {
  static devmap::Mark marks[48];
  unsigned nm = 0; dev::Target t;
  for (unsigned i = 0, n = api->targetCount(); i < n && nm < 32; ++i) {
    if (!api->target(i, t) || !t.hasPos) continue;
    devmap::Mark &m = marks[nm++];
    m.lat = t.lat; m.lon = t.lon;
    m.color = t.kind == dev::Kind::Room ? BOARD : t.kind == dev::Kind::Repeater ? INFO : MESH;
    snprintf(m.label, sizeof(m.label), "%s", t.name);
  }
  for (size_t i = log.size(); i-- > 0 && nm < 48;) {   // positions from messages, newest first (web notePos)
    const chat::Entry &e = log.at(i); double la, lo;
    if (!chat::findPos(e.text, la, lo)) continue;
    devmap::Mark &m = marks[nm++]; m.lat = la; m.lon = lo; m.color = WINE;
    const char *body; char who[34]; authorName(e, who, sizeof(who), &body);
    if (e.conv[0] == 'd' && !e.own) { dev::Target c; if (dev::findTarget(*api, e.conv, c)) snprintf(who, sizeof(who), "%s", c.name); }
    snprintf(m.label, sizeof(m.label), "%s", who);
  }
  devmap::Mark me; bool haveMe = false;
  dev::Gps g = api->gps();
  if (g.receiverOn && g.fresh) { me.lat = g.lat; me.lon = g.lon; me.color = TEXT; snprintf(me.label, sizeof(me.label), "Ich"); haveMe = true; }
  map.render(d, marks, nm, haveMe ? &me : nullptr);
  d->nativeFill(0, NAV_H, W, 36, NAV_BG);
  ui::text(d, 4, NAV_H + 6, MESH, map.map().name, 32);
  ui::button(d, 228, NAV_H + 4, 88, 32, "Karte >", false);
  const char *controls[] = { "- Zoom", "Mein GPS", "Zoom +" };
  for (int i = 0; i < 3; ++i) {
    auto r = touchui::mapControl(i);
    ui::button(d, r.x, r.y, r.w, r.h, controls[i], false);
  }
  if (status[0])
    ui::text(d, 4, 182, PEND, status);
  else if (!haveMe)
    ui::text(d, 4, 182, MUTED, "Kein GPS-Fix / Kartenansicht");
}

void UI::drawLocation(DisplayDriver *d) {
  dev::Gps g = api->gps();
  char line[96];
  snprintf(line, sizeof(line), "GPS: %s / %ld Sat",
           !g.detected     ? "nicht erkannt"
           : !g.receiverOn ? "aus"
           : g.fresh       ? "Fix"
                           : "sucht",
           g.sats);
  ui::text(d, 4, 68, g.fresh && g.receiverOn ? MESH : PEND, line);
  if (g.receiverOn && g.fresh)
    snprintf(line, sizeof(line), "%.5f, %.5f", g.lat, g.lon);
  else
    snprintf(line, sizeof(line), "Keine frische Position");
  ui::text(d, 4, 80, MUTED, line);
  for (int i = 0; i < 2; ++i) {
    int y = 94 + i * 38;
    bool v = i == 0 ? g.receiverOn : g.share;
    ui::button(d, 4, y, 312, 34, "", locSel == i);
    ui::text(d, 12, y + 9, TEXT, i == 0 ? "GPS-Empfaenger" : "Standort teilen", 19, 2);
    d->nativeFill(256, y + 6, 52, 22, v ? MESH : BG);
    ui::centerText(d, 282, y + 13, v ? INK : MUTED, v ? "AN" : "AUS");
    addHit(4, y, 312, 34, i);
  }
  ui::button(d, 4, 172, 154, 38, "Position senden", locSel == 2, g.share && g.fresh && g.receiverOn);
  ui::button(d, 162, 172, 154, 38, "Bekanntgabe", locSel == 3);
  addHit(4, 172, 154, 38, 2);
  addHit(162, 172, 154, 38, 3);
  dev::Target to;
  const char *id = conv[0] && !convRoom ? conv : "c0";
  snprintf(line, sizeof(line), "Ziel: %s", dev::findTarget(*api, id, to) ? to.name : id);
  ui::text(d, 4, 216, MUTED, line);
  if (status[0])
    ui::hint(d, status, PEND);
  else
    ui::hint(d,
             g.share ? "GPS wird nur mit frischem Fix geteilt" : "Freigabe AUS / keine GPS-Position senden");
}

void UI::drawDevice(DisplayDriver *d) {
  const char *buttons[DEVICE_BUTTONS] = { "Kontakte neu lesen", "Bekanntgabe senden",
                                          "Display aus",        "Auf SD speichern",
                                          "Touch kalibrieren",  "Standort / GPS / Teilen",
                                          "Geraeteinfo / Akku" };
  for (int row = 0; row < LIST_ROWS && devTop + row < DEVICE_BUTTONS; ++row) {
    int i = devTop + row;
    auto r = touchui::row(row);
    ui::button(d, r.x, r.y, r.w, r.h, "", i == devSel);
    ui::centerText(d,r.x+r.w/2,r.y+9,TEXT,buttons[i],2);
    addHit(r.x, r.y, r.w, r.h, i);
  }
  char note[64];
  snprintf(note, sizeof(note), "%d-%d / %d  |  Wischen: weitere Einstellungen", devTop + 1,
           devTop + LIST_ROWS, DEVICE_BUTTONS);
  ui::text(d, 4, 222, MUTED, note);
  drawHint(d);
}

void UI::drawDeviceInfo(DisplayDriver *d) {
  dev::Info in; api->info(in);
  int y = NAV_H + 3;
  char v[96], when[16];
  auto row = [&](const char *k, const char *val, uint16_t c) {
    ui::text(d, 4, y, MUTED, k, 14);
    ui::text(d, 92, y, c, val, 37);
    y += 13;
  };
  row("Name", in.name[0] ? in.name : "\xe2\x80\x93", TEXT);
  row("Firmware", in.firmware, TEXT);
  snprintf(v, sizeof(v), "%s \xc2\xb7 %s", in.bleConnected ? "App verbunden" : "nicht verbunden", in.defaultPin ? "Standard-PIN" : "eigene PIN");
  row("Bluetooth", v, in.bleConnected ? MESH : TEXT);
  snprintf(v, sizeof(v), "%u Kan\xc3\xa4le \xc2\xb7 %u Kontakte", in.channels, in.contacts);
  row("Netz", v, TEXT);
  if (in.historyOK) snprintf(v, sizeof(v), "letzte Nr. %u (96 im Ger\xc3\xa4t)", in.history); else snprintf(v, sizeof(v), "FEHLER \xe2\x80\x93 wird nicht gespeichert");
  row("Verlauf", v, in.historyOK ? TEXT : BAD);
  stampText(api->now(), when, sizeof(when));
  row("Uhr", in.clockOK ? when : "nicht gestellt (App/GPS)", in.clockOK ? TEXT : PEND);
  snprintf(v, sizeof(v), "TX %lu/%lu \xc2\xb7 RX %lu/%lu (Flood/Direkt)", (unsigned long)in.txFlood, (unsigned long)in.txDirect,
           (unsigned long)in.rxFlood, (unsigned long)in.rxDirect);
  row("Funk", v, TEXT);
  snprintf(v, sizeof(v), "%lu KB frei \xc2\xb7 min. %lu KB", (unsigned long)(in.heapFree / 1024), (unsigned long)(in.heapMin / 1024));
  row("Speicher", v, in.heapMin && in.heapMin < 40 * 1024 ? BAD : TEXT);
  if (in.sdCard) snprintf(v, sizeof(v), "Config %s \xc2\xb7 Log %s%s%s", in.sdConfig ? "ja" : "nein", in.sdLog ? "an" : "aus",
                          in.sdNote[0] ? " \xc2\xb7 " : "", in.sdNote);
  else snprintf(v, sizeof(v), "%s", in.sdNote[0] ? in.sdNote : "keine Karte");
  row("SD-Karte", v, in.sdCard ? TEXT : MUTED);
  row("Akku", battLine[0] ? battLine : "\xe2\x80\x93", battPct >= 0 && battPct < 15 ? BAD : TEXT);
  const char *est = !strncmp(battEst, "Verbrauch: ", 11) ? battEst + 11 : battEst;   // the row is labelled already
  row("Verbrauch", est[0] ? est : "noch keine Sch\xc3\xa4tzung", est[0] ? TEXT : MUTED);
  ui::text(d, 4, 216, BAD, "Kein Notrufsystem / Lebensgefahr: 112");
  drawHint(d);
}

void UI::render(DisplayDriver *d) {
  ++touchRevision;
  dirty = NONE;
  if (page == BOOT) { drawBoot(d); return; }
#ifdef SUI_HAS_QUICK
  if (page == QUICK) { quick.render(d); return; }
#endif
  d->startFrame(BG);
  nhits = 0;
  bool inConv = page == CHAT_VIEW || page == BOARD_VIEW;
  ui::statusLine(d, title(page), page != MENU, inConv ? 0 : unreadChat() + unreadBoard());
  switch (page) {
    case MENU: drawMenu(d); drawHint(d); break;
    case CHAT_LIST: case BOARD_LIST: drawList(d, page); drawHint(d); break;
    case CHAT_VIEW: case BOARD_VIEW: drawConversation(d); break;
    case MAP: drawMap(d); break;
    case LOCATION: drawLocation(d); break;
    case DEVICE: drawDevice(d); break;
    case DEVICE_INFO:
      drawDeviceInfo(d);
      break;
    default: break;
  }
  d->endFrame();
}

void UI::renderPart(DisplayDriver *d) {
  if (dirty == FULL || (page != CHAT_VIEW && page != BOARD_VIEW)) {
    render(d);
    return;
  }
  ++touchRevision;
  dirty = NONE;
  if (page == CHAT_VIEW || page == BOARD_VIEW) drawCompose(d); else drawHint(d);
  d->endFrame();
}
}
#endif
