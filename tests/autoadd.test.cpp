// HaldenFunk MeshCore default: only repeaters (2) and room servers (3) are added automatically.
#include <assert.h>
#include "../examples/companion_radio/AutoAddPolicy.h"
int main() {
  const uint8_t m = autoadd::DISASTER_DEFAULT_MANUAL, c = autoadd::DISASTER_DEFAULT_CONFIG;
  assert(m == 1 && c == 0x0C);
  assert(!autoadd::shouldAdd(m, c, 1));  // chat / companion: manual
  assert(autoadd::shouldAdd(m, c, 2));   // repeater
  assert(autoadd::shouldAdd(m, c, 3));   // room server
  assert(!autoadd::shouldAdd(m, c, 4));  // sensor: manual
  assert(!autoadd::shouldAdd(m, c, 0) && !autoadd::shouldAdd(m, c, 9));  // none/unknown
  assert((c & autoadd::OVERWRITE_OLDEST) == 0);
  // MeshCore "auto add all" (manual bit clear) and an app-chosen selection still work
  for (uint8_t t = 0; t < 16; t++) assert(autoadd::shouldAdd(0, 0, t));
  assert(autoadd::shouldAdd(1, autoadd::CHAT, 1) && !autoadd::shouldAdd(1, autoadd::CHAT, 2));
  assert(!autoadd::shouldAdd(1, 0, 2));
}
