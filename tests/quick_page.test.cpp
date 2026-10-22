// TDeckQuickPage on the T-Deck Plus standalone build (Issue #35; taken over from 0.9.5-test #34):
// keyboard focus, recipients, GPS freshness + "Standort teilen", explicit sending only.
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "FakeApi.h"
#include "FakeDisplay.h"
#include "../examples/companion_radio/ui-brand/TDeckQuickPage.h"

static FakeApi api;

static void fix(bool fresh) { api.g.fresh = fresh; api.g.lat = 50.54258; api.g.lon = 7.17612; }

int main() {
  FakeDisplay screen; const uint32_t stamp = 1792400000;
  api.add("c0", "Oeffentlich", dev::Kind::Channel);
  api.add("d010203040506", "Kontakt", dev::Kind::Chat);
  api.add("d99887766aabb", "Repeater", dev::Kind::Repeater);   // never a quick target
  api.g.share = true;
  TDeckQuickPage p; p.begin(&api); p.tick(0); p.render(&screen);
  assert(screen.outOfBounds == 0);
  // ALT+C moves focus; typing/ENTER must not transmit until Send is explicitly selected.
  p.key('1', stamp); p.key('4', stamp); p.key('\r', stamp); // edit -> place
  assert(api.sends == 0);
  p.key(12, stamp); p.key(12, stamp); p.key('\r', stamp); // target -> send -> attempt
  assert(api.sends == 0); // no device GPS fix
  fix(true);
  p.key('\r', stamp); assert(api.sends == 1);
  assert(api.lastId == "c0" && strstr(api.lastText.c_str(), "50.5426,7.1761"));
  p.key('\r', stamp); assert(api.sends == 1); // repeated key cannot duplicate an accepted submission
  snprintf(api.st, sizeof(api.st), "Antwort unklar; nicht erneut senden"); assert(p.tick(1000));
  p.key('\r', stamp); assert(api.sends == 1);
  p.render(&screen); assert(screen.outOfBounds == 0);

  TDeckQuickPage contact; contact.begin(&api);
  contact.key(12, stamp); contact.key('A', stamp); // edit
  contact.key(12, stamp); contact.key(12, stamp); contact.key(' ', stamp); // choose contact
  snprintf(api.targets[1].id, sizeof(api.targets[1].id), "dffffffffffff"); // refresh must not retarget the draft
  contact.key(12, stamp); contact.key('\r', stamp);
  assert(api.sends == 2 && api.lastId == "d010203040506");
  contact.key(12, stamp); contact.key(12, stamp); contact.key(12, stamp); contact.key(12, stamp); contact.key(12, stamp);
  contact.key(' ', stamp);                         // target list: channel + chat contacts only, wraps
  snprintf(api.targets[1].id, sizeof(api.targets[1].id), "d010203040506");

  TDeckQuickPage position; position.begin(&api);
  for (int i = 0; i < 5; ++i) position.key(12, stamp);
  fix(false); position.key('\r', stamp); assert(api.sends == 2);
  fix(true); api.g.receiverOn = false;
  position.key('\r', stamp); assert(api.sends == 2);
  api.g.receiverOn = true; api.g.share = false;   // sharing off: never the device position
  position.key('\r', stamp); assert(api.sends == 2);
  assert(strstr(position.statusText(), "Standort teilen"));
  api.g.share = true; position.key('\r', stamp); assert(api.sends == 3);
  assert(api.lastText == "!DP1|50.5426,7.1761|1792400000");
  position.render(&screen);

  // sharing off when the page opens: default place is "Ohne Standort"
  api.g.share = false;
  TDeckQuickPage noshare; noshare.begin(&api); assert(noshare.draft().place == -1);
  noshare.key('2', stamp);
  for (int i = 0; i < 3; ++i) noshare.key(12, stamp);
  noshare.key('\r', stamp); assert(api.sends == 4 && !strstr(api.lastText.c_str(), "50.54"));
  api.g.share = true; noshare.enter(); assert(noshare.draft().place == 0);

  TDeckQuickPage keys; keys.begin(&api);
  for (int i = 0; i < 4; ++i) assert(!keys.key(12, stamp));
  keys.key(' ', stamp); assert(api.sends == 4); // space cannot activate the send button
  for (int i = 0; i < 2; ++i) keys.key(12, stamp);
  assert(keys.key('\r', stamp)); // reachable Home action on the original keyboard
  // trackball: up/down = field, left/right = selection; unused left/right is reported for tab switching
  TDeckQuickPage ball; ball.begin(&api);
  assert(ball.move(1, 0) && ball.draft().preset == 1);
  assert(ball.move(0, 1) && ball.textFocus());
  assert(!ball.move(1, 0));
  TDeckQuickPage clear; clear.begin(&api);
  for (int i = 0; i < 7; ++i) clear.key(12, stamp);
  clear.key('\r', stamp); // Clear -> Text; Backspace remains an edit, not Home
  assert(!clear.key(8, stamp));
  clear.key('X', stamp); clear.render(&screen);
  clear.key('\r', stamp); assert(clear.key(8, stamp)); // Backspace outside text returns Home
  assert(screen.outOfBounds == 0);
  puts("quick_page.test.cpp: keyboard, recipients, GPS sharing and explicit sending passed");
}
