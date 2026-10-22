// Host test of the T-Deck Plus standalone pages (Issue #35): boot logo -> main menu, trackball/touch selection
// inside pages, ALT+Backspace (K_BACK) one level back everywhere, chat, unread, Schnellmeldung, Brett, map with
// "Zurück", Standort, Gerät (battery, advert, display off); every page stays inside 320 x 240.
// g++ -std=c++11 -Wall -Wextra -Wpedantic -Werror -fsanitize=address,undefined -DLILYGO_TDECK -DDISASTER_STANDALONE
//     -I src -I tests/ui_fakes tests/standalone_ui.test.cpp examples/companion_radio/ui-brand/StandaloneUI.cpp
//     examples/companion_radio/ui-brand/TDeckQuickPage.cpp
#include <assert.h>
#include <stdio.h>
#include <string>
#include "FakeApi.h"
#include "FakeDisplay.h"
#include "../examples/companion_radio/ui-brand/StandaloneUI.h"
#include "../examples/companion_radio/ui-brand/Widgets.h"

static FakeApi fake;
dev::Api &dev::api() { return fake; }
static uint32_t ms = 1000;
static void step(sui::UI &u, FakeDisplay &d) { ms += 300; u.tick(ms); if (u.pending() == sui::UI::PART) u.renderPart(&d); else if (u.pending()) u.render(&d); }
static void type(sui::UI &u, const char *s) { while (*s) u.key(uint8_t(*s++)); }

static void testTheme() {
  char out[64];
  theme::toCp437("\xc3\xa4\xc3\xb6\xc3\xbc\xc3\x9f \xe2\x80\x93 \xe2\x80\xa6 \xe2\x86\x92 \xf0\x9f\x98\x80", out, sizeof(out));
  assert(!strcmp(out, "\x84\x94\x81\xe1 - ... \x1a ?"));
  theme::toCp437("abcdef", out, sizeof(out), 3); assert(!strcmp(out, "abc"));
  theme::toCp437("\xc3\xa4\xc3\xa4", out, 2); assert(!strcmp(out, "\x84"));      // never splits; cap respected
  char t[20];
  theme::stampText(1782900000u, t, sizeof(t)); assert(!strcmp(t, "01.07. 12:00"));    // CEST: UTC+2
  theme::stampText(1767268800u, t, sizeof(t)); assert(!strcmp(t, "01.01. 13:00"));    // CET: UTC+1
  theme::stampText(5, t, sizeof(t)); assert(!strcmp(t, "--.--. --:--"));
  int lines = theme::countLines("eins zwei drei vier fuenf sechs", 10); assert(lines == 4);
}

static void testMap() {
  devmap::View v; v.select(0);
  double x0 = -5, y0 = 100, x1 = 400, y1 = 100;
  assert(devmap::View::clip(x0, y0, x1, y1) && x0 == 0 && x1 == 319);
  x0 = -5; y0 = 5; x1 = -1; y1 = 300; assert(!devmap::View::clip(x0, y0, x1, y1));
  assert(v.inside(51.5279, 11.5480));                 // Mansfelder-Land overview contains Lutherstadt Eisleben
  // View-only fallback: the village centre must project to the screen centre.
  v.centerOn(51.527900, 11.548000);
  double ux, uy, px, py;
  v.toUnits(51.527900, 11.548000, ux, uy); v.project(ux, uy, px, py);
  assert(fabs(px - 160) < 0.01 && fabs(py - (devmap::View::TOP + devmap::View::HEIGHT / 2.0)) < 0.01);
  v.select(0);
  assert(!v.inside(52.52, 13.40));               // Berlin is not on the map
  double before = v.ppd(); v.zoom(2); assert(v.ppd() > 1.9 * before);
  v.zoom(1000); assert(v.zoomFactor() <= 32.0001);
  v.centerOn(51.5279, 11.5480); assert(v.ppd() >= 29999); assert(v.inside(51.5279, 11.5480));
  FakeDisplay d; d.startFrame(0);
  for (int i = 0; i < devmap::MAP_COUNT; ++i) { v.select(i); unsigned s = v.render(&d, nullptr, 0, nullptr); assert(s > 50); }
  assert(d.outOfBounds == 0);
  assert(d.allText.find("OpenStreetMap") != std::string::npos);
}


// Back to the main menu (K_BACK = ALT+Backspace), select entry i with the trackball, open it with a click.
static void openEntry(sui::UI &u, FakeDisplay &d, int i) {
  while (u.key(sui::K_BACK)) {}
  assert(u.current() == sui::MENU);
  for (int k = 0; k < 6; ++k) u.move(0, -1);
  for (int k = 0; k < i; ++k) u.move(0, 1);
  assert(u.menuSelection() == i);
  u.render(&d); assert(d.outOfBounds == 0);
  assert(u.click());
}
enum { M_CHAT, M_QUICK, M_BOARD, M_MAP, M_LOC, M_DEV };

int main() {
  testTheme();
  testMap();
  fake.demo();
  sui::UI u; FakeDisplay d;
  static const uint16_t logo[40 * 20] = {0};
  u.setLogo(logo, 40, 20);
  assert(u.begin(&fake));
  u.tick(ms);
  assert(u.history().size() == 8);              // 0x82 is no line; it marks the f4 delivered
  assert(u.unreadChat() == 0 && u.unreadBoard() == 0);   // everything before start counts as read
  assert(!u.takeIncoming());

  // Boot logo: brand image + version + 112 line, then the main menu after BOOT_MS
  assert(u.onBoot());
  u.render(&d); assert(d.outOfBounds == 0);
  assert(d.allText.find("standalone-test") != std::string::npos && d.allText.find("112") != std::string::npos);
  ms += sui::UI::BOOT_MS - 100; u.tick(ms); assert(u.onBoot());
  ms += 200; u.tick(ms); assert(u.current() == sui::CHAT_LIST);
  u.key(sui::K_BACK);
  {   // the first key / trackball step / click / tap only leaves the logo (nothing typed, nothing opened)
    sui::UI b; b.begin(&fake); b.tick(ms);
    assert(b.onBoot() && b.key('x') && b.current() == sui::CHAT_LIST && !b.draft()[0]);
    sui::UI c; c.begin(&fake); c.tick(ms); assert(c.move(0, 1) && c.current() == sui::CHAT_LIST && c.menuSelection() == 0);
    sui::UI e; e.begin(&fake); e.tick(ms); assert(e.click() && e.current() == sui::CHAT_LIST);
    sui::UI f; f.begin(&fake); f.tick(ms); assert(f.tap(200, 120) && f.current() == sui::CHAT_LIST);
  }
  // main menu: six entries, status line with title and battery, no tab bar, no letter shortcuts
  u.setBattery(87, "~87 % \xc2\xb7 4,05 V", "ca. 18 h Restlaufzeit");
  assert(ui::navBattery() == 87);
  u.render(&d); assert(d.outOfBounds == 0);
  const char *entries[6] = {"Chat", "Meldung", "Brett", "Karte", "Standort", "Ger\x84t"};
  for (int i = 0; i < 6; ++i) assert(d.allText.find(entries[i]) != std::string::npos);
  assert(d.allText.find("87%") != std::string::npos && d.allText.find("Men") != std::string::npos);
  const char *letters = "cmbksgh";
  for (const char *l = letters; *l; ++l) { u.key(uint8_t(*l)); assert(u.current() == sui::MENU); }
  assert(!u.key(sui::K_BACK) && u.current() == sui::MENU);           // the menu is the top level
  u.move(1, 0); assert(u.menuSelection() == 1);                       // left/right also move the selection
  u.move(0, -1); u.move(0, -3); assert(u.menuSelection() == 0);
  for (int i = 0; i < 8; ++i) u.move(0, 1);
  assert(u.menuSelection() == 5 && u.current() == sui::MENU);         // clamps, never leaves the menu
  // touch: tap the "Brett" row (row 2), the status line goes back
  u.render(&d);
  assert(u.tap(100, touchui::tile(2).y + 10) && u.current() == sui::BOARD_LIST);
  assert(u.tap(40, theme::TAB_H + 5) && u.current() == sui::MENU);
  assert(!u.tap(theme::W - 10, theme::TAB_H + 5));                                   // battery field is no back button
  u.render(&d); assert(u.tap(100, theme::NAV_H + 12) && u.current() == sui::CHAT_LIST);   // "Chat" row
  u.render(&d); assert(d.outOfBounds == 0);
  assert(d.allText.find("Mansfelder-Hilfe") != std::string::npos);

  // chat list -> conversation; letters are text there, Enter sends, own line appears
  assert(u.click() && u.current() == sui::CHAT_VIEW && !strcmp(u.conversation(), "c0"));
  u.render(&d); assert(d.outOfBounds == 0);
  assert(d.allText.find("Apotheke am Markt") != std::string::npos);
  type(u, "bin unterwegs, komme in 10 min");
  assert(u.current() == sui::CHAT_VIEW && !strcmp(u.draft(), "bin unterwegs, komme in 10 min"));
  assert(u.pending() == sui::UI::PART); u.renderPart(&d);
  u.key(sui::K_BACKSPACE); assert(!strcmp(u.draft(), "bin unterwegs, komme in 10 mi"));   // plain Backspace deletes
  assert(u.move(1, 0) == false && u.current() == sui::CHAT_VIEW);    // trackball never leaves the page
  u.key(sui::K_ENTER);
  assert(fake.sends == 1 && fake.lastId == "c0" && fake.lastText == "bin unterwegs, komme in 10 mi" && !u.draft()[0]);
  step(u, d);
  assert(u.history().size() == 9 && u.history().at(8).own);
  assert(d.allText.find("Gesendet") != std::string::npos);
  // 100-byte limit
  for (int i = 0; i < 120; ++i) u.key('x');
  assert(strlen(u.draft()) == 100);
  // ALT+Backspace leaves even a filled text field (draft stays with its conversation)
  u.key(sui::K_BACK); assert(u.current() == sui::CHAT_LIST);
  u.click(); assert(u.current() == sui::CHAT_VIEW && strlen(u.draft()) == 100);
  while (u.draft()[0]) u.key(sui::K_BACKSPACE);
  // empty Enter does not send; tap on the compose box = Enter
  u.key(sui::K_ENTER); assert(fake.sends == 1);
  type(u, "per Tipp"); u.render(&d); assert(u.tap(280, 210) && fake.sends == 2 && fake.lastText == "per Tipp");
  type(u, "draft across tabs"); u.render(&d);
  assert(!u.tap(100,210) && fake.sends == 2);
  assert(!u.gesture(280,210,240,210) && fake.sends == 2);
  assert(!u.gesture(10,10,80,10) && u.current() == sui::CHAT_VIEW);
  assert(u.tap(90,10) && u.current() == sui::QUICK);
  assert(u.tap(20,10) && u.current() == sui::CHAT_LIST);
  u.click(); assert(!strcmp(u.draft(), "draft across tabs"));
  assert(u.gesture(100,100,100,150) && fake.sends == 2);
  while (u.draft()[0]) u.key(sui::K_BACKSPACE);
  // Backspace on an empty draft goes back to the list (fallback without ALT detection)
  u.key(sui::K_BACKSPACE); assert(u.current() == sui::CHAT_LIST);

  // unread indicator and wake-up
  fake.channelIn(1, "Lea: Pumpe kommt um 15 Uhr", fake.clock);
  step(u, d);
  assert(u.takeIncoming() && !u.takeIncoming());
  assert(u.unreadChat() == 1);
  u.render(&d); assert(d.allText.find("Pumpe") != std::string::npos && d.allText.find("1 neu") != std::string::npos);
  u.move(0, 1); u.click(); assert(!strcmp(u.conversation(), "c1"));
  assert(u.unreadChat() == 0);
  u.key(sui::K_BACK); assert(u.current() == sui::CHAT_LIST);
  // direct chat by touch: tap the third row
  u.render(&d);
  assert(u.tap(100, touchui::row(2).y + 5) && !strcmp(u.conversation(), "d0a1b2c3d4e5f"));
  u.render(&d); assert(d.allText.find("zugestellt") != std::string::npos);
  u.key(sui::K_BACK); u.key(sui::K_BACK); assert(u.current() == sui::MENU);

  // Brett: join with an empty Enter, post with text
  openEntry(u, d, M_BOARD); assert(u.current() == sui::BOARD_LIST);
  u.render(&d); assert(d.outOfBounds == 0 && d.allText.find("Brett Eisleben") != std::string::npos);
  u.click(); assert(u.current() == sui::BOARD_VIEW);
  u.render(&d); assert(d.allText.find("Bautrockner") != std::string::npos);
  assert(d.allText.find("Anna (Eisleben)") != std::string::npos);     // author resolved by key prefix
  u.key(sui::K_ENTER); assert(fake.logins == 1 && !strcmp(fake.rm.id, "dcafe0012ab34"));
  fake.rm.pending = false; fake.rm.rights = 2; step(u, d);
  assert(d.allText.find("beigetreten") != std::string::npos);
  type(u, "Biete Kaffee"); u.click();
  assert(fake.sends == 3 && fake.lastId == "dcafe0012ab34" && fake.lastText == "Biete Kaffee");
  fake.roomIn("dcafe0012ab34", "0a1b2c3d", "Danke!", fake.clock);
  u.key(sui::K_BACK); step(u, d);
  assert(u.current() == sui::BOARD_LIST && u.unreadBoard() == 1);

  // map: "Zurück" button top left (tap, trackball click, ALT+Backspace) back to the menu
  openEntry(u, d, M_MAP); assert(u.current() == sui::MAP && u.mapReturnPage() == sui::MENU);
  double fallbackX, fallbackY, fallbackPX, fallbackPY;
  u.mapView().toUnits(51.527900,11.548000,fallbackX,fallbackY);
  u.mapView().project(fallbackX,fallbackY,fallbackPX,fallbackPY);
  assert(fabs(fallbackPX-160)<0.01 && fabs(fallbackPY-(devmap::View::TOP+devmap::View::HEIGHT/2.0))<0.01);
  assert(!fake.g.fresh && fake.g.lat==0 && fake.g.lon==0);
  u.render(&d); assert(d.outOfBounds == 0);
  assert(d.allText.find("< Zurueck") != std::string::npos);
  assert(d.fb[(theme::TAB_H + 2) * 320 + 2] == theme::LINE); // drawn focused: the click target
  for (int i = 0; i < 6; ++i) assert(u.move(-1, 0) && u.current() == sui::MAP);   // trackball pans, never leaves
  u.move(0, -3); assert(u.current() == sui::MAP);
  u.key('+'); assert(u.mapView().zoomFactor() > 1.9);
  u.move(1, 1); u.key('a'); assert(u.mapView().zoomFactor() < 1.01);
  u.key('n'); assert(fake.map == 1 && u.mapView().index() == 1);
  u.render(&d); assert(d.outOfBounds == 0);
  u.key('i'); assert(std::string(u.statusText()).find("GPS") != std::string::npos);   // no fix yet
  for (int i = 0; i < 4; ++i) u.key('n');
  assert(fake.map == 0);
  assert(u.click() && u.current() == sui::MENU);                      // trackball click = Zurück
  u.click(); u.render(&d); assert(u.current() == sui::MAP);           // menu selection stayed on "Karte"
  assert(!u.tap(200, 120) && u.current() == sui::MAP);                // a tap on the map itself does nothing
  assert(u.tap(sui::UI::BACK_X + 10, theme::TAB_H + 5) && u.current() == sui::MENU); // touch on Zurück
  u.click(); u.key(sui::K_BACK); assert(u.current() == sui::MENU);    // ALT+Backspace
  u.click(); u.key(sui::K_BACKSPACE); assert(u.current() == sui::MENU);   // plain Backspace (no text field)

  // Map touch controls and GPS following are local only, never enable sharing.
  bool shareBefore = fake.g.share;
  fake.g.fresh=true; fake.g.lat=51.5279; fake.g.lon=11.5480;
  openEntry(u,d,M_MAP); u.render(&d);
  assert(u.mapView().ppd() >= 29999);
  assert(u.tap(20,215));
  double zoomBefore=u.mapView().zoomFactor();
  assert(u.tap(280,215) && u.mapView().zoomFactor()>zoomBefore);
  assert(u.tap(20,215));
  assert(u.tap(160,215) && fake.g.share == shareBefore);
  u.move(1,0); double cx,cy; u.mapView().project(0,0,cx,cy);
  fake.g.lon+=0.001; step(u,d); double nx,ny; u.mapView().project(0,0,nx,ny);
  assert(cx == nx && cy == ny); // manual pan pauses following
  u.tap(160,215); fake.g.lon+=0.001; step(u,d);
  u.mapView().project(0,0,nx,ny); assert(cx != nx);
  fake.g.fresh=false;
  // Standort: share off by default; position only with share + fresh fix
  openEntry(u, d, M_LOC); assert(u.current() == sui::LOCATION);
  u.render(&d); assert(d.outOfBounds == 0 && d.allText.find("Standort teilen") != std::string::npos);
  u.move(0, 1); u.move(0, 1); u.click();
  assert(fake.sends == 3 && std::string(u.statusText()).find("Standort teilen") != std::string::npos);
  u.move(0, -1); u.click(); assert(fake.g.share);
  u.move(0, 1); u.click(); assert(fake.sends == 3);           // no fresh fix
  fake.g.fresh = true; fake.g.lat = 51.52791; fake.g.lon = 11.54799;
  u.click(); assert(fake.sends == 4 && fake.lastText.compare(0, 23, "!DP1|51.5279,11.5480|179") == 0);
  assert(fake.lastId == "c0");                                // last conversation was a Brett: default channel
  u.move(0, 1); u.click(); assert(fake.adverts == 1);
  u.move(0, -1); u.move(0, -1); u.move(0, -1); u.click(); assert(!fake.g.receiverOn);
  u.render(&d); assert(d.outOfBounds == 0);
  // touch on the first row (receiver toggle)
  { bool on = fake.g.receiverOn; u.move(0, 3); u.render(&d);
    assert(u.tap(100, 98) && fake.g.receiverOn != on); }            // row 0 at y 65..84

  // Gerät
  openEntry(u, d, M_DEV); assert(u.current() == sui::DEVICE);
  u.render(&d); assert(d.outOfBounds == 0);
  assert(d.allText.find("Bekanntgabe senden") != std::string::npos &&
         d.allText.find("Display aus") != std::string::npos);
  for (int i = 0; i < 6; ++i)
    u.move(0, 1);
  u.click();
  u.render(&d);
  assert(u.current() == sui::DEVICE_INFO && d.allText.find("112") != std::string::npos);
  assert(d.allText.find("Standard-PIN") != std::string::npos &&
         d.allText.find("123456") == std::string::npos);
  assert(d.allText.find("~87 %") != std::string::npos && d.allText.find("18 h") != std::string::npos);
  u.back();
  for (int i = 0; i < 7; ++i)
    u.move(0, -1);
  u.click(); assert(fake.refreshes == 1);
  int adv = fake.adverts;
  u.move(0, 1); u.click(); assert(fake.adverts == adv + 1);            // Advert senden
  u.move(0, 1); assert(!u.takeDisplayOff()); u.click(); assert(u.takeDisplayOff() && !u.takeDisplayOff());   // Display aus
  u.move(0, 1); u.click(); assert(fake.saves == 1);
  u.key('d'); assert(u.takeDisplayOff() && u.current() == sui::DEVICE);   // key 'd' outside text fields
  { int r = fake.refreshes; u.render(&d);
    assert(u.tap(100, touchui::row(0).y + 10) && fake.refreshes == r + 1);
  } // "Kontakte neu lesen" at y 131..147

  // Schnellmeldung: preset, letters into the text, Enter path to "Meldung senden"
  fake.g.fresh = true;                        // receiver was switched off/on above: wait for a new fix
  openEntry(u, d, M_QUICK); assert(u.current() == sui::QUICK);
  u.render(&d); assert(d.outOfBounds == 0 && d.allText.find("Meldung") != std::string::npos);
  u.key('1'); assert(u.draft()[0] == 0);     // the quick page has its own draft
  type(u, " bitte");                          // text focus: letters stay text
  assert(u.current() == sui::QUICK);
  u.move(0, 1); u.move(0, 1); u.move(0, 1);  // PlaceSelect, Target, Send
  u.click();
  assert(fake.sends == 5 && fake.lastId == "c0");
  assert(fake.lastText.find("bitte") != std::string::npos && fake.lastText.find("51.5279") != std::string::npos);
  u.render(&d); assert(d.outOfBounds == 0);
  u.key(sui::K_BACK); assert(u.current() == sui::MENU);               // ALT+Backspace, also from the text field
  openEntry(u, d, M_QUICK); u.render(&d);
  assert(u.tap(20, 80) && u.quickPage().picking());
  u.render(&d);
  assert(u.tap(20, touchui::row(0).y + 10) && u.quickPage().draft().focus == quick::Text); // tap on preset 1
  type(u, "x"); assert(u.current() == sui::QUICK);
  u.key(sui::K_BACK); assert(u.current() == sui::MENU);
  openEntry(u, d, M_QUICK); u.render(&d); assert(u.tap(30, theme::TAB_H + 5) && u.current() == sui::MENU);   // status line

  // not ready: lists say so
  fake.isReady = false; openEntry(u, d, M_CHAT); u.render(&d); assert(d.allText.find("Funkkern startet") != std::string::npos);
  // status line with unread, focus frame and an unknown / low battery
  ui::navBattery() = -1; d.startFrame(0); ui::statusLine(&d, "Ger\xc3\xa4t", true, 123, true); assert(d.outOfBounds == 0);
  assert(d.allText.find("--%") != std::string::npos && d.allText.find("99 neu") != std::string::npos);
  ui::navBattery() = 7; d.startFrame(0); ui::statusLine(&d, "Karte", true, 0); assert(d.allText.find("7%") != std::string::npos);
  puts("standalone_ui: ok");
  return 0;
}
