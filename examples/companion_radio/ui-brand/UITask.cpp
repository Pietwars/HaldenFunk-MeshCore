#include "UITask.h"
#include "../MyMesh.h"
#include "target.h"
#include "BrandLogo.h"
#ifdef PIN_USER_BTN
  #include <helpers/ui/MomentaryButton.h>
  // T-Deck-Trackball: kurz klicken = Display an/aus, 2,5 s halten = Flood-Advert. Kein Ausschalten.
  #ifndef BRAND_ADVERT_HOLD_MILLIS
    #define BRAND_ADVERT_HOLD_MILLIS 2500
  #endif
  static MomentaryButton brand_btn(PIN_USER_BTN, BRAND_ADVERT_HOLD_MILLIS, true, false, false);
#endif

#define BRAND_LOGO_Y 2   // Bild oben, volle Breite

// Farben (RGB565), HaldenFunk: Anthrazit/Kupfer wie Weboberflaeche und Titelbild
#define C_BG     0x1082   // Hintergrund Anthrazit (#141214)
#define C_COPPER 0xECAB   // Kupfer (#e8955a), Knopf "Advert senden"
#define C_GREEN  0x4EF0   // Akku gut / "Advert gesendet"
#define C_WHITE  0xFFFF
#define C_GREY   0x8410
#define C_ORANGE 0xFD20
#define C_RED    0xF800

void UITask::begin(DisplayDriver* display, SensorManager* sensors, NodePrefs* node_prefs) {
  _display = display;
  _node_prefs = node_prefs;
#ifdef PIN_USER_BTN
  brand_btn.begin();
#endif
  readBattery();
  sampleBattery(millis());
  if (_display) _display->turnOn();
  _auto_off = millis() + BRAND_AUTO_OFF_MILLIS;
  _dirty = true;
}

// ---- Akku ---------------------------------------------------------------------------------
// Momentanwerte lesen. T-Watch S3: AXP2101 (Spannung, Ladestand-%, USB, Laden).
// T-Deck Plus: nur die Spannung am ADC; Prozent daraus geschaetzt, USB/Laden nicht messbar.
void UITask::readBattery() {
  batt::Now b;
#if defined(LILYGO_TWATCH_S3)
  int mv = 0, pct = -1, usb = -1, chg = -1;
  if (board.readPower(mv, pct, usb, chg)) {
    b.mv = mv; b.pct = pct > 100 ? 100 : pct; b.pct_approx = false; b.usb = usb; b.charging = chg;
    if (b.mv > 0 && b.pct < 0) { b.pct = (int)(batt::percentFromMv(b.mv) + 0.5f); b.pct_approx = true; }
  }
#else
  int mv = board.getBattMilliVolts();
  if (mv >= 1000) { b.mv = mv; b.pct = (int)(batt::percentFromMv(mv) + 0.5f); b.pct_approx = true; }
#endif
  _batt = b;
}

// Ein Messpunkt pro Minute fuer den Trend, auch bei dunklem Display (eine ADC- bzw. I2C-Abfrage).
void UITask::sampleBattery(unsigned long now) {
  if (_next_sample && (long)(now - _next_sample) < 0) return;
  _next_sample = now + BATT_SAMPLE_SECS * 1000UL;
  readBattery();
  if (_batt.usb != _last_usb) { _trend.reset(); _last_usb = _batt.usb; }   // Stecker rein/raus
  if (_batt.mv <= 0 || _batt.charging == 1 || _batt.usb == 1) { _trend.reset(); return; }
  _trend.add(now / 1000, _batt.mv);
}

int UITask::stripHeight() const {
  int lines = _display->nativeWidth() <= 240 ? 2 : 1;
  return lines * _display->nativeTextHeight(1) + (lines - 1) + 2;
}

// Statuszeile zwischen Bild und Knopf: Akkusymbol, Ladestand + Spannung, Verbrauch (Schaetzung).
// T-Deck: eine Zeile (Verbrauch rechtsbuendig), T-Watch: zwei Zeilen (Verbrauch zentriert darunter).
void UITask::drawBattery(bool push) {
  DisplayDriver* d = _display;
  int W = d->nativeWidth(); bool small = W <= 240;
  const int m = 4;
  int y = BRAND_LOGO_Y + BRAND_LOGO_H + 2, h = stripHeight(), th = d->nativeTextHeight(1);
  batt::texts(_batt, _trend, _bl, sizeof(_bl), _be, sizeof(_be), 0);
  d->nativeFill(0, y, W, h, C_BG);
  // Akkusymbol, Fuellung nach Ladestand (gruen ab 40 %, orange ab 15 %, sonst rot)
  int iw = small ? 22 : 18, ih = small ? 12 : 8, iy = y + 1 + (th - ih) / 2;
  d->nativeRect(m, iy, iw, ih, C_WHITE);
  d->nativeFill(m + iw, iy + ih / 4, 2, ih - 2 * (ih / 4), C_WHITE);
  if (_batt.pct >= 0) {
    uint16_t c = _batt.pct >= 40 ? C_GREEN : (_batt.pct >= 15 ? C_ORANGE : C_RED);
    int fw = (iw - 4) * _batt.pct / 100; if (fw < 1 && _batt.pct > 0) fw = 1;
    if (fw > 0) d->nativeFill(m + 2, iy + 2, fw, ih - 4, c);
  }
  int tx = m + iw + 2 + (small ? 6 : 4);
  d->nativeText(tx, y + 1, 1, C_WHITE, _bl);
  // Verbrauch: laengste Textvariante, die passt
  char est[48], l[40]; est[0] = 0;
  int avail = small ? W - 2 * m : W - m - (tx + d->nativeTextWidth(_bl, 1) + 8);
  for (int v = 0; v < 3; v++) {
    batt::texts(_batt, _trend, l, sizeof(l), est, sizeof(est), v);
    if (d->nativeTextWidth(est, 1) <= avail) break;
    if (v == 2) est[0] = 0;
  }
  if (est[0]) {
    int ew = d->nativeTextWidth(est, 1);
    if (small) d->nativeText((W - ew) / 2, y + 1 + th + 1, 1, C_GREY, est);
    else d->nativeText(W - m - ew, y + 1, 1, C_GREY, est);
  }
  if (push) d->endFrame();
}

void UITask::wake() {
  if (!_display) return;
  if (!_display->isOn()) { _display->turnOn(); readBattery(); _next_batt_draw = millis() + BRAND_BATT_REFRESH_MILLIS; }
  _auto_off = millis() + BRAND_AUTO_OFF_MILLIS;
  _dirty = true;
}

void UITask::toggleDisplay() {
  if (!_display) return;
  if (_display->isOn()) _display->turnOff();
  else wake();
}

void UITask::requestAdvert() {
  wake();
  unsigned long now = millis();
  if (_advert_sent_once && now - _last_advert < BRAND_ADVERT_GAP_MILLIS) {
    _status = 2;
    _wait_secs = (int)((BRAND_ADVERT_GAP_MILLIS - (now - _last_advert) + 999) / 1000);
  } else {
    bool ok = the_mesh.floodAdvert();
    _status = ok ? 1 : 3;
    if (ok) { _last_advert = now; _advert_sent_once = true; }
  }
  _status_until = now + BRAND_STATUS_MILLIS;
  _dirty = true;
}

void UITask::buttonRect(int& x, int& y, int& w, int& h) const {
  // Bild moeglichst gross (volle Breite), darunter der Knopf bis kurz vor den unteren Rand
  int W = _display->nativeWidth(), H = _display->nativeHeight();
  x = W / 20; w = W - 2 * x; y = BRAND_LOGO_Y + BRAND_LOGO_H + 2 + stripHeight() + 2; h = H - y - 5;
}

static void centerText(DisplayDriver* d, int cx, int y, int size, uint16_t c, const char* s) {
  d->nativeText(cx - d->nativeTextWidth(s, size) / 2, y, size, c, s);
}

void UITask::render() {
  DisplayDriver* d = _display;
  int W = d->nativeWidth();
  bool small = W <= 240;
  d->startFrame(C_BG);
  d->nativeImage((W - BRAND_LOGO_W) / 2, BRAND_LOGO_Y, BRAND_LOGO_W, BRAND_LOGO_H, BRAND_LOGO);
  drawBattery(false);

  int bx, by, bw, bh; buttonRect(bx, by, bw, bh);
  int big = small ? 2 : 3;
  int hs = d->nativeTextHeight(1), hb = d->nativeTextHeight(big);
  if (_status && millis() < _status_until) {
    char t[32];
    uint16_t c = _status == 1 ? C_GREEN : (_status == 2 ? C_ORANGE : C_RED);
    const char* s = _status == 1 ? "Advert gesendet" : (_status == 2 ? t : "Advert-Fehler");
    if (_status == 2) snprintf(t, sizeof(t), "Bitte %d s warten", _wait_secs);
    d->nativeFill(bx, by, bw, bh, c);
    centerText(d, W / 2, by + (bh - hb) / 2, big, C_BG, s);
  } else {
    d->nativeFill(bx, by, bw, bh, C_COPPER);
    d->nativeRect(bx, by, bw, bh, C_WHITE);
    int gap = (bh - hb - hs) / 3; if (gap < 1) gap = 1;
    centerText(d, W / 2, by + gap, big, C_BG, "Advert senden");
#if defined(UI_HAS_TOUCH)
    centerText(d, W / 2, by + 2 * gap + hb, 1, C_BG, "antippen");
#else
    centerText(d, W / 2, by + 2 * gap + hb, 1, C_BG, "Trackball 2,5 s halten");
#endif
  }
  d->endFrame();
}

void UITask::loop() {
  if (!_display) return;
  unsigned long now = millis();

#ifdef PIN_USER_BTN
  int ev = brand_btn.check();
  if (ev == BUTTON_EVENT_CLICK) toggleDisplay();
  else if (ev == BUTTON_EVENT_LONG_PRESS) requestAdvert();
#endif

#if defined(LILYGO_TWATCH_S3)
  int k = board.pollPowerKey();
  if (k == PKEY_SHORT) toggleDisplay();
  else if (k == PKEY_LONG) requestAdvert();
#endif

#if defined(UI_HAS_TOUCH)
  {
    int tx, ty;
    bool down = _display->getTouch(&tx, &ty);
    if (down) {
      if (!_touch_down) { _touch_x = tx; _touch_y = ty; }
      _touch_down = true;
    } else if (_touch_down) {
      _touch_down = false;
      if (!_display->isOn()) {
        wake();                                   // erster Tipp weckt nur
      } else {
        int bx, by, bw, bh; buttonRect(bx, by, bw, bh);
        if (_touch_x >= bx && _touch_x < bx + bw && _touch_y >= by && _touch_y < by + bh) requestAdvert();
        else _auto_off = now + BRAND_AUTO_OFF_MILLIS;
      }
    }
  }
#endif

  if (_status && now >= _status_until) { _status = 0; _dirty = true; }
  sampleBattery(now);

  if (_display->isOn()) {
    if ((long)(now - _auto_off) > 0) {
      _display->turnOff();
    } else if (_dirty) {
      _dirty = false;
      render();
      _next_batt_draw = now + BRAND_BATT_REFRESH_MILLIS;
    } else if ((long)(now - _next_batt_draw) >= 0) {
      // nur bei hellem Display: Akkuwerte neu lesen, Statuszeile nur bei geaendertem Text neu zeichnen
      _next_batt_draw = now + BRAND_BATT_REFRESH_MILLIS;
      char ol[40], oe[48]; strcpy(ol, _bl); strcpy(oe, _be);
      readBattery();
      char nl[40], ne[48]; batt::texts(_batt, _trend, nl, sizeof(nl), ne, sizeof(ne), 0);
      if (strcmp(ol, nl) || strcmp(oe, ne)) drawBattery(true);
    }
  }
}
