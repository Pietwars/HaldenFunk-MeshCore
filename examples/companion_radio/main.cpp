#include <Arduino.h>   // needed for PlatformIO
#include <Mesh.h>
#include "MyMesh.h"

// Believe it or not, this std C function is busted on some platforms!
static uint32_t _atoi(const char* sp) {
  uint32_t n = 0;
  while (*sp && *sp >= '0' && *sp <= '9') {
    n *= 10;
    n += (*sp++ - '0');
  }
  return n;
}

#ifdef DISASTER_PORTAL
#include "DisasterPortal.h"
DisasterPortal disaster_portal;
#include "DisasterBootGuard.h"
disaster::BootGuard boot_guard;
#endif

// interface manager
#include <helpers/MultiSerialInterface.h>
MultiSerialInterface interface_manager;

// include bluetooth interface
#if defined(BLE_PIN_CODE)
  #if defined(ESP32) && defined(DISASTER_NIMBLE)
    // klassische ESP32 mit WLAN-Portal: NimBLE (weniger Heap, Classic-BT-Speicher freigegeben)
    #include <helpers/esp32/SerialNimBLEInterface.h>
    SerialNimBLEInterface bluetooth_interface;
  #elif defined(ESP32)
    // include esp32 bluetooth interface
    #include <helpers/esp32/SerialBLEInterface.h>
    SerialBLEInterface bluetooth_interface;
  #elif defined(NRF52_PLATFORM)
    // include nrf52 bluetooth interface
    #include <helpers/nrf52/SerialBLEInterface.h>
    SerialBLEInterface bluetooth_interface;
  #elif defined(RP2040_PLATFORM)
    // include rp2040 (Pico W / CYW43) bluetooth interface
    #include <helpers/rp2040/SerialBLEInterface.h>
    SerialBLEInterface bluetooth_interface;
  #else
    #error "SerialBLEInterface is not defined for this platform"
  #endif
#endif

// include wifi interface
#ifdef ENABLE_WIFI_INTERFACE
  #ifndef WIFI_SSID
    #define WIFI_SSID ""
  #endif
  #ifndef WIFI_PWD
    #define WIFI_PWD ""
  #endif
  #ifndef TCP_PORT
    #define TCP_PORT 5000
  #endif
  #ifndef WIFI_RETRY_INTERVAL
    #if defined(RP2040_PLATFORM)
      #define WIFI_RETRY_INTERVAL 30000   // each attempt blocks loop(), so retry less often
    #else
      #define WIFI_RETRY_INTERVAL 10000   // millis between reconnect attempts
    #endif
  #endif
  #ifndef WIFI_RETRY_TIMEOUT
    #define WIFI_RETRY_TIMEOUT 5000     // RP2040: cap on how long one join may block loop()
  #endif
  #if defined(ESP32) || defined(RP2040_PLATFORM)
    #include <helpers/wifi/SerialWifiInterface.h>
    SerialWifiInterface wifi_interface;
  #else
    #error "SerialWifiInterface is not defined for this platform"
  #endif
#endif

// include usb interface
#if defined(ENABLE_USB_INTERFACE)
  #include <helpers/ArduinoSerialInterface.h>
  ArduinoSerialInterface usb_serial_interface;
#endif

// include ethernet interface
#if defined(ETHERNET_ENABLED)
  #include <helpers/ethernet/EthernetInterface.h>
  ETHERNET_CLASS ethernet_interface;
#endif

// include hardware serial interface
#if defined(SERIAL_RX)
  #include <helpers/ArduinoSerialInterface.h>
  ArduinoSerialInterface hardware_serial_interface;
  HardwareSerial companion_serial(1);
#endif

// platform file system
#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  #include <InternalFileSystem.h>
  #if defined(QSPIFLASH)
    #include <CustomLFS_QSPIFlash.h>
    DataStore store(InternalFS, QSPIFlash, rtc_clock);
  #else
    #if defined(EXTRAFS)
      #include <CustomLFS.h>
      CustomLFS ExtraFS(0xD4000, 0x19000, 128);
      DataStore store(InternalFS, ExtraFS, rtc_clock);
    #else
      DataStore store(InternalFS, rtc_clock);
    #endif
  #endif
#elif defined(RP2040_PLATFORM)
  #include <LittleFS.h>
  DataStore store(LittleFS, rtc_clock);
#elif defined(ESP32)
  #include <SPIFFS.h>
  DataStore store(SPIFFS, rtc_clock);
#endif

/* GLOBAL OBJECTS */
#ifdef DISPLAY_CLASS
  #include "UITask.h"
  UITask ui_task(&board, &interface_manager);
#endif

StdRNG fast_rng;
SimpleMeshTables tables;
MyMesh the_mesh(radio_driver, fast_rng, rtc_clock, tables, store);

/* END GLOBAL OBJECTS */

void halt() {
  while (1) ;
}

/* WIFI RECONNECT TRACKERS */
#ifdef ENABLE_WIFI_INTERFACE
  bool wifi_needs_reconnect = false;
  unsigned long last_wifi_reconnect_attempt = 0;
  char wifi_ssid[33] = WIFI_SSID;   // replaced by stored prefs at boot, if set
  char wifi_pwd[64] = WIFI_PWD;
  bool wifi_was_connected = false;
  bool wifi_enabled = false;   // set at boot from prefs; false also when the effective SSID is blank
#endif

#ifdef DISASTER_DIAG
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#define DIAG(m) do { Serial.printf("[DIAG %lu] %s heap=%u\n", millis(), m, (unsigned)ESP.getFreeHeap()); Serial.flush(); } while (0)
#else
#define DIAG(m) do {} while (0)
#endif

void setup() {
#ifdef DISASTER_DIAG
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
#endif
  Serial.begin(115200);
#ifdef DISASTER_DIAG
  delay(300);
  Serial.printf("\n[DIAG] boot, reset reason=%d\n", (int)esp_reset_reason());
  Serial.flush();
#endif
#ifdef DISASTER_PORTAL
  boot_guard.begin();
#endif
  DIAG("board.begin");
  board.begin();
  DIAG("board ok");

#ifdef HAS_EXTERNAL_WATCHDOG
  external_watchdog.begin();
#endif

#ifdef DISPLAY_CLASS
  DisplayDriver* disp = NULL;
  if (display.begin()) {
    disp = &display;
    disp->startFrame();
  #ifdef ST7789
    disp->setTextSize(2);
  #endif
    disp->drawTextCentered(disp->width() / 2, 28, "Loading...");
    disp->endFrame();
  }
#endif

  DIAG("display done, radio_init");
  if (!radio_init()) { DIAG("radio_init FAILED -> halt"); halt(); }
  DIAG("radio ok");

  fast_rng.begin(radio_driver.getRngSeed());

#if defined(NRF52_PLATFORM) || defined(STM32_PLATFORM)
  InternalFS.begin();
  #if defined(QSPIFLASH)
    if (!QSPIFlash.begin()) {
      // debug output might not be available at this point, might be too early. maybe should fall back to InternalFS here?
      MESH_DEBUG_PRINTLN("CustomLFS_QSPIFlash: failed to initialize");
    } else {
      MESH_DEBUG_PRINTLN("CustomLFS_QSPIFlash: initialized successfully");
    }
  #else
  #if defined(EXTRAFS)
      ExtraFS.begin();
  #endif
  #endif
  store.begin();
  the_mesh.begin();
#elif defined(RP2040_PLATFORM)
  LittleFS.begin();
  store.begin();
  the_mesh.begin();
#elif defined(ESP32)
#ifdef DISASTER_PORTAL
  DIAG("SPIFFS.begin");
  if (!SPIFFS.begin(false)) {
    Serial.println("SPIFFS mount failed. Initialize the filesystem with uploadfs (after backup).");
    halt();
  }
#else
  SPIFFS.begin(true);
#endif
  store.begin();
  the_mesh.begin();
#else
  #error "need to define filesystem"
#endif

#if defined(BLE_PIN_CODE) && defined(DISASTER_PORTAL)
  // HaldenFunk-MeshCore: zufaellige, pro Geraet dauerhaft gespeicherte BLE-PIN (seriell + Display)
  if (the_mesh.getNodePrefs()->ble_pin == 0) {
    uint32_t pin = 100000 + (esp_random() % 900000);
    the_mesh.getNodePrefs()->ble_pin = pin;
    the_mesh.savePrefs();
  }
  the_mesh.setBLEPin(the_mesh.getNodePrefs()->ble_pin);
  disaster_portal.setBlePin(the_mesh.getBLEPin());
  Serial.printf("[BLE] MeshCore-App: Bluetooth-PIN %06lu\n", (unsigned long)the_mesh.getBLEPin());
#elif defined(BLE_PIN_CODE) // 123456 by default
  if (the_mesh.getNodePrefs()->ble_pin == 0) {
#ifdef DISPLAY_CLASS
    if (disp != NULL && BLE_PIN_CODE == 123456) {
      StdRNG rng;
      the_mesh.setBLEPin(rng.nextInt(100000, 999999)); // random pin each session
    } else {
      the_mesh.setBLEPin(BLE_PIN_CODE); // otherwise static pin
    }
#else
    the_mesh.setBLEPin(BLE_PIN_CODE); // otherwise static pin
#endif
  } else {
    the_mesh.setBLEPin(the_mesh.getNodePrefs()->ble_pin);
  }
#else
  the_mesh.setBLEPin(0);
#endif

// add bluetooth interface
#if defined(BLE_PIN_CODE) && !defined(DISASTER_PORTAL)
  bluetooth_interface.begin(BLE_NAME_PREFIX, the_mesh.getNodePrefs()->node_name, the_mesh.getBLEPin());
  interface_manager.addInterface(InterfaceType::Bluetooth, &bluetooth_interface);
#endif
// DisAHRsterRadio: Bluetooth erst NACH dem WLAN-Portal und nur mit genug Heap (siehe unten).

// add wifi interface
#ifdef ENABLE_WIFI_INTERFACE
  // use wifi ssid and password from prefs if ssid is not empty, otherwise use the build flag defaults
  if (the_mesh.getNodePrefs()->wifi_ssid[0]) {
    strcpy(wifi_ssid, the_mesh.getNodePrefs()->wifi_ssid);
    strcpy(wifi_pwd, the_mesh.getNodePrefs()->wifi_pwd);
  }
  // only start wifi if enabled and ssid is not empty
  wifi_enabled = the_mesh.getNodePrefs()->wifi_enabled;
  if (wifi_enabled && wifi_ssid[0]) {
#if defined(ESP32)
    board.setInhibitSleep(true);   // prevent sleep when WiFi is active
    WiFi.setAutoReconnect(true);

    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info){
        if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
            WIFI_DEBUG_PRINTLN("WiFi disconnected (reason=%s). Flagging for reconnect...",
              WiFi.disconnectReasonName((wifi_err_reason_t)info.wifi_sta_disconnected.reason));
            wifi_needs_reconnect = true;
        } else if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
            WIFI_DEBUG_PRINTLN("WiFi connected successfully!");
            wifi_needs_reconnect = false;
        }
    });
#endif

    WIFI_DEBUG_PRINTLN("connecting to %s", wifi_ssid);

#if defined(RP2040_PLATFORM)
    // the join itself blocks inside the core (CYW43::begin busy-waits for the
    // association), so every attempt stalls the mesh loop. beginNoBlock() only skips the
    // extra DHCP wait. Give the first connect a full window, then bound the retries below.
    WiFi.beginNoBlock(wifi_ssid, wifi_pwd);
    last_wifi_reconnect_attempt = millis();   // let DHCP finish before the poll can retry
#else
    WiFi.begin(wifi_ssid, wifi_pwd);
#endif
    wifi_interface.begin(TCP_PORT);
    interface_manager.addInterface(InterfaceType::WiFi, &wifi_interface);
  } else {
    WIFI_DEBUG_PRINTLN("wifi disabled");
  }
#endif

// add usb interface
#if defined(ENABLE_USB_INTERFACE)
  usb_serial_interface.begin(Serial);
  interface_manager.addInterface(InterfaceType::USB, &usb_serial_interface);
#endif

// add ethernet interface
#if defined(ETHERNET_ENABLED)
  ethernet_interface.begin();
  interface_manager.addInterface(InterfaceType::Ethernet, &ethernet_interface);
#endif

// add hardware serial interface
#if defined(SERIAL_RX)
  companion_serial.setPins(SERIAL_RX, SERIAL_TX);
  companion_serial.begin(115200);
  hardware_serial_interface.begin(companion_serial);
  interface_manager.addInterface(InterfaceType::HardwareSerial, &hardware_serial_interface);
#endif

#ifdef DISASTER_PORTAL
  board.setInhibitSleep(true);
  DIAG("mesh ok, wait before WiFi");
#ifdef DISASTER_WIFI_DELAY_MS
  delay(DISASTER_WIFI_DELAY_MS);  // WLAN zuletzt und verzoegert (Brownout-Schutz)
#elif defined(DISASTER_DIAG)
  delay(1500);
#endif
  DIAG("portal.begin (softAP)");
  boot_guard.heap("vor WLAN");
  boot_guard.enter(disaster::STAGE_PORTAL);
#if defined(BLE_PIN_CODE)
  // WLAN+BLE-Koexistenz verlangt WLAN-Modem-Sleep (sonst IDF-Abbruch im STA-Betrieb); AP bleibt unberuehrt.
  disaster_portal.setModemSleep(!boot_guard.bleBlocked);
#endif
  disaster_portal.begin();
  DIAG("portal ok");
  boot_guard.heap("nach WLAN");
  interface_manager.addInterface(InterfaceType::WiFi, &disaster_portal);
#if defined(BLE_PIN_CODE)
  // Das WLAN-Portal hat Vorrang. Bluetooth (MeshCore-App) nur, wenn danach noch genug Heap bleibt
  // und der letzte Start nicht beim Bluetooth-Start abgestuerzt ist (sonst Bootschleife).
  if (boot_guard.allowBle()) {
    boot_guard.enter(disaster::STAGE_BLE);
    bluetooth_interface.begin(BLE_NAME_PREFIX, the_mesh.getNodePrefs()->node_name, the_mesh.getBLEPin());
    interface_manager.addInterface(InterfaceType::Bluetooth, &bluetooth_interface);
    boot_guard.enter(disaster::STAGE_RUN_BLE);
    boot_guard.heap("nach Bluetooth");
    Serial.printf("[BLE] MeshCore-App: Bluetooth an, PIN %06lu\n", (unsigned long)the_mesh.getBLEPin());
  } else {
    disaster_portal.setBlePin(0);
    Serial.printf("[BLE] Bluetooth AUS (%s). WLAN-Portal: http://192.168.4.1\n", boot_guard.reason);
  }
#endif
#endif

  the_mesh.startInterface(interface_manager);
  sensors.begin();

#if ENV_INCLUDE_GPS == 1
  the_mesh.applyGpsPrefs();
#endif
#if defined(DISASTER_PORTAL) && defined(DISASTER_GPS)
  sensors.setSettingValue("gps", "1");  // GPS profile: module on by default
#endif

#ifdef DISPLAY_CLASS
  ui_task.begin(disp, &sensors, the_mesh.getNodePrefs());  // still want to pass this in as dependency, as prefs might be moved
  the_mesh.setListener(&ui_task);
#endif

  board.onBootComplete();
}

void loop() {
  the_mesh.loop();
#ifdef DISASTER_PORTAL
  boot_guard.loop();
  disaster_portal.serviceAP();
#ifdef DISPLAY_CLASS
  {
    static unsigned long lastOled = 0; static bool showPw = false;
    if (millis() - lastOled > 15000) {  // abwechselnd WLAN-Name und Passwort auf dem OLED
      lastOled = millis(); showPw = !showPw;
      String t = showPw ? String("PW: ") + disaster_portal.wifiPassword()
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
#endif
  interface_manager.loop();
  sensors.loop();
#ifdef DISPLAY_CLASS
  ui_task.loop();
#endif
  rtc_clock.tick();
  board.loop();
#ifdef HAS_EXTERNAL_WATCHDOG
  external_watchdog.loop();
#endif

  if (!the_mesh.hasPendingWork()) {
#if defined(NRF52_PLATFORM)
    board.sleep(0); // nrf ignores seconds param, sleeps whenever possible
#endif
  }

#ifdef ENABLE_WIFI_INTERFACE
  if (wifi_enabled) {
    // RP2040 has no WiFi event callbacks, so poll the link state instead
  #if defined(RP2040_PLATFORM)
      wifi_needs_reconnect = (WiFi.status() != WL_CONNECTED);
      if (wifi_was_connected == wifi_needs_reconnect) {   // link state changed
        wifi_was_connected = !wifi_needs_reconnect;
        if (wifi_was_connected) {
          WIFI_DEBUG_PRINTLN("connected, listening on %s:%d", WiFi.localIP().toString().c_str(), TCP_PORT);
        } else {
          WIFI_DEBUG_PRINTLN("link lost");
        }
      }
  #endif

    // Safely attempt to reconnect if flagged. On RP2040 each attempt blocks the mesh loop
    // for up to WIFI_RETRY_TIMEOUT, so retry less often and cap how long a join may stall.
    if (wifi_needs_reconnect && (millis() - last_wifi_reconnect_attempt > WIFI_RETRY_INTERVAL)) {
      WIFI_DEBUG_PRINTLN("Attempting manual WiFi reconnect to %s (status %d)...", wifi_ssid, WiFi.status());
    #if defined(RP2040_PLATFORM)
        WiFi.setTimeout(WIFI_RETRY_TIMEOUT);
        WiFi.beginNoBlock(wifi_ssid, wifi_pwd);   // no reconnect() on this platform
    #else
        WiFi.disconnect();
        WiFi.reconnect();
    #endif
      last_wifi_reconnect_attempt = millis();
    }
  }
#endif
}
