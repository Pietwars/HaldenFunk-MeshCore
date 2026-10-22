#include <assert.h>
#include <string.h>
#include <math.h>
#include "../examples/companion_radio/ui-brand/QuickModel.h"
#include "../src/helpers/sensors/FixFreshness.h"

int main() {
  using namespace quick;
  Model m; char out[101]; uint32_t stamp = 1792400000;
  assert(PRESET_COUNT == 8 && PLACE_COUNT == 12);
  // Defaults must use the real receiver; old/missing fixes cannot turn into 0,0.
  assert(m.place == 0);
  assert(m.compose(false, {}, stamp, out, sizeof(out)) == Result::NoFix && !out[0]);
  assert(m.compose(true, {}, stamp, out, sizeof(out)) == Result::NoFix);
  assert(!validFix(Fix(true, 0, 0)) && !validFix(Fix(true, NAN, 7)));
  assert(!validFix(Fix(true, 91, 7)) && !validFix(Fix(true, 50, 181)));
  Fix gps(true, 50.5426, 7.1761);
  assert(m.compose(false, gps, stamp, out, sizeof(out)) == Result::Ready);
  assert(strcmp(out, "[HILFE] Trinkwasser benötigt, Personen: @ 50.5426,7.1761") == 0);
  assert(m.compose(true, gps, stamp, out, sizeof(out)) == Result::Ready);
  assert(strcmp(out, "!DP1|50.5426,7.1761|1792400000") == 0);
  m.place = 1; // explicitly selected manual place, without a GPS fix
  assert(m.compose(false, {}, stamp, out, sizeof(out)) == Result::Ready);
  assert(strstr(out, "@ Ahrweiler 50.5430,7.0965"));
  m.place = -1;
  assert(m.compose(false, {}, stamp, out, sizeof(out)) == Result::Ready);
  assert(!strstr(out, " @ "));
  assert(m.compose(true, {}, stamp, out, sizeof(out)) == Result::NoFix);
  memset(m.text, 'x', 100); m.text[100] = 0;
  assert(!m.append('y'));
  assert(m.compose(false, {}, stamp, out, sizeof(out)) == Result::Ready && strlen(out) == 100);
  m.place = 0;
  assert(m.compose(false, gps, stamp, out, sizeof(out)) == Result::TooLong && !out[0]);
  assert(m.compose(false, gps, 0, out, sizeof(out)) == Result::InvalidTime);
  strcpy(m.text, "  "); m.place = -1;
  assert(m.compose(false, {}, stamp, out, sizeof(out)) == Result::Empty);
  strcpy(m.text, "Äöüß");
  m.backspace(); assert(strcmp(m.text, "Äöü") == 0);
  m.backspace(); assert(strcmp(m.text, "Äö") == 0);
  m.backspace(); m.backspace(); m.backspace(); assert(!m.text[0]);
  m.place = PLACE_COUNT; m.cyclePlace(1); assert(m.place == -1);
  m.cyclePlace(-1); assert(m.place == PLACE_COUNT);
  m.place = PLACE_COUNT + 1;
  assert(m.compose(false, gps, stamp, out, sizeof(out)) == Result::NoFix);
  for (int i = 0; i < PRESET_COUNT; ++i) {
    m.selectPreset(i); m.place = -1;
    assert(m.compose(false, {}, stamp, out, sizeof(out)) == Result::Ready);
  }
  uint8_t packet[176];
  size_t n = frame("c0", "Hallo", stamp, packet, sizeof(packet));
  assert(n == 12 && packet[0] == 3 && packet[1] == 0 && packet[2] == 0);
  uint32_t received; memcpy(&received, packet + 3, 4); assert(received == stamp);
  assert(memcmp(packet + 7, "Hallo", 5) == 0);
  n = frame("d0102030405ff", "Test", stamp, packet, sizeof(packet));
  assert(n == 17 && packet[0] == 2 && packet[7] == 1 && packet[12] == 255);
  assert(frame("d0102030405zz", "Test", stamp, packet, sizeof(packet)) == 0);
  assert(frame("c8", "Test", stamp, packet, sizeof(packet)) == 0);
  assert(frame("c0", "", stamp, packet, sizeof(packet)) == 0);
  assert(frame("c0", "Test", stamp, packet, 10) == 0);
  // A provider that once had a fix must not keep using it after cable loss.
  assert(gpsfix::fresh(true, true, 15000, 0, 15000));
  assert(!gpsfix::fresh(true, true, 15001, 0, 15000));
  assert(!gpsfix::fresh(false, true, 1000, 900, 15000));
  assert(!gpsfix::fresh(true, false, 1000, 900, 15000));
  assert(gpsfix::fresh(true, true, 20, 0xFFFFFFF0u, 15000)); // millis wrap
  puts("quick_model.test.cpp: presets, UTF-8, frame limits and fresh GPS passed");
}
