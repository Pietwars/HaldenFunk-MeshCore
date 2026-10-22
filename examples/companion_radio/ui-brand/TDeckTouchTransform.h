#pragma once
#include <math.h>
#include <stdint.h>

struct TDeckTouchTransform {
  uint32_t magic = 0x54444b31;
  float ax=0, bx=1, cx=0, ay=-1, by=0, cy=239;
  bool valid() const {
    return magic == 0x54444b31 && isfinite(ax) && isfinite(bx) && isfinite(cx) &&
      isfinite(ay) && isfinite(by) && isfinite(cy) && fabsf(ax)<=8 && fabsf(bx)<=8 &&
      fabsf(ay)<=8 && fabsf(by)<=8 && fabsf(cx)<32768 && fabsf(cy)<32768 &&
      fabsf(ax*by-bx*ay)>0.00001f;
  }
  bool fit(const int x[3], const int y[3]) {
    const float ux=x[1]-x[0], uy=y[1]-y[0], vx=x[2]-x[0], vy=y[2]-y[0];
    const float det=ux*vy-uy*vx;
    if (fabsf(det)<100) return false;
    ax=272*vy/det; bx=-272*vx/det; cx=24-ax*x[0]-bx*y[0];
    ay=-152*uy/det; by=152*ux/det; cy=44-ay*x[0]-by*y[0];
    return valid();
  }
  void map(int rawX, int rawY, int &x, int &y) const {
    x=lroundf(ax*rawX+bx*rawY+cx); y=lroundf(ay*rawX+by*rawY+cy);
  }
};
