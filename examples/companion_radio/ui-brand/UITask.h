#pragma once
// Marken-Anzeige (HaldenFunk-MeshCore, uebernommen aus DisAHRsterRadio-MeshCore 0.9.4): Das Display zeigt NUR
// das HaldenFunk-Bild mit Schriftzug, eine Akku-Statuszeile (Ladestand, Spannung, geschaetzter Verbrauch, siehe
// BattTrend.h) und einen Knopf "Advert senden" (Flood-Advert). Nachrichten, Einstellungen usw. laufen
// ausschliesslich ueber die MeshCore-App (Bluetooth ohne PIN) und die Weboberflaeche.
// Keine PIN-Anzeige: HaldenFunk-MeshCore koppelt Bluetooth ohne PIN (offener Zugang).

#include <MeshCore.h>
#include <helpers/ui/DisplayDriver.h>
#include <helpers/SensorManager.h>
#include <helpers/MultiSerialInterface.h>
#include <Arduino.h>
#include "../AbstractUITask.h"
#include "../NodePrefs.h"
#include "BattTrend.h"

#ifndef BRAND_AUTO_OFF_MILLIS
  #define BRAND_AUTO_OFF_MILLIS   30000   // Display geht nach 30 s ohne Bedienung aus
#endif
#define BRAND_ADVERT_GAP_MILLIS   10000   // hoechstens ein Advert je 10 s
#define BRAND_STATUS_MILLIS        2000   // "Advert gesendet" so lange zeigen
#define BRAND_BATT_REFRESH_MILLIS 15000   // Akkuzeile bei hellem Display hoechstens so oft neu lesen/zeichnen

class UITask : public AbstractUITask {
  DisplayDriver* _display = NULL;
  NodePrefs* _node_prefs = NULL;
  unsigned long _auto_off = 0;
  unsigned long _last_advert = 0;
  bool _advert_sent_once = false;
  unsigned long _status_until = 0;
  uint8_t _status = 0;           // 0 keiner, 1 gesendet, 2 warten, 3 Fehler
  int _wait_secs = 0;
  bool _dirty = true;
  int _msgcount = 0;
  batt::Now _batt;
  batt::Trend _trend;
  unsigned long _next_sample = 0, _next_batt_draw = 0;
  int _last_usb = -1;
  char _bl[40] = "", _be[48] = "";   // zuletzt gezeichnete Akkutexte
#if defined(UI_HAS_TOUCH)
  bool _touch_down = false;
  int _touch_x = 0, _touch_y = 0;
#endif

  void wake();
  void toggleDisplay();
  void requestAdvert();
  void render();
  void buttonRect(int& x, int& y, int& w, int& h) const;
  int  stripHeight() const;
  void readBattery();
  void sampleBattery(unsigned long now);
  void drawBattery(bool push);

public:
  UITask(mesh::MainBoard* board, MultiSerialInterface* serial) : AbstractUITask(board, serial) { }
  void begin(DisplayDriver* display, SensorManager* sensors, NodePrefs* node_prefs);
  void showAlert(const char* text, int duration_millis) { }   // keine Hinweis-Popups: Anzeige bleibt fest
  int  getMsgCount() const { return _msgcount; }
  bool hasDisplay() const { return _display != NULL; }

  // MyMesh::Listener: Nachrichten werden nicht angezeigt (App/Weboberflaeche)
  void onMessageRecv(mesh::Packet *pkt, const ContactInfo &from, uint8_t txt_type, uint32_t sender_timestamp, const char* text) override { }
  void onChannelMessageRecv(mesh::Packet *pkt, ChannelDetails& channel_details, const char* text) override { }
  void onQueueSizeChanged(int offline_queue_size) override { _msgcount = offline_queue_size; }

  void notify(UIEventType t = UIEventType::none) override { }
  void loop() override;
};
