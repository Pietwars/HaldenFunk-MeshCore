#include "UITask.h"
#include "../MyMesh.h"
#include "target.h"
#include "BrandLogo.h"
#include <helpers/esp32/SerialBLEInterface.h>
#ifdef PIN_USER_BTN
  #include <helpers/ui/MomentaryButton.h>
  // T-Deck-Trackball: kurz klicken = Display an/aus, 2,5 s halten = Flood-Advert. Kein Ausschalten.
  #ifndef BRAND_ADVERT_HOLD_MILLIS
    #define BRAND_ADVERT_HOLD_MILLIS 2500
  #endif
  static MomentaryButton brand_btn(PIN_USER_BTN, BRAND_ADVERT_HOLD_MILLIS, true, false, false);
#endif

#define BRAND_LOGO_Y 2   // Bild oben, volle Breite

#ifdef DAR_STANDALONE_UI
  #include <Wire.h>
  #include <Preferences.h>
  extern const uint8_t tdeckSplash[] asm("_binary_examples_companion_radio_ui_brand_assets_tdeck_splash_rgb565_start");
  // Trackball directions of the T-Deck (Hall sensors, one pulse per step), counted in interrupts.
  static volatile uint16_t tb_count[4] = {0, 0, 0, 0};   // up, down, left, right
  static portMUX_TYPE tb_mux = portMUX_INITIALIZER_UNLOCKED;
  static void IRAM_ATTR tb_pulse(int i) {
    portENTER_CRITICAL_ISR(&tb_mux); if(tb_count[i]<32)++tb_count[i]; portEXIT_CRITICAL_ISR(&tb_mux);
  }
  static void IRAM_ATTR tb_up() { tb_pulse(0); }
  static void IRAM_ATTR tb_down() { tb_pulse(1); }
  static void IRAM_ATTR tb_left() { tb_pulse(2); }
  static void IRAM_ATTR tb_right() { tb_pulse(3); }
  #define TDECK_KEYBOARD_ADDR 0x55   // keyboard MCU on SDA 18 / SCL 8, character mode
  #include "TDeckTouchRaw.h"
  static TDeckTouch tdeck_touch;
  static bool tdeck_touch_found=false;
#endif

// Farben (RGB565)
#define C_BG     0x08A3   // Hintergrund wie das Titelbild (#0b141c)
#define C_GREEN  0x4EF0   // Markengruen
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
#ifdef DAR_STANDALONE_UI
  _sui.begin(&dev::api());
  _sui.setSplash(tdeckSplash);   // boot logo, then Chat
  pushBattery();
  Wire.setTimeOut(20);
  tdeck_touch_found=tdeck_touch.begin();
  Preferences touchPrefs;
  if(touchPrefs.begin("dar-mc-touch",true)) {
    _touch_cal_loaded=touchPrefs.getBytesLength("cal-v1")==sizeof(_touch_transform) &&
      touchPrefs.getBytes("cal-v1",&_touch_transform,sizeof(_touch_transform))==sizeof(_touch_transform) && _touch_transform.valid();
    touchPrefs.end();
  }
  if(!_touch_cal_loaded)_touch_transform=TDeckTouchTransform();
  _touch_cal_needed=tdeck_touch_found && !_touch_cal_loaded;
  const uint8_t pins[4] = {PIN_TB_UP, PIN_TB_DOWN, PIN_TB_LEFT, PIN_TB_RIGHT};
  void (*isr[4])() = {tb_up, tb_down, tb_left, tb_right};
  for (int i = 0; i < 4; i++) { pinMode(pins[i], INPUT_PULLUP); attachInterrupt(digitalPinToInterrupt(pins[i]), isr[i], CHANGE); }
#endif
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

#ifdef DAR_STANDALONE_UI
// Battery level/consumption for the status line (every page) and the Gerät page (replaces the Start battery line).
void UITask::pushBattery() {
  char l[48], e[48]; batt::texts(_batt, _trend, l, sizeof(l), e, sizeof(e), 0);
  _sui.setBattery(_batt.pct, l, e);
}
#endif

void UITask::wake() {
  if (!_display) return;
  if (!_display->isOn()) {
    _display->turnOn(); readBattery(); _next_batt_draw = millis() + BRAND_BATT_REFRESH_MILLIS;
#ifdef DAR_STANDALONE_UI
    pushBattery();
#endif
  }
  _auto_off = millis() + BRAND_AUTO_OFF_MILLIS;
  _dirty = true;
}

void UITask::toggleDisplay() {
  if (!_display) return;
  if (_display->isOn() && !_pairing) _display->turnOff();
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
#ifdef DAR_STANDALONE_UI
  char t[40];
  if (_status == 2) snprintf(t, sizeof(t), "Advert: bitte %d s warten", _wait_secs);
  _sui.notify(_status == 1 ? "Advert gesendet" : _status == 2 ? t : "Advert-Fehler");
#endif
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
#ifdef DAR_STANDALONE_UI
  if(!_pairing && _touch_calibrating) {renderCalibration(); return;}
  if (!_pairing) { _sui.render(d); return; }      // boot logo or page; the PIN screen below stays as before
#endif
  d->startFrame(C_BG);
  d->nativeImage((W - BRAND_LOGO_W) / 2, BRAND_LOGO_Y, BRAND_LOGO_W, BRAND_LOGO_H, BRAND_LOGO);
  drawBattery(false);

  int bx, by, bw, bh; buttonRect(bx, by, bw, bh);
  int big = small ? 2 : 3;
  int hs = d->nativeTextHeight(1), hb = d->nativeTextHeight(big);
  if (_pairing) {
    char pin[16]; snprintf(pin, sizeof(pin), "%06lu", (unsigned long)the_mesh.getBLEPin());
    int sp = 3, hp = d->nativeTextHeight(sp);
    d->nativeFill(bx, by, bw, bh, C_WHITE);
    centerText(d, W / 2, by + 2, 1, C_BG, "Bluetooth-Kopplung - PIN:");
    centerText(d, W / 2, by + (bh - hp) / 2 + 2, sp, C_BG, pin);
    centerText(d, W / 2, by + bh - hs - 1, 1, C_GREY, "am Handy eingeben");
  } else if (_status && millis() < _status_until) {
    char t[32];
    uint16_t c = _status == 1 ? C_GREEN : (_status == 2 ? C_ORANGE : C_RED);
    const char* s = _status == 1 ? "Advert gesendet" : (_status == 2 ? t : "Advert-Fehler");
    if (_status == 2) snprintf(t, sizeof(t), "Bitte %d s warten", _wait_secs);
    d->nativeFill(bx, by, bw, bh, c);
    centerText(d, W / 2, by + (bh - hb) / 2, big, C_BG, s);
  } else {
    d->nativeFill(bx, by, bw, bh, C_GREEN);
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
#ifdef DAR_STANDALONE_UI
  // click = Enter on the pages (map: "Zurück"); dark display: the click only wakes it
  if (ev == BUTTON_EVENT_CLICK) {
    if(_touch_calibrating && !_pairing)cancelCalibration();
    else if (_display->isOn() && !_pairing && _sui.click()) input();
    else toggleDisplay();
  }
#else
  if (ev == BUTTON_EVENT_CLICK) toggleDisplay();
#endif
  else if (ev == BUTTON_EVENT_LONG_PRESS) {
#ifdef DAR_STANDALONE_UI
    if(!_touch_calibrating)
#endif
      requestAdvert();
  }
#endif

#if defined(LILYGO_TWATCH_S3)
  int k = board.pollPowerKey();
  if (k == PKEY_SHORT) toggleDisplay();
  else if (k == PKEY_LONG) requestAdvert();
#endif

#if defined(UI_HAS_TOUCH) && !defined(DAR_STANDALONE_UI)
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
        if (!_pairing && _touch_x >= bx && _touch_x < bx + bw && _touch_y >= by && _touch_y < by + bh) requestAdvert();
        else _auto_off = now + BRAND_AUTO_OFF_MILLIS;
      }
    }
  }
#endif

  bool pairing = ble_pairing_active();
  if (pairing != _pairing) {
    _pairing = pairing;
    if (pairing) wake();                          // PIN nur waehrend der Kopplung zeigen
    _dirty = true;
  }
  if (_status && now >= _status_until) { _status = 0; _dirty = true; }
  sampleBattery(now);
#ifdef DAR_STANDALONE_UI
  pollKeyboard(now);
  pollTrackball(now);
  pollTouch(now);
  if (_sui.takeDisplayOff() && !_pairing) _display->turnOff();   // Gerät "Display aus" or key 'd'
  if (_sui.tick(now)) _dirty = true;
  if(_sui.takeTouchCalibration()) {
    if(tdeck_touch_found)beginCalibration();
    else {_sui.notify("Touch nicht erkannt"); input();}
  }
  if(_touch_cal_needed && !_sui.onBoot() && !_pairing)beginCalibration();
  if(_touch_calibrating)_auto_off=now+BRAND_AUTO_OFF_MILLIS;
  if (_sui.takeIncoming()) wake();                // incoming message: display on (unread badge)
#endif

  if (_display->isOn()) {
    if (_pairing) _auto_off = now + BRAND_AUTO_OFF_MILLIS;
    if ((long)(now - _auto_off) > 0) {
      _display->turnOff();
    } else if (_dirty) {
      _dirty = false;
#ifdef DAR_STANDALONE_UI
      if (!_pairing && !_touch_calibrating && !_sui.onBoot() && _sui.pending() == sui::UI::PART) _sui.renderPart(_display);
      else
#endif
      render();
      _next_batt_draw = now + BRAND_BATT_REFRESH_MILLIS;
    } else if ((long)(now - _next_batt_draw) >= 0) {
      // nur bei hellem Display: Akkuwerte neu lesen, Statuszeile nur bei geaendertem Text neu zeichnen
      _next_batt_draw = now + BRAND_BATT_REFRESH_MILLIS;
#ifdef DAR_STANDALONE_UI
      // standalone: battery in the status line and on Gerät; the UI asks for a redraw only if a shown value changed
      readBattery(); pushBattery();
      if (_sui.pending() != sui::UI::NONE) _dirty = true;
#else
      char ol[40], oe[48]; strcpy(ol, _bl); strcpy(oe, _be);
      readBattery();
      char nl[40], ne[48]; batt::texts(_batt, _trend, nl, sizeof(nl), ne, sizeof(ne), 0);
      if (strcmp(ol, nl) || strcmp(oe, ne)) drawBattery(true);
#endif
    }
  }
}

#ifdef DAR_STANDALONE_UI
void UITask::input() {
  _auto_off = millis() + BRAND_AUTO_OFF_MILLIS;
  _dirty = true;
}

void UITask::pollKeyboard(unsigned long now) {
  if ((long)(now - _next_keyboard) < 0) return;
  if (!_keyboard_present) {
    _next_keyboard = now + 5000;
    Wire.beginTransmission(TDECK_KEYBOARD_ADDR);
    _keyboard_present = Wire.endTransmission() == 0;
    if (_keyboard_present) {
      // character mode, in case an earlier firmware left the keyboard in raw matrix mode
      Wire.beginTransmission(TDECK_KEYBOARD_ADDR); Wire.write(uint8_t(0x04)); Wire.endTransmission();
      Serial.println("[TDECK] Tastatur erkannt (I2C 0x55, Zeichenmodus)");
    }
    return;
  }
  _next_keyboard = now + 30;
  if (Wire.requestFrom(uint8_t(TDECK_KEYBOARD_ADDR), uint8_t(1)) != 1) {
    _keyboard_present = false; _next_keyboard = now + 5000;
    Serial.println("[TDECK] Tastatur nicht erreichbar; erneuter Versuch in 5 s");
    return;
  }
  uint8_t key = Wire.read();
  if (!key || _pairing) return;
  if(_touch_calibrating) {if(key==sui::K_ESC)cancelCalibration(); return;}
  if (!_display->isOn()) { wake(); return; }      // a key only wakes the display, never sends
  // The keyboard sends 0x08 for Backspace with and without ALT. ALT+Backspace = "back": ask the keyboard for its
  // key matrix (raw mode 0x03, LilyGO keyboard firmware from 2025-06) while the keys are still held.
  if (key == sui::K_BACKSPACE && altHeld()) key = sui::K_BACK;
  _sui.key(key);
  input();
}

// T-Deck Plus touch panel (GT911): a tap acts on release at the press position; dark display: wake only.
void UITask::pollTouch(unsigned long now) {
  if ((long)(now-_next_touch)<0) return;
  _next_touch=now+20;
  if(!tdeck_touch_found) {
    if((long)(now-_next_touch_probe)>=0) {
      _next_touch_probe=now+5000; tdeck_touch_found=tdeck_touch.begin();
      if(tdeck_touch_found&&!_touch_cal_loaded)_touch_cal_needed=true;
    }
    return;
  }
  bool down=false; int x=0,y=0;
  if(!tdeck_touch.sample(down,x,y))return;
  if(down) {
    if (!_touch_down) {
      _touch_x = x;
      _touch_y = y;
      _touch_started = now;
      _touch_valid = x >= 0 && y >= 0;
      _touch_travelled = false;
      _touch_epoch = _sui.touchEpoch();
    }
    if(x<0||y<0)_touch_valid=false;
    if (_touch_valid && !_touch_calibrating) {
      int sx, sy, cx, cy;
      _touch_transform.map(_touch_x, _touch_y, sx, sy);
      _touch_transform.map(x, y, cx, cy);
      if (abs(cx - sx) > 12 || abs(cy - sy) > 12) _touch_travelled = true;
    }
    _touch_last_x=x;_touch_last_y=y;_touch_down=true; return;
  }
  if(!_touch_down)return;
  _touch_down=false;
  if (_pairing || !_touch_valid || now - _touch_started < 25 ||
      now - _touch_started > (_touch_calibrating ? 5000u : 2000u))
    return;
  if(!_display->isOn()){wake();return;}
  // Calibration is deliberately slower than a normal tap: allow a careful press.
  if(_touch_calibrating){calibrationTap(_touch_last_x,_touch_last_y);return;}
  if (_touch_epoch != _sui.touchEpoch()) return; // content changed beneath the finger: start a new gesture
  int sx,sy,ex,ey;
  _touch_transform.map(_touch_x,_touch_y,sx,sy);
  _touch_transform.map(_touch_last_x,_touch_last_y,ex,ey);
  if(sx<0||sx>=320||sy<0||sy>=240)return;
  if (_sui.gesture(sx, sy, ex, ey, _touch_travelled, now - _touch_started))
    input();
  else _auto_off=now+BRAND_AUTO_OFF_MILLIS;
}

void UITask::beginCalibration() {
  _touch_cal_needed=false;_touch_calibrating=true;_touch_step=0;
  _touch_down=false;_cal_accept_at=millis()+300;wake();input();
}
void UITask::cancelCalibration() {
  _touch_calibrating=false;_touch_cal_needed=false;_touch_down=false;
  _sui.forceRedraw();_sui.notify("Kalibrierung abgebrochen");input();
}
void UITask::calibrationTap(int x,int y) {
  if((long)(millis()-_cal_accept_at)<0)return;
  _cal_accept_at=millis()+250;
  if(_touch_step<3) {
    _cal_x[_touch_step]=x;_cal_y[_touch_step]=y;++_touch_step;
    if(_touch_step==3&&!_pending_transform.fit(_cal_x,_cal_y))_touch_step=0;
    input();return;
  }
  int sx,sy;_pending_transform.map(x,y,sx,sy);
  if(abs(sx-160)>20||abs(sy-120)>20){_touch_step=0;input();return;}
  _touch_transform=_pending_transform;
  Preferences prefs;bool saved=false;
  if(prefs.begin("dar-mc-touch",false)) {
    saved=prefs.putBytes("cal-v1",&_touch_transform,sizeof(_touch_transform))==sizeof(_touch_transform);prefs.end();
  }
  _touch_cal_loaded=saved;_touch_calibrating=false;_sui.forceRedraw();
  _sui.notify(saved?"Touch gespeichert":"Touch nur bis Neustart gespeichert");input();
}
void UITask::renderCalibration() {
  static const int px[4]={24,296,24,160},py[4]={44,44,196,120};
  const int x=px[_touch_step],y=py[_touch_step];
  _display->startFrame(C_BG);
  _display->nativeText(45,5,1,C_WHITE,"Touch kalibrieren");
  char progress[40];
  snprintf(progress,sizeof(progress),"%u/4: Kreuz antippen",unsigned(_touch_step)+1);
  _display->nativeText(45,203,1,C_WHITE,progress);
  _display->nativeText(45,219,1,C_GREY,"Klick / Esc: abbrechen");
  _display->nativeLine(x-10,y,x+10,y,C_GREEN);
  _display->nativeLine(x,y-10,x,y+10,C_GREEN);
  _display->endFrame();
}

// Raw matrix of the T-Deck keyboard: 5 bytes (one per column, bit = row). ALT = column 0 row 4, Backspace =
// column 4 row 3. Both must be down and every byte must look like a matrix (7 rows), else plain Backspace.
// Old keyboard firmware without raw mode answers with its one-byte character mode: then ALT is never seen and
// Backspace on an empty field / outside text fields still goes back. Character mode is restored in any case.
bool UITask::altHeld() {
  uint8_t m[5] = {0, 0, 0, 0, 0}; uint8_t n = 0;
  Wire.beginTransmission(TDECK_KEYBOARD_ADDR); Wire.write(uint8_t(0x03));
  if (Wire.endTransmission() == 0 && Wire.requestFrom(uint8_t(TDECK_KEYBOARD_ADDR), uint8_t(5)) == 5)
    for (; n < 5 && Wire.available(); ++n) m[n] = uint8_t(Wire.read());
  while (Wire.available()) Wire.read();
  Wire.beginTransmission(TDECK_KEYBOARD_ADDR); Wire.write(uint8_t(0x04)); Wire.endTransmission();
  if (n != 5) return false;
  for (int i = 0; i < 5; ++i) if (m[i] & 0x80) return false;
  bool alt = m[0] & (1 << 4), bksp = m[4] & (1 << 3);
  if (alt && bksp) Serial.println("[TDECK] ALT+Backspace: zurueck");
  return alt && bksp;
}

void UITask::pollTrackball(unsigned long now) {
  uint16_t c[4];
  portENTER_CRITICAL(&tb_mux);
  for(int i=0;i<4;++i){c[i]=tb_count[i];tb_count[i]=0;}
  portEXIT_CRITICAL(&tb_mux);
  if(_pairing||_touch_calibrating){_ball_filter=TDeckTrackball();return;}
  auto action=_ball_filter.update(now,int(c[3])-int(c[2]),int(c[1])-int(c[0]),false);
  if(action==TDeckTrackball::NONE)return;
  if(!_display->isOn()){wake();return;}
  int dx=action==TDeckTrackball::LEFT?-1:action==TDeckTrackball::RIGHT?1:0;
  int dy=action==TDeckTrackball::UP?-1:action==TDeckTrackball::DOWN?1:0;
  if(_sui.move(dx,dy))input();
  else _auto_off=now+BRAND_AUTO_OFF_MILLIS;
}
#endif
