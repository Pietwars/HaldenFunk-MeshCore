#include <cassert>
#include "../examples/companion_radio/ui-brand/TDeckTrackball.h"

int main() {
  using T = TDeckTrackball;
  T t;
  // A resting motion signal, even if LOW, must never produce repeated steps.
  for (unsigned n = 400; n < 5000; n += 20) assert(t.update(n, 0, 0, false) == T::NONE);
  assert(t.update(5100, 1, 0, false) == T::NONE); // incidental touch
  assert(t.update(5120, -1, 0, false) == T::NONE); // opposite jitter cancels
  assert(t.update(5140, 3, 0, false) == T::NONE);
  assert(t.update(5160, 1, 0, false) == T::RIGHT);
  assert(t.update(5180, 30, 0, false) == T::NONE); // no buffered runaway
  assert(t.update(5600, 0, 0, false) == T::NONE);
  assert(t.update(5620, -4, 0, false) == T::LEFT);
  assert(t.update(6000, 0, -4, false) == T::UP);
  assert(t.update(6400, 0, 4, false) == T::DOWN);
  assert(t.update(6800, 2, 0, false) == T::NONE);
  assert(t.update(7050, 2, 0, false) == T::NONE); // old partial gesture expires
  // Bounce and long holds cannot trigger the second quick-help confirmation.
  assert(t.update(7100, 0, 0, true) == T::NONE);
  assert(t.update(7110, 0, 0, false) == T::NONE);
  assert(t.update(7120, 0, 0, true) == T::NONE);
  assert(t.update(7150, 0, 0, true) == T::CLICK);
  for (unsigned n = 7170; n < 9000; n += 20) assert(t.update(n, 10, 0, true) == T::NONE);
  assert(t.update(9010, 0, 0, false) == T::NONE);
  assert(t.update(9050, 0, 0, false) == T::NONE);
  assert(t.update(9070, 0, 0, true) == T::NONE);
  assert(t.update(9110, 0, 0, true) == T::CLICK);
  T wrap;
  assert(wrap.update(0xffffff00u, 4, 0, false) == T::RIGHT);
  assert(wrap.update(0x80u, -4, 0, false) == T::LEFT); // millis rollover
}
