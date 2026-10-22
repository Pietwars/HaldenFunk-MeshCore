#pragma once
// Simplified map of the T-Deck Plus standalone display (Issue #35): the five web maps reduced to water, roads,
// railway and points (MapData.h, tools/gen_device_map.py), drawn like drawMap() of the web UI on 320 x 224.
// (c) OpenStreetMap-Mitwirkende, ODbL 1.0.
#include <math.h>
#include <helpers/ui/DisplayDriver.h>
#include "MapData.h"
#include "QuickData.h"
#include "Theme.h"

namespace devmap {
struct Mark { double lat, lon; uint16_t color; char label[34]; };

// Label boxes already drawn in this frame: later labels that would overlap are left out (dots stay).
struct Labels {
  int16_t box[64][4]; unsigned n = 0;
  bool take(int x, int y, int w, int h) {
    for (unsigned i = 0; i < n; ++i)
      if (x < box[i][0] + box[i][2] && box[i][0] < x + w && y < box[i][1] + box[i][3] && box[i][1] < y + h) return false;
    if (n < 64) { box[n][0] = int16_t(x); box[n][1] = int16_t(y); box[n][2] = int16_t(w); box[n][3] = int16_t(h); ++n; }
    return true;
  }
};

class View {
  int idx = 0;
  double cx = 0, cy = 0, ppu = 1, k = 1, fitPpu = 1;   // centre in map units, pixels per unit (latitude)
public:
  static constexpr int TOP = theme::NAV_H, WIDTH = theme::W, HEIGHT = theme::H - theme::NAV_H;
  const Map &map() const { return MAPS[idx]; }
  int index() const { return idx; }
  void select(int i) {
    idx = ((i % MAP_COUNT) + MAP_COUNT) % MAP_COUNT;
    const Map &m = MAPS[idx];
    double lat = (m.lat0 + (m.vy0 + m.vy1) / 2.0 * UNIT) / 1e5;
    k = cos(lat * M_PI / 180);
    fit();
  }
  void fit() {
    const Map &m = MAPS[idx];
    cx = (m.vx0 + m.vx1) / 2.0; cy = (m.vy0 + m.vy1) / 2.0;
    double sx = WIDTH / ((m.vx1 - m.vx0) * k), sy = HEIGHT / double(m.vy1 - m.vy0);
    ppu = fitPpu = (sx < sy ? sx : sy) * 0.95;
  }
  // pixels per degree latitude: same zoom thresholds as the web map
  double ppd() const { return ppu * 1e5 / UNIT; }
  double zoomFactor() const { return ppu / fitPpu; }
  void zoom(double f) {
    // GPS centring can exceed the overview-relative limit. '+' must never zoom OUT.
    if (f > 1 && ppu >= fitPpu * 32) return;
    double z = ppu * f;
    if (z < fitPpu * 0.5) z = fitPpu * 0.5;
    if (z > fitPpu * 32) z = fitPpu * 32;
    ppu = z;
  }
  void pan(int dx_px, int dy_px) { cx += dx_px / (ppu * k); cy -= dy_px / ppu; }
  bool toUnits(double lat, double lon, double &x, double &y) const {
    const Map &m = MAPS[idx];
    x = (lon * 1e5 - m.lon0) / UNIT; y = (lat * 1e5 - m.lat0) / UNIT;
    return isfinite(x) && isfinite(y);
  }
  void centerOn(double lat, double lon) {
    double x, y; if (!toUnits(lat, lon, x, y)) return;
    cx = x; cy = y; if (ppd() < 30000) ppu = 30000.0 * UNIT / 1e5;   // web "Ich": at least 30000 px/deg
  }
  void project(double x, double y, double &px, double &py) const {
    px = WIDTH / 2.0 + (x - cx) * ppu * k; py = TOP + HEIGHT / 2.0 - (y - cy) * ppu;
  }
  bool inside(double lat, double lon) const {
    double x, y, px, py; if (!toUnits(lat, lon, x, y)) return false; project(x, y, px, py);
    return px >= 0 && px < WIDTH && py >= TOP && py < theme::H;
  }

  // Cohen-Sutherland clipping to the map area.
  static int code(double x, double y) {
    return (x < 0 ? 1 : x > WIDTH - 1 ? 2 : 0) | (y < TOP ? 4 : y > theme::H - 1 ? 8 : 0);
  }
  static bool clip(double &x0, double &y0, double &x1, double &y1) {
    int c0 = code(x0, y0), c1 = code(x1, y1);
    for (int guard = 0; guard < 8; ++guard) {
      if (!(c0 | c1)) return true;
      if (c0 & c1) return false;
      int c = c0 ? c0 : c1; double x = 0, y = 0;
      if (c & 8) { x = x0 + (x1 - x0) * (theme::H - 1 - y0) / (y1 - y0); y = theme::H - 1; }
      else if (c & 4) { x = x0 + (x1 - x0) * (TOP - y0) / (y1 - y0); y = TOP; }
      else if (c & 2) { y = y0 + (y1 - y0) * (WIDTH - 1 - x0) / (x1 - x0); x = WIDTH - 1; }
      else { y = y0 + (y1 - y0) * (0 - x0) / (x1 - x0); x = 0; }
      if (c == c0) { x0 = x; y0 = y; c0 = code(x0, y0); } else { x1 = x; y1 = y; c1 = code(x1, y1); }
    }
    return false;
  }
  // Draw the map with marks; returns the number of drawn line segments (tests, timing).
  unsigned render(DisplayDriver *d, const Mark *marks, unsigned nmarks, const Mark *me) const {
    static const uint16_t COLOR[STYLE_COUNT] = {theme::rgb(0x1f4f74), theme::rgb(0x3d7fb0), theme::rgb(0x4da3e0),
                                                theme::rgb(0x8a97a2), theme::rgb(0xb08ad0), theme::rgb(0xe8c07a)};
    const Map &m = MAPS[idx];
    d->nativeFill(0, TOP, WIDTH, HEIGHT, theme::MAP_BG);
    double p = ppd(); unsigned segs = 0;
    // visible window in map units
    double hw = WIDTH / (2 * ppu * k), hh = HEIGHT / (2 * ppu);
    double wx0 = cx - hw, wx1 = cx + hw, wy0 = cy - hh, wy1 = cy + hh;
    for (uint16_t i = 0; i < m.nlines; ++i) {
      const Line &l = m.lines[i];
      if (l.style == ROAD && p < 6000) continue;                    // web: minor roads only from 6000 px/deg
      if (l.x1 < wx0 || l.x0 > wx1 || l.y1 < wy0 || l.y0 > wy1) continue;
      const uint16_t *q = m.xy + 2 * l.at;
      double ax, ay; project(q[0], q[1], ax, ay);
      int thick = l.style == RIVER || (l.style == MAIN_ROAD && p >= 15000) ? 2 : 1;
      for (uint16_t j = 1; j < l.n; ++j) {
        double bx, by; project(q[2 * j], q[2 * j + 1], bx, by);
        double x0 = ax, y0 = ay, x1 = bx, y1 = by;
        if (clip(x0, y0, x1, y1)) {
          d->nativeLine(int(x0), int(y0), int(x1), int(y1), COLOR[l.style]);
          if (thick > 1 && int(y0) + 1 < theme::H && int(y1) + 1 < theme::H) d->nativeLine(int(x0), int(y0) + 1, int(x1), int(y1) + 1, COLOR[l.style]);
          ++segs;
        }
        ax = bx; ay = by;
      }
    }
    char buf[40]; Labels labels;
    // pins first: own position and contacts keep their labels
    if (me) pin(d, *me, labels);
    for (unsigned i = 0; i < nmarks; ++i) pin(d, marks[i], labels);
    for (uint16_t i = 0; i < m.npoints; ++i) {
      const Point &pt = m.points[i];
      double px, py; project(pt.x, pt.y, px, py);
      if (px < 2 || py < TOP + 2 || px > WIDTH - 3 || py > theme::H - 3) continue;
      if (pt.kind >= TOWN) {
        bool big = pt.kind == TOWN, vil = pt.kind == VILLAGE;
        if (!big && !(vil && p >= 6000) && p < 20000) continue;
        int n = int(strlen(pt.name)); if (n > 24) n = 24;
        memcpy(buf, pt.name, n); buf[n] = 0;
        int x = int(px) - n * 3; if (x < 0) x = 0; if (x + n * 6 > WIDTH) x = WIDTH - n * 6;
        if (labels.take(x - 1, int(py) - 5, n * 6 + 2, 10)) d->nativeText(x, int(py) - 4, 1, big ? theme::TEXT : theme::rgb(0xb7c7d0), buf);
      } else if (p > 9000) {
        d->nativeDisc(int(px), int(py), 2, pt.kind == PHARMACY ? theme::PHARMACY : theme::WATER);
      }
    }
    if (p < 8000) for (const auto &pl : quick::PLACES) {             // web: regional HaldenFunk places in the overview
      double x, y, px, py; toUnits(pl.lat, pl.lon, x, y); project(x, y, px, py);
      if (px < 2 || py < TOP + 10 || px > WIDTH - 3 || py > theme::H - 3) continue;
      d->nativeDisc(int(px), int(py), 1, theme::SAGE);
      char name[40]; size_t n = theme::toCp437(pl.name, name, sizeof(name), 24);
      int x0 = int(px) - int(n) * 3; if (x0 < 0) x0 = 0; if (x0 + int(n) * 6 > WIDTH) x0 = WIDTH - int(n) * 6;
      if (labels.take(x0 - 1, int(py) - 11, int(n) * 6 + 2, 10)) d->nativeText(x0, int(py) - 10, 1, theme::SAGE, name);
    }
    // dots of the pins again on top of the map data (labels were reserved first)
    for (unsigned i = 0; i < nmarks; ++i) dot(d, marks[i]);
    if (me) dot(d, *me);
    // ODbL attribution (web: "© OpenStreetMap-Mitwirkende · ODbL")
    static const char attr[] = "(c) OpenStreetMap-Mitwirkende \xfa ODbL";
    int aw = int(sizeof(attr) - 1) * 6 + 4;
    d->nativeFill(WIDTH - aw, theme::H - 10, aw, 10, theme::BG);
    d->nativeText(WIDTH - aw + 2, theme::H - 9, 1, theme::MUTED, attr);
    return segs;
  }
  bool where(const Mark &mk, int &x, int &y) const {
    double ux, uy, px, py; if (!toUnits(mk.lat, mk.lon, ux, uy)) return false; project(ux, uy, px, py);
    if (px < 4 || py < TOP + 4 || px > WIDTH - 5 || py > theme::H - 5) return false;
    x = int(px); y = int(py); return true;
  }
  void dot(DisplayDriver *d, const Mark &mk) const {
    int x, y; if (!where(mk, x, y)) return;
    d->nativeDisc(x, y, 4, 0x0000);
    d->nativeDisc(x, y, 3, mk.color);
  }
  void pin(DisplayDriver *d, const Mark &mk, Labels &labels) const {
    int x, y; if (!where(mk, x, y)) return;
    char name[40]; size_t n = theme::toCp437(mk.label, name, sizeof(name), 20);
    int lx = x + 7; if (lx + int(n) * 6 > WIDTH) lx = x - 7 - int(n) * 6; if (lx < 0) lx = 0;
    labels.take(x - 4, y - 4, 9, 9);
    if (!labels.take(lx - 1, y - 4, int(n) * 6 + 2, 9)) return;
    d->nativeFill(lx - 1, y - 4, int(n) * 6 + 2, 9, theme::MAP_BG);
    d->nativeText(lx, y - 3, 1, theme::TEXT, name);
  }
};
}
