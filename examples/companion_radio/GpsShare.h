#pragma once
// "Standort teilen" of the T-Deck Plus standalone build (Issue #35, GPS ins Mesh). Pure logic, host tested
// (tests/gps_share.test.cpp).
//
// MeshCore writes every valid GPS position into sensors.node_lat/lon while the receiver runs, and adverts carry
// node_lat/lon whenever advert_loc_policy is SHARE (DAR: after an explicitly set location). In the standalone
// build the GPS position goes into the mesh only after the user switched sharing on:
//   share on  + fresh fix (15 s)  -> node position = fix, policy SHARE (adverts, "Position in Chat senden")
//   otherwise                     -> the explicit location and policy (set in the MeshCore app; may be "none")
// The explicit location is what is persisted (savePrefs hook), so a GPS fix never becomes the stored location.
// Changes made by the app (CMD_SET_ADVERT_LATLON, advert policy) are recognised and become the new explicit
// location; values written by the GPS driver (equal to the receiver's position) are not.
#include <stdint.h>
#include <math.h>

namespace gpsshare {
constexpr uint8_t POLICY_NONE = 0, POLICY_SHARE = 1;   // ADVERT_LOC_NONE / ADVERT_LOC_SHARE (NodePrefs.h)
struct Loc {
  double lat = 0, lon = 0; uint8_t policy = POLICY_NONE;
  Loc() {}
  Loc(double la, double lo, uint8_t p) : lat(la), lon(lo), policy(p) {}
};
inline bool samePos(double a, double b) { return fabs(a - b) < 5e-7; }
inline bool same(const Loc &a, const Loc &b) { return samePos(a.lat, b.lat) && samePos(a.lon, b.lon) && a.policy == b.policy; }

struct Fix {
  bool valid = false, fresh = false; double lat = 0, lon = 0;   // receiver position (also a stale one)
  Fix() {}
  Fix(bool v, bool f, double la, double lo) : valid(v), fresh(f), lat(la), lon(lo) {}
};

class Share {
  Loc expl, out;
  bool started = false;
  // Take over changes of the app; ignore what the GPS driver wrote.
  void observe(const Loc &node, const Fix &gps) {
    if (!started) { expl = out = node; started = true; return; }
    if (node.policy != out.policy) expl.policy = node.policy;
    bool moved = !samePos(node.lat, out.lat) || !samePos(node.lon, out.lon);
    bool byGps = gps.valid && samePos(node.lat, gps.lat) && samePos(node.lon, gps.lon);
    if (moved && !byGps) { expl.lat = node.lat; expl.lon = node.lon; }
  }
public:
  void begin(const Loc &stored) { expl = out = stored; started = true; }
  // Once per loop with the current node values and receiver state. Returns what the node must use.
  Loc update(const Loc &node, bool share, const Fix &gps) {
    observe(node, gps);
    out = share && gps.valid && gps.fresh ? Loc(gps.lat, gps.lon, POLICY_SHARE) : expl;
    return out;
  }
  // Values to persist (savePrefs hook): never a GPS fix.
  Loc persist(const Loc &node, const Fix &gps) { observe(node, gps); return expl; }
  const Loc &explicitLoc() const { return expl; }
  bool sharingFix() const { return out.policy == POLICY_SHARE && !same(out, expl); }
};
}
