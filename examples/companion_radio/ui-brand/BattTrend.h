#pragma once
// Akku-Anzeige (ui-brand): Prozent aus der Spannung, Trend ueber ein gleitendes Fenster, Texte.
// Ohne Arduino-Abhaengigkeiten, damit tests/batt_trend.test.cpp es am PC pruefen kann.
//
// Ehrlich: Weder T-Deck Plus (nur Spannungsteiler am ADC, GPIO 4) noch T-Watch S3 (AXP2101, kein
// Strom-ADC) messen den Akkustrom. "Verbrauch" ist deshalb eine Schaetzung: Abfall des Ladestands in
// Prozent pro Stunde (lineare Regression ueber bis zu 60 min, ein Messpunkt pro Minute) und daraus die
// Restlaufzeit. Angezeigt erst nach mindestens 20 min ohne Spannungssprung/USB-Wechsel, immer mit "ca.".
#include <stdint.h>
#include <stdio.h>
#include <string.h>

namespace batt {

// Ladestand einer LiPo-Zelle aus der Ruhespannung (mV), stueckweise linear. Nur eine Naeherung:
// unter Last (Funk senden, Display) liegt die Spannung etwas tiefer.
inline float percentFromMv(int mv) {
  static const int16_t T[][2] = {
    {4200, 100}, {4150, 95}, {4110, 90}, {4080, 85}, {4020, 80}, {3980, 75}, {3950, 70},
    {3910, 65}, {3870, 60}, {3850, 55}, {3840, 50}, {3820, 45}, {3800, 40}, {3790, 35},
    {3770, 30}, {3750, 25}, {3730, 20}, {3710, 15}, {3690, 10}, {3610, 5}, {3270, 0}};
  const int N = sizeof(T) / sizeof(T[0]);
  if (mv >= T[0][0]) return 100;
  if (mv <= T[N - 1][0]) return 0;
  for (int i = 1; i < N; i++) {
    if (mv >= T[i][0]) {
      float f = (float)(mv - T[i][0]) / (float)(T[i - 1][0] - T[i][0]);
      return T[i][1] + f * (T[i - 1][1] - T[i][1]);
    }
  }
  return 0;
}

#define BATT_SAMPLE_SECS   60    // ein Messpunkt pro Minute (auch bei dunklem Display)
#define BATT_WINDOW        61    // 60 min Fenster
#define BATT_MIN_SECS    1200    // Schaetzung erst ab 20 min Daten
#define BATT_JUMP_MV       60    // Sprung zwischen zwei Messpunkten -> Neustart (Stecker, Akkuwechsel)

class Trend {
  uint32_t _t[BATT_WINDOW]; float _p[BATT_WINDOW]; int16_t _mv[BATT_WINDOW];
  int _n = 0, _head = 0;   // _head = naechster Schreibplatz
public:
  void reset() { _n = 0; _head = 0; }
  int count() const { return _n; }
  // t in Sekunden (monoton), mv gemessene Spannung
  void add(uint32_t t, int mv) {
    if (_n > 0) {
      int last = (_head + BATT_WINDOW - 1) % BATT_WINDOW;
      int d = mv - _mv[last]; if (d < 0) d = -d;
      if (d > BATT_JUMP_MV || t < _t[last]) reset();
    }
    _t[_head] = t; _p[_head] = percentFromMv(mv); _mv[_head] = (int16_t)mv;
    _head = (_head + 1) % BATT_WINDOW; if (_n < BATT_WINDOW) _n++;
  }
  uint32_t spanSecs() const {
    if (_n < 2) return 0;
    int first = (_head + BATT_WINDOW - _n) % BATT_WINDOW, last = (_head + BATT_WINDOW - 1) % BATT_WINDOW;
    return _t[last] - _t[first];
  }
  // Steigung in %/h (negativ = Entladen) und geglaetteter Ladestand jetzt; false solange zu wenig Daten
  bool slope(float& pct_per_h, float& pct_now) const {
    if (_n < 10 || spanSecs() < BATT_MIN_SECS) return false;
    int first = (_head + BATT_WINDOW - _n) % BATT_WINDOW;
    double t0 = _t[first], st = 0, sp = 0, stt = 0, stp = 0;
    for (int i = 0; i < _n; i++) {
      int k = (first + i) % BATT_WINDOW; double x = (_t[k] - t0) / 3600.0, y = _p[k];
      st += x; sp += y; stt += x * x; stp += x * y;
    }
    double den = _n * stt - st * st; if (den <= 0) return false;
    double b = (_n * stp - st * sp) / den, a = (sp - b * st) / _n;
    int last = (_head + BATT_WINDOW - 1) % BATT_WINDOW;
    pct_per_h = (float)b; pct_now = (float)(a + b * (_t[last] - t0) / 3600.0);
    if (pct_now < 0) pct_now = 0;
    if (pct_now > 100) pct_now = 100;
    return true;
  }
};

// Momentanwerte. Unbekanntes = -1 (T-Deck: Laden/USB nicht messbar).
struct Now {
  int mv = 0;          // Akkuspannung, 0 = kein Akku erkannt
  int pct = -1;        // Ladestand in % (T-Watch: Ladestandsanzeige des AXP2101, T-Deck: aus der Spannung)
  bool pct_approx = true;  // true: aus Spannung geschaetzt ("ca.")
  int usb = -1;        // 1 USB-Spannung da, 0 nein, -1 unbekannt
  int charging = -1;   // 1 laedt, 0 nein, -1 unbekannt
};

inline void fmt1(char* o, size_t n, float v) {        // 2,1  (Dezimalkomma)
  int t = (int)(v * 10 + 0.5f); snprintf(o, n, "%d,%d", t / 10, t % 10);
}

// Texte fuer die Statuszeile. left: Ladestand + Spannung, est: Verbrauch/Restzeit oder Zustand.
// variant: 0 lang, 1 mittel, 2 kurz (die Anzeige nimmt die laengste, die passt).
//   0 "Verbr. ca. 5,0 %/h, Rest ca. 13 h"   1 "Verbr. ca. 5,0 %/h, Rest 13 h"   2 "ca. 5,0 %/h, Rest 13 h"
inline void texts(const Now& b, const Trend& tr, char* left, size_t nl, char* est, size_t ne, int variant = 0) {
  if (b.mv <= 0) {
    snprintf(left, nl, "kein Akku");
    snprintf(est, ne, "%s", b.usb == 1 ? "USB" : "");
    return;
  }
  int cv = (b.mv + 5) / 10; char v[24]; snprintf(v, sizeof(v), "%d,%02d V", cv / 100, cv % 100);
  if (b.pct >= 0) snprintf(left, nl, "%s%d %% %s", b.pct_approx ? "ca. " : "", b.pct, v);
  else snprintf(left, nl, "%s", v);
  if (b.charging == 1) { snprintf(est, ne, "USB: Laden"); return; }
  if (b.usb == 1) { snprintf(est, ne, "USB: kein Laden"); return; }
  float r, p;
  if (!tr.slope(r, p)) {
    uint32_t s = tr.spanSecs(); int m = (int)((BATT_MIN_SECS - (s < BATT_MIN_SECS ? s : BATT_MIN_SECS) + 59) / 60);
    snprintf(est, ne, variant ? "Verbrauch: noch %d min" : "Verbrauch: Messung, noch %d min", m);
    return;
  }
  if (r > 0.5f) { snprintf(est, ne, "Spannung steigt (Laden?)"); return; }
  float use = -r;
  if (use < 0.1f) { snprintf(est, ne, "Verbrauch unter 0,1 %%/h"); return; }
  char u[16]; fmt1(u, sizeof(u), use);
  float h = p / use;
  char rest[32], hv[16];
  const char* ca = variant == 0 ? "ca. " : "";
  if (h > 99) snprintf(rest, sizeof(rest), "mehr als 99 h");
  else if (h >= 10) snprintf(rest, sizeof(rest), "%s%d h", ca, (int)(h + 0.5f));
  else { fmt1(hv, sizeof(hv), h); snprintf(rest, sizeof(rest), "%s%s h", ca, hv); }
  snprintf(est, ne, variant == 2 ? "ca. %s %%/h, Rest %s" : "Verbr. ca. %s %%/h, Rest %s", u, rest);
}

}  // namespace batt
