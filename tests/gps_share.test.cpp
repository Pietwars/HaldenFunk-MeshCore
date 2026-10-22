// GPS sharing of the T-Deck Plus standalone build (Issue #35): only after switching on, only a fresh fix,
// the explicit location stays the stored one.
#include <assert.h>
#include <stdio.h>
#include "../examples/companion_radio/GpsShare.h"
using namespace gpsshare;

int main() {
  Share s; s.begin(Loc(50.5, 7.1, POLICY_SHARE));               // explicit location set in the app
  Fix none, stale(true, false, 50.54, 7.11), fresh(true, true, 50.54, 7.11);
  // share off: GPS overwrote node_lat (driver) -> explicit location is restored, policy unchanged
  Loc n = s.update(Loc(50.54, 7.11, POLICY_SHARE), false, fresh);
  assert(samePos(n.lat, 50.5) && samePos(n.lon, 7.1) && n.policy == POLICY_SHARE);
  // share on, fix stale -> no GPS position
  n = s.update(n, true, stale); assert(samePos(n.lat, 50.5) && !s.sharingFix());
  // share on + fresh fix -> fix with SHARE
  n = s.update(n, true, fresh); assert(samePos(n.lat, 50.54) && n.policy == POLICY_SHARE && s.sharingFix());
  // persisted is still the explicit location
  Loc p = s.persist(n, fresh); assert(samePos(p.lat, 50.5) && samePos(p.lon, 7.1));
  // the fix moves on (driver writes the new receiver position): still shared, not taken as explicit
  Fix moved(true, true, 50.545, 7.115);
  n = s.update(Loc(50.545, 7.115, POLICY_SHARE), true, moved); assert(samePos(n.lat, 50.545));
  assert(samePos(s.explicitLoc().lat, 50.5));
  // fix gets old -> back to the explicit location
  Fix old(true, false, 50.545, 7.115);
  n = s.update(n, true, old); assert(samePos(n.lat, 50.5));
  // switching off -> explicit
  n = s.update(n, false, fresh); assert(samePos(n.lat, 50.5) && !s.sharingFix());

  // the app sets a new location while sharing is on: becomes explicit, survives persist
  n = s.update(n, true, fresh); assert(samePos(n.lat, 50.54));
  p = s.persist(Loc(50.6, 7.2, POLICY_SHARE), fresh);
  assert(samePos(p.lat, 50.6) && samePos(p.lon, 7.2));
  n = s.update(Loc(50.6, 7.2, POLICY_SHARE), false, fresh); assert(samePos(n.lat, 50.6));
  // the app switches location sharing off (policy NONE): also honoured while GPS sharing is off
  n = s.update(Loc(50.6, 7.2, POLICY_NONE), false, fresh); assert(n.policy == POLICY_NONE);
  // GPS share on with fresh fix still shares the fix (the user switched it on on the device)
  n = s.update(n, true, fresh); assert(n.policy == POLICY_SHARE && samePos(n.lat, 50.54));
  n = s.update(n, false, fresh); assert(n.policy == POLICY_NONE && samePos(n.lat, 50.6));

  // no explicit location, sharing off: nothing in adverts even with a fix
  Share z; z.begin(Loc(0, 0, POLICY_NONE));
  n = z.update(Loc(50.54, 7.11, POLICY_NONE), false, fresh); assert(n.policy == POLICY_NONE && samePos(n.lat, 0));
  n = z.update(n, true, none); assert(n.policy == POLICY_NONE);
  n = z.update(n, true, fresh); assert(n.policy == POLICY_SHARE && samePos(n.lat, 50.54));
  assert(samePos(z.persist(n, fresh).lat, 0) && z.persist(n, fresh).policy == POLICY_NONE);
  puts("gps_share: ok");
  return 0;
}
