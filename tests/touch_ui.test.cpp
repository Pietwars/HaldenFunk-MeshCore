#include "../examples/companion_radio/ui-brand/StandaloneUI.h"
#include "FakeApi.h"
#include "FakeDisplay.h"

#include <algorithm>
#include <assert.h>
#include <string>

static FakeApi api;
dev::Api &dev::api() {
  return ::api;
}
static FakeDisplay screen;
static void draw(sui::UI &u) {
  u.render(&screen);
  assert(screen.outOfBounds == 0);
}
static bool tap(sui::UI &u, int x, int y) {
  return u.gesture(x, y, x, y);
}
static void start(sui::UI &u) {
  assert(u.begin(&api));
  u.key('x');
  draw(u);
}
static void type(sui::UI &u, const char *s) {
  while (*s)
    u.key(uint8_t(*s++));
}
static void tab(sui::UI &u, int i) {
  assert(tap(u, 32 + i * 64, 15));
  draw(u);
}
static void back(sui::UI &u) {
  assert(tap(u, 30, 48));
  draw(u);
}

static void testReleaseAndLists() {
  api = FakeApi();
  api.add("c0", "Oeffentlich", dev::Kind::Channel);
  for (int i = 0; i < 18; ++i) {
    char id[14], name[32];
    snprintf(id, sizeof(id), "d%012x", i + 1);
    snprintf(name, sizeof(name), "Kontakt %d", i);
    api.add(id, name, dev::Kind::Chat);
  }
  sui::UI u;
  start(u);
  // Cross-row releases, row gaps and tab gaps are not selections.
  assert(!u.gesture(100, 100, 100, 108));
  assert(!tap(u, 100, 103));
  assert(!tap(u, 64, 15));
  assert(u.current() == sui::CHAT_LIST);
  assert(u.gesture(100, 210, 105, 90));
  draw(u);
  assert(tap(u, 100, 80));
  assert(std::string(u.conversation()) == "d000000000004");
  draw(u);
  back(u);
  // The displayed ID wins over a subsequently reordered live contact list.
  draw(u);
  std::swap(api.targets[4], api.targets[5]);
  assert(tap(u, 100, 80));
  assert(std::string(u.conversation()) == "d000000000004");
  draw(u);
  type(u, "Bitte Wasser");
  draw(u);
  int sends = api.sends;
  assert(!tap(u, 150, 205));                           // text box is not SEND
  assert(!tap(u, 228, 205));                           // 4 px gap before SEND
  assert(!u.gesture(280, 205, 220, 205));              // leave SEND
  assert(!u.gesture(280, 205, 280, 205, true));        // moved out and back
  assert(!u.gesture(280, 205, 280, 205, false, 1000)); // long resting finger
  assert(!u.gesture(280, 205, 280, 140));              // scroll starts on SEND
  assert(!u.gesture(61, 15, 66, 15));                  // another tab on release
  assert(api.sends == sends && u.draft()[0]);
  uint32_t before = u.touchEpoch();
  draw(u);
  assert(u.touchEpoch() != before);
  assert(tap(u, 280, 205));
  assert(api.sends == sends + 1);
  assert(api.lastId == "d000000000004" && api.lastText == "Bitte Wasser");
  draw(u);
  assert(tap(u, 280, 205));
  assert(api.sends == sends + 1); // empty repeat does not send
  assert(u.gesture(8, 120, 100, 125));
  assert(u.current() == sui::CHAT_LIST);
  draw(u);
  // A disappeared displayed contact must never open its replacement at the same index.
  api.targets.erase(api.targets.begin() + 4);
  tap(u, 100, 80);
  assert(u.current() == sui::CHAT_LIST);
}

static void testQuickPickers() {
  api = FakeApi();
  api.add("c0", "Oeffentlich", dev::Kind::Channel);
  api.add("d000000000001", "Anna", dev::Kind::Chat);
  sui::UI u;
  start(u);
  tab(u, 1);
  assert(!u.gesture(80, 195, 80, 150));
  assert(api.sends == 0);
  tap(u, 80, 80);
  draw(u);
  assert(u.quickPage().picking());
  assert(u.gesture(100, 210, 100, 120));
  draw(u);
  tap(u, 100, 80);
  assert(u.quickPage().draft().preset == 3);
  assert(api.sends == 0);
  draw(u);
  // Place chooser is explicit and does not silently enable GPS sharing.
  tap(u, 80, 160);
  draw(u);
  assert(u.quickPage().picking());
  tap(u, 100, touchui::row(2).y + 10);
  draw(u);
  assert(u.quickPage().draft().place == 1 && !api.g.share && !api.g.fresh);
  tap(u, 240, 160);
  draw(u);
  assert(u.quickPage().picking());
  std::swap(api.targets[0], api.targets[1]);
  tap(u, 100, touchui::row(1).y + 10);
  draw(u); // displayed Anna, not the new index 1
  assert(!u.quickPage().picking());
  tap(u, 80, 199);
  assert(api.sends == 1 && api.lastId == "d000000000001");
  tap(u, 80, 199);
  assert(api.sends == 1); // accepted quick message cannot duplicate
  draw(u);
  tap(u, 240, 160);
  draw(u);
  api.targets.clear();
  tap(u, 100, 80);
  assert(u.quickPage().picking() && api.sends == 1);
  back(u);
  assert(u.current() == sui::QUICK && !u.quickPage().picking());
  back(u);
  assert(u.current() == sui::MENU);
  // Every menu tile is independently reachable through a centred touch.
  for (int i = 0; i < sui::MENU_COUNT; ++i) {
    auto r = touchui::tile(i);
    tap(u, r.x + r.w / 2, r.y + r.h / 2);
    draw(u);
    while (u.back()) {
    }
    draw(u);
  }
}

static void testMapAndSettings() {
  api = FakeApi();
  api.add("c0", "Oeffentlich", dev::Kind::Channel);
  sui::UI u;
  start(u);
  tab(u, 3);
  double x, y, px, py;
  u.mapView().toUnits(50.543333, 7.175806, x, y);
  u.mapView().project(x, y, px, py);
  assert(abs(px - 160) < 1);
  assert(u.gesture(150, 140, 210, 170));
  u.mapView().project(x, y, px, py);
  assert(px > 160 && !api.g.fresh && !api.g.share && api.sends == 0);
  draw(u);
  double zoom = u.mapView().zoomFactor();
  tap(u, 265, 211);
  assert(u.mapView().zoomFactor() >= zoom);
  tap(u, 50, 211);
  zoom = u.mapView().zoomFactor();
  tap(u, 265, 211);
  assert(u.mapView().zoomFactor() > zoom);
  assert(!tap(u, 107, 211)); // gap, not a zoom or GPS control
  tap(u, 270, 80);
  assert(api.map == 1);
  draw(u);
  tap(u, 160, 211);
  assert(!api.g.fresh && !api.g.share && api.sends == 0);
  draw(u);
  tab(u, 4);
  assert(u.current() == sui::DEVICE);
  assert(u.gesture(100, 210, 100, 100));
  draw(u);
  assert(screen.allText.find("Touch kalibrieren") != std::string::npos);
  tap(u, 100, touchui::row(1).y + 10);
  assert(u.takeTouchCalibration() && !u.takeTouchCalibration());
  draw(u);
  tap(u, 100, touchui::row(3).y + 10);
  assert(u.current() == sui::DEVICE_INFO);
  draw(u);
  assert(screen.allText.find("Akku") != std::string::npos);
  back(u);
  tap(u, 100, touchui::row(2).y + 10);
  assert(u.current() == sui::LOCATION);
  draw(u);
  tap(u, 80, 190);
  assert(api.sends == 0); // sharing disabled, no fix
  tap(u, 80, 145);
  assert(api.g.share);
  draw(u);
  tap(u, 80, 190);
  assert(api.sends == 0); // enabled sharing is not a fresh fix
  api.g.fresh = true;
  api.g.lat = 50.54;
  api.g.lon = 7.17;
  draw(u);
  tap(u, 80, 190);
  assert(api.sends == 1 && api.lastText.find("!DP1|") == 0);
  assert(!u.gesture(-1, 80, 50, 80));
  assert(!u.gesture(10, 250, 10, 200));
}

int main() {
  testReleaseAndLists();
  testQuickPickers();
  testMapAndSettings();
  puts("touch_ui: release safety, stable recipients, scrolling, pickers, settings and GPS passed");
}
