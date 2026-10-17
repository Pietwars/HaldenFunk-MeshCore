#pragma once
// MeshCore-Companion ueber BLE mit NimBLE statt Bluedroid (nur Profile mit -D DISASTER_NIMBLE).
// Grund: Auf klassischen ESP32 ohne PSRAM (T3 V1.6.1, Heltec V2) passen Bluedroid (+ BR/EDR-Controller)
// und das WLAN-Portal nicht gemeinsam in den Heap -> abort() in operator new -> Bootschleife.
// NimBLE gibt den Classic-BT-Controllerspeicher frei, betreibt den Controller nur im BLE-Modus und
// braucht als Host deutlich weniger RAM. Gleiche Dienste/UUIDs/PIN-Kopplung wie SerialBLEInterface.
//
// Verhalten wie die Bluedroid-Variante (MeshCore 1.18), mit drei Unterschieden, die NimBLE verlangt:
//  - TX (Notify) ohne READ_ENC/READ_AUTHEN: NimBLE wuerde sonst beim CCCD-Abo selbst eine
//    Security-Request schicken (Bluedroid tut das nicht) und so mit der Kopplung/dem ersten Befehl
//    der App kollidieren. Gekoppelt wird wie im Original ueber RX (WRITE_ENC + WRITE_AUTHEN);
//    gesendet wird nur an verschluesselte + authentifizierte (PIN) Verbindungen.
//  - Notifications ueber ble_gattc_notify_custom() mit Rueckgabewert: ein Frame wird erst aus der
//    Sendewarteschlange entfernt, wenn NimBLE ihn angenommen hat (kein stilles Verwerfen bei
//    fehlendem Abo, noch nicht ausgehandelter MTU oder vollem mbuf-Pool).
//  - [BLE]-Diagnosezeilen auf der seriellen Konsole (aus der Loop-Task, nicht aus dem NimBLE-Host).
#ifdef DISASTER_NIMBLE
#include "../BaseSerialInterface.h"
#include <NimBLEDevice.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

class SerialNimBLEInterface : public BaseSerialInterface, NimBLEServerCallbacks, NimBLECharacteristicCallbacks {
  NimBLEServer *pServer = nullptr;
  NimBLEService *pService = nullptr;
  NimBLECharacteristic *pTxCharacteristic = nullptr;
  volatile bool deviceConnected = false;
  bool oldDeviceConnected = false;
  bool _isEnabled = false;
  bool _started = false;
  volatile uint16_t last_conn_id = 0xffff;
  volatile bool _subscribed = false;       // App hat Notifications auf TX abonniert (CCCD)
  volatile unsigned long _conn_time = 0;   // millis() beim Verbindungsaufbau
  volatile unsigned long _auth_time = 0;   // millis() bei erfolgreicher Kopplung
  uint32_t _pin_code = 0;
  unsigned long _last_write = 0;
  unsigned long adv_restart_time = 0;
  uint8_t _tx_retries = 0;
  uint16_t _rx_logged = 0, _tx_logged = 0;
  uint32_t _tx_ok = 0, _tx_drop = 0, _rx_ok = 0;
  unsigned long _last_err_log = 0;

  // Ereignisse aus der NimBLE-Host-Task, ausgegeben in checkRecvFrame() (Loop-Task)
  volatile uint32_t ev_flags = 0;
  volatile uint16_t ev_conn = 0, ev_mtu = 0, ev_sub = 0;
  volatile uint8_t ev_enc = 0, ev_authn = 0, ev_bonded = 0, ev_rx_bad = 0, ev_rx_full = 0;
  void logEvents();

  struct Frame {
    uint8_t len;
    uint8_t buf[MAX_FRAME_SIZE];
  };

  #define NIMBLE_RECV_QUEUE_SIZE  4
  #define NIMBLE_SEND_QUEUE_SIZE  8
  StaticQueue_t recv_queue_state;
  uint8_t recv_queue_storage[NIMBLE_RECV_QUEUE_SIZE * sizeof(Frame)];
  QueueHandle_t recv_queue;
  int send_queue_len = 0;
  Frame send_queue[NIMBLE_SEND_QUEUE_SIZE];

  void clearBuffers();
  void popSendQueue();
  void serviceSendQueue();

protected:
  // NimBLEServerCallbacks
  void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) override;
  void onDisconnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) override;
  void onMTUChange(uint16_t MTU, ble_gap_conn_desc* desc) override;
  uint32_t onPassKeyRequest() override;
  bool onConfirmPIN(uint32_t pin) override;
  void onAuthenticationComplete(ble_gap_conn_desc* desc) override;
  // NimBLECharacteristicCallbacks
  void onWrite(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc) override;
  void onSubscribe(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc, uint16_t subValue) override;

public:
  SerialNimBLEInterface() {
    recv_queue = xQueueCreateStatic(NIMBLE_RECV_QUEUE_SIZE, sizeof(Frame), recv_queue_storage, &recv_queue_state);
  }

  /** wie SerialBLEInterface::begin(); name "@@MAC" wird durch die MAC ersetzt (IN/OUT). */
  void begin(const char* prefix, char* name, uint32_t pin_code);
  bool started() const { return _started; }

  // BaseSerialInterface
  void enable() override;
  void disable() override;
  bool isEnabled() const override { return _isEnabled; }
  bool isConnected() const override { return deviceConnected; }
  bool isWriteBusy() const override;
  size_t writeFrame(const uint8_t src[], size_t len) override;
  size_t checkRecvFrame(uint8_t dest[]) override;
};

#ifndef BLE_DEBUG_PRINTLN
#if BLE_DEBUG_LOGGING && ARDUINO
  #include <Arduino.h>
  #define BLE_DEBUG_PRINTLN(F, ...) Serial.printf("BLE: " F "\n", ##__VA_ARGS__)
#else
  #define BLE_DEBUG_PRINTLN(...) {}
#endif
#endif
#endif
