#pragma once
// Native operation of the LilyGO T-Deck Plus standalone build (Issue #35): the pages of the web UI
// (web/index.html: Chat, Schnellmeldung, Brett, Karte, Standort, Gerät) on 320 x 240 with keyboard, trackball and
// touch. HaldenFunk standalone appearance: full boot image, CHAT/HILFE/BRETT/KARTE/SETUP tabs,
// explicit SEND button and swipe scrolling. MeshCore adds back/unread/battery below the tabs.
// ALT+Backspace keeps the hierarchical back path and opens the auxiliary six-entry menu.
// Hardware-free: everything goes through dev::Api (DeviceApi.h), so host tests and the preview harness
// (tools/standalone_preview.py) run the same code as the firmware.
#include "ChatLog.h"
#include "DeviceApi.h"
#include "MapView.h"
#include "Theme.h"
#include "TouchLayout.h"

#include <helpers/ui/DisplayDriver.h>
#if defined(LILYGO_TDECK) && defined(DISASTER_STANDALONE)
#include "TDeckQuickPage.h"
#define SUI_HAS_QUICK 1
#endif

namespace sui {
// Key codes of the T-Deck keyboard (character mode, I2C 0x55) used here.
constexpr uint8_t K_ENTER = 0x0D, K_BACKSPACE = 0x08, K_ALT_C = 0x0C, K_ESC = 0x1B;
// "One level back": UITask turns ALT+Backspace into this code (the keyboard itself sends 0x08 for both); Esc too.
constexpr uint8_t K_BACK = K_ESC;
enum Page : uint8_t {
  BOOT,
  MENU,
  CHAT_LIST,
  CHAT_VIEW,
  QUICK,
  BOARD_LIST,
  BOARD_VIEW,
  MAP,
  LOCATION,
  DEVICE,
  DEVICE_INFO,
  PAGE_COUNT
};
constexpr size_t COMPOSE_BYTES = 100;   // disaster::LOCAL_TEXT_LIMIT
constexpr unsigned MAX_LIST = 72;       // 8 channels + 64 contacts
constexpr int MENU_COUNT = 6;           // Chat, Meldung, Brett, Karte, Standort, Gerät

// Map keys of MapData.h ("mansfelder-land", ...) for the SD settings file; -1 / nullptr if unknown.
int mapIndexForKey(const char *key);
const char *mapKey(int index);

class UI {
public:
  enum Redraw : uint8_t { NONE = 0, PART = 1, FULL = 2 };
private:
  dev::Api *api = nullptr;
  chat::Log log;
  devmap::View map;
  Page page = BOOT;
  bool mapFollow = true;
  double followLat = 0, followLon = 0;
  Page mapReturn = MENU;             // page that the map's "Zurück" button returns to
  int menuSel = 0;
  // touch targets of the drawn page (index = list row / menu entry / button)
  struct Hit {
    int16_t x, y, w, h, idx;
    char id[14];
  };
  static constexpr int MAX_HITS = 16;
  Hit hits[MAX_HITS];
  int nhits = 0;
  void addHit(int x, int y, int w, int h, int idx);
  uint32_t bootUntil = 0, nowMs = 0;
  const uint8_t *splash = nullptr;
  const uint16_t *logo = nullptr; int logoW = 0, logoH = 0;
  int battPct = -1; char battLine[48] = "", battEst[48] = "";
  bool displayOffReq = false;
  bool touchCalibrationReq = false;
  int listSel[PAGE_COUNT] = {0}, listTop[PAGE_COUNT] = {0};
  int locSel = 0, devSel = 0, devTop = 0, scroll = 0;
  uint32_t touchRevision = 0;
  int touchTarget(int x, int y) const;
  bool swipeList(int dy);
  char conv[14] = "c0", convName[40] = "";
  bool convRoom = false;
  char compose[COMPOSE_BYTES + 1] = "";
  char status[96] = "";
  uint32_t statusUntil = 0, lastIncoming = 0, wakeSeq = 0, nextPoll = 0, lastTargetSig = 0;
  Redraw dirty = FULL;
#ifdef SUI_HAS_QUICK
  TDeckQuickPage quick;
#endif
  unsigned buildList(Page p, dev::Target *out, unsigned cap) const;
  void open(Page p);
  void openConversation(const dev::Target &t, bool room);
  void setStatus(const char *s, uint32_t ms = 6000);
  bool typing() const { return page == CHAT_VIEW || page == BOARD_VIEW || (page == QUICK && quickText()); }
  bool quickText() const;
  void sendCompose();
  void sendPosition();
  void leaveBoot() { open(CHAT_LIST); }
  void leaveMap() { open(mapReturn); }
  void requestDisplayOff() { displayOffReq = true; }
  bool listMove(Page p, int dy, unsigned n);
  // drawing
  void drawList(DisplayDriver *d, Page p);
  void drawConversation(DisplayDriver *d);
  void drawCompose(DisplayDriver *d);
  void drawMap(DisplayDriver *d);
  void drawLocation(DisplayDriver *d);
  void drawDevice(DisplayDriver *d);
  void drawDeviceInfo(DisplayDriver *d);
  void drawHint(DisplayDriver *d);
  void drawBoot(DisplayDriver *d);
  void drawMenu(DisplayDriver *d);
  const char *authorName(const chat::Entry &e, char *buf, size_t cap, const char **body) const;
public:
  static constexpr uint32_t BOOT_MS = 1800;                // original standalone timing, then channel/contact list
  // Shared back button below the tabs on every page.
  static constexpr int BACK_X = 2, BACK_Y = theme::TAB_H + 2, BACK_W = 76, BACK_H = 24;
  bool begin(dev::Api *a);
  // Brand image of the boot logo (RGB565, BrandLogo.h in the firmware); without one only text is shown.
  void setSplash(const uint8_t *pixels) { splash = pixels; }
  void setLogo(const uint16_t *img, int w, int h) { logo = img; logoW = w; logoH = h; }
  // Battery reading of UITask: level for the tab strip, texts (level/voltage, consumption) for Gerät.
  void setBattery(int pct, const char *line, const char *estimate);
  Page current() const { return page; }
  bool onBoot() const { return page == BOOT; }
  // Input. Each returns true if the page must be redrawn.
  bool key(uint8_t k);
  bool move(int dx, int dy);
  // Trackball click: Enter on every page; on the map it is the "Zurück" button; on the boot logo it opens the chat list.
  bool click();
  // Touch tap at screen coordinates: status line = back; menu entries, list rows, buttons, SEND, the map's
  // "Zurück" (targets of the last render); leaves the boot logo.
  bool tap(int x, int y);
  bool gesture(int x, int y, int endX, int endY, bool travelled = false, uint32_t duration = 200);
  uint32_t touchEpoch() const { return touchRevision; }
  // One level back (ALT+Backspace, Esc, status line tap): conversation -> list -> menu, map -> where it was opened.
  bool back();
  // true once after "Display aus" (Gerät button or key 'd'): the caller switches the display off.
  bool takeDisplayOff() { bool r = displayOffReq; displayOffReq = false; return r; }
  bool takeTouchCalibration() { bool r=touchCalibrationReq; touchCalibrationReq=false; return r; }
  // Poll the history / status; returns true if something visible changed.
  bool tick(uint32_t ms);
  // true once per new incoming message (display wake-up).
  bool takeIncoming();
  unsigned unreadChat() const;
  unsigned unreadBoard() const;
  Redraw pending() const { return dirty; }
  void render(DisplayDriver *d);              // whole page (boot logo included)
  void renderPart(DisplayDriver *d);          // only the compose box / status line
  void forceRedraw() { dirty = FULL; }
  void notify(const char *s) { setStatus(s, 4000); if (dirty < PART) dirty = PART; }
  // for tests
  const char *conversation() const { return conv; }
  const char *draft() const { return compose; }
  const char *statusText() const { return status; }
  Page mapReturnPage() const { return mapReturn; }
  int menuSelection() const { return menuSel; }
  int battery() const { return battPct; }
  const devmap::View &mapView() const { return map; }
  const chat::Log &history() const { return log; }
#ifdef SUI_HAS_QUICK
  const TDeckQuickPage &quickPage() const { return quick; }
#endif
};
}
