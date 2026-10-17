#pragma once
// MeshCore-Companion ueber BLE mit NimBLE statt Bluedroid (nur Profile mit -D DISASTER_NIMBLE).
// Grund: Auf klassischen ESP32 ohne PSRAM (T3 V1.6.1, Heltec V2) passen Bluedroid (+ BR/EDR-Controller)
// und das WLAN-Portal nicht gemeinsam in den Heap -> abort() in operator new -> Bootschleife.
// NimBLE gibt den Classic-BT-Controllerspeicher frei, betreibt den Controller nur im BLE-Modus und
// braucht als Host deutlich weniger RAM. Gleiche Dienste/UUIDs/PIN-Kopplung wie SerialBLEInterface.
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
  uint32_t _pin_code = 0;
  unsigned long _last_write = 0;
  unsigned long adv_restart_time = 0;

  struct Frame {
    uint8_t len;
    uint8_t buf[MAX_FRAME_SIZE];
  };

  #define NIMBLE_FRAME_QUEUE_SIZE  4
  StaticQueue_t recv_queue_state;
  uint8_t recv_queue_storage[NIMBLE_FRAME_QUEUE_SIZE * sizeof(Frame)];
  QueueHandle_t recv_queue;
  int send_queue_len = 0;
  Frame send_queue[NIMBLE_FRAME_QUEUE_SIZE];

  void clearBuffers();

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

public:
  SerialNimBLEInterface() {
    recv_queue = xQueueCreateStatic(NIMBLE_FRAME_QUEUE_SIZE, sizeof(Frame), recv_queue_storage, &recv_queue_state);
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
