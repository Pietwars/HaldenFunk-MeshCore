// Hosttest: Akku-Prozent aus Spannung, Trend (Verbrauch %/h, Restzeit) und Texte der Display-Statuszeile.
#include "../examples/companion_radio/ui-brand/BattTrend.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
using namespace batt;

static void expect(const char* got, const char* want) {
  if (strcmp(got, want)) { printf("FEHLER: '%s' != '%s'\n", got, want); assert(0); }
}

int main() {
  // Kurve: Ecken und Interpolation
  assert(percentFromMv(4300) == 100 && percentFromMv(3000) == 0);
  assert(fabs(percentFromMv(3840) - 50) < 0.01f && fabs(percentFromMv(3845) - 52.5f) < 0.01f);
  for (int mv = 3270; mv < 4200; mv += 5) assert(percentFromMv(mv + 5) >= percentFromMv(mv));

  Now b; b.mv = 3925; b.pct = 67; b.pct_approx = true;
  Trend t; char l[40], e[48];
  // zu wenig Daten
  texts(b, t, l, sizeof l, e, sizeof e, 0);
  expect(l, "ca. 67 % 3,93 V"); expect(e, "Verbrauch: Messung, noch 20 min");
  for (int i = 0; i <= 9; i++) t.add(1000 + i * 60, 4000);
  { float a0, b0; assert(!t.slope(a0, b0)); }
  texts(b, t, l, sizeof l, e, sizeof e, 2); expect(e, "Verbrauch: noch 11 min");

  // gleichmaessiger Abfall: 3980 -> 3950 mV (75 -> 70 %) in 60 min = 5 %/h, Rest ca. 14 h
  t.reset();
  for (int i = 0; i <= 60; i++) t.add(5000 + i * 60, 3980 - i / 2);
  float r, p; assert(t.slope(r, p)); assert(fabs(r + 5) < 0.3f && fabs(p - 70) < 0.5f);
  texts(b, t, l, sizeof l, e, sizeof e, 0); expect(e, "Verbr. ca. 5,0 %/h, Rest ca. 14 h");
  texts(b, t, l, sizeof l, e, sizeof e, 1);  expect(e, "Verbr. ca. 5,0 %/h, Rest 14 h");
  texts(b, t, l, sizeof l, e, sizeof e, 2);  expect(e, "ca. 5,0 %/h, Rest 14 h");
  // Fenster begrenzt auf 61 Punkte
  for (int i = 61; i < 200; i++) t.add(5000 + i * 60, 3950);
  assert(t.count() == BATT_WINDOW);

  // Spannungssprung (z. B. USB ab) -> neu messen
  t.add(5000 + 200 * 60, 3850); assert(t.count() == 1);

  // Laden / USB (T-Watch) und kein Akku
  Now w; w.mv = 4105; w.pct = 88; w.pct_approx = false; w.usb = 1; w.charging = 1;
  texts(w, t, l, sizeof l, e, sizeof e, 0); expect(l, "88 % 4,11 V"); expect(e, "USB: Laden");
  w.charging = 0; texts(w, t, l, sizeof l, e, sizeof e, 0); expect(e, "USB: kein Laden");
  Now n; n.usb = 1; texts(n, t, l, sizeof l, e, sizeof e, 0); expect(l, "kein Akku"); expect(e, "USB");

  // steigende Spannung ohne USB-Erkennung (T-Deck am Ladegeraet)
  t.reset(); for (int i = 0; i <= 30; i++) t.add(i * 60, 3900 + i);
  texts(b, t, l, sizeof l, e, sizeof e, 0); expect(e, "Spannung steigt (Laden?)");
  // sehr kleiner Verbrauch, kurze Restzeit
  t.reset(); for (int i = 0; i <= 30; i++) t.add(i * 60, 3900);
  texts(b, t, l, sizeof l, e, sizeof e, 0); expect(e, "Verbrauch unter 0,1 %/h");
  t.reset(); for (int i = 0; i <= 30; i++) t.add(i * 60, 3730 - i);   // 20 -> ~11 %, ca. 18 %/h
  texts(b, t, l, sizeof l, e, sizeof e, 0);
  assert(!strncmp(e, "Verbr. ca. 1", 12) && strstr(e, "Rest ca. 0,"));
  t.reset(); for (int i = 0; i <= 60; i++) t.add(i * 60, 4000);
  for (int i = 61; i <= 120; i++) t.add(i * 60, 4000 - (i % 2));      // Rauschen um 0
  b.mv = 3995; texts(b, t, l, sizeof l, e, sizeof e, 0); expect(l, "ca. 67 % 4,00 V");
  printf("batt_trend.test.cpp: ok\n");
  return 0;
}
