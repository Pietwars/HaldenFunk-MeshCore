// DAR-MC web layer (DISASTER_PORTAL, see Frontend.h): WLAN access point + DisasterPortal web interface.
// Not part of the T-Deck Plus standalone build: platformio.standalone.ini removes this file via build_src_filter.
#include "Frontend.h"
#ifdef DISASTER_PORTAL

#include "OpenAccess.h"  // DISASTER_BLE_PIN
#include <Arduino.h>
#include <SPIFFS.h>
#include <helpers/MultiSerialInterface.h>
#include "MyMesh.h"
#include "DisasterPortal.h"

DisasterPortal disaster_portal;

#ifdef DISPLAY_CLASS
  #include "UITask.h"
  extern UITask ui_task;
#endif

bool frontend::mountFilesystem() {
  DIAG("SPIFFS.begin");
  if (!SPIFFS.begin(false)) {
    Serial.println("SPIFFS mount failed. Initialize the filesystem with uploadfs (after backup).");
    return false;
  }
  return true;
}

void frontend::begin(MultiSerialInterface& interfaces) {
  board.setInhibitSleep(true);
  DIAG("mesh ok, wait before WiFi");
#ifdef DISASTER_WIFI_DELAY_MS
  delay(DISASTER_WIFI_DELAY_MS);  // WLAN zuletzt und verzoegert (Brownout-Schutz)
#elif defined(DISASTER_DIAG)
  delay(1500);
#endif
  DIAG("portal.begin (softAP)");
  disaster_portal.begin();
  DIAG("portal ok");
  interfaces.addInterface(InterfaceType::Portal, &disaster_portal);
  // 0.9.3: von der App gesendete Nachrichten auch im Web-Verlauf zeigen
  the_mesh.setAppSentObserver([](const uint8_t *cmd, size_t len, const uint8_t *tag) { disaster_portal.recordAppSend(cmd, len, tag); });
}

void frontend::sensorsReady() {
#ifdef DISASTER_GPS
  sensors.setSettingValue("gps", "1");  // GPS profile: module on by default
#endif
}

void frontend::loop() {
  disaster_portal.serviceAP();
#ifdef DISPLAY_CLASS
  {
    static unsigned long lastOled = 0; static bool showPw = false;
    if (millis() - lastOled > 15000) {  // abwechselnd WLAN-Name und offener Zugang auf dem OLED
      lastOled = millis(); showPw = !showPw;
      String t = showPw ? (the_mesh.getBLEPin() == DISASTER_BLE_PIN ? String("WLAN offen / BLE-PIN 123456") : String("WLAN offen / BLE eigene PIN"))
                        : (disaster_portal.apRunning() ? String("WLAN ") : String("WLAN FEHLER ")) + disaster_portal.ssid().substring(16);
      ui_task.showAlert(t.c_str(), 6000);
    }
  }
#endif
  disaster_portal.setRadioDiagnostics(the_mesh.getNumSentFlood(), the_mesh.getNumSentDirect(),
    the_mesh.getNumRecvFlood(), the_mesh.getNumRecvDirect(), the_mesh.getTotalAirTime(),
    the_mesh.getReceiveAirTime(), the_mesh.getRemainingTxBudget(),
    the_mesh.isTxBudgetWaiting(), the_mesh.getTxBudgetWaitEvents(),
    the_mesh.getTxBudgetWaitSeconds(), the_mesh.getTxQueuePackets());
#if defined(DISASTER_GPS) && ENV_INCLUDE_GPS
  disaster_portal.setGps(true, nmea.isValid(), sensors.node_lat, sensors.node_lon);
#endif
}

#endif
