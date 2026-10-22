#include <cassert>
#include <cmath>
#include "../examples/companion_radio/ui-brand/TDeckTouchTransform.h"
int main() {
  TDeckTouchTransform t;
  assert(t.valid());
  // Native panel coordinates, swapped axes and a mirrored axis.
  const int x[]={196,196,44}, y[]={24,296,24};
  assert(t.fit(x,y));
  int px,py;
  t.map(196,24,px,py); assert(px==24 && py==44);
  t.map(196,296,px,py); assert(px==296 && py==44);
  t.map(44,24,px,py); assert(px==24 && py==196);
  t.map(120,160,px,py); assert(px==160 && py==120);
  const int bad[]={40,40,40}; assert(!t.fit(bad,bad));
  t.ax=NAN; assert(!t.valid());
  t=TDeckTouchTransform(); t.magic=0; assert(!t.valid());
}
