#ifdef DISASTER_NIMBLE
#include "SerialNimBLEInterface.h"
#include "esp_mac.h"

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
#define ADVERT_RESTART_DELAY  1000   // millis
#define BLE_WRITE_MIN_INTERVAL  60

void SerialNimBLEInterface::begin(const char* prefix, char* name, uint32_t pin_code) {
  _pin_code = pin_code;
  if (strcmp(name, "@@MAC") == 0) {
    uint8_t addr[8] = {};
    esp_efuse_mac_get_default(addr);
    sprintf(name, "%02X%02X%02X%02X%02X%02X", addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
  }
  char dev_name[32+16];
  snprintf(dev_name, sizeof(dev_name), "%s%s", prefix, name);

  NimBLEDevice::init(dev_name);           // gibt Classic-BT-Speicher frei, Controller nur BLE
  NimBLEDevice::setMTU(MAX_FRAME_SIZE);
  // Kopplung wie Bluedroid-Variante: Bonding + MITM + Secure Connections, statische PIN, Geraet zeigt PIN an
  NimBLEDevice::setSecurityAuth(true, true, true);
  NimBLEDevice::setSecurityPasskey(pin_code);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);

  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(this, false);
  pServer->advertiseOnDisconnect(false);  // Neustart der Werbung wie im Original verzoegert in checkRecvFrame()

  pService = pServer->createService(SERVICE_UUID);
  pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::READ_AUTHEN);
  NimBLECharacteristic *pRx = pService->createCharacteristic(CHARACTERISTIC_UUID_RX,
      NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_ENC | NIMBLE_PROPERTY::WRITE_AUTHEN);
  pRx->setCallbacks(this);
  pServer->getAdvertising()->addServiceUUID(SERVICE_UUID);
  pServer->getAdvertising()->setScanResponse(true);
  _started = true;
}

// -------- Server-Callbacks (NimBLE-Host-Task)

void SerialNimBLEInterface::onConnect(NimBLEServer*, ble_gap_conn_desc* desc) {
  last_conn_id = desc->conn_handle;
  BLE_DEBUG_PRINTLN("onConnect(), conn=%d", desc->conn_handle);
}

void SerialNimBLEInterface::onDisconnect(NimBLEServer*, ble_gap_conn_desc*) {
  BLE_DEBUG_PRINTLN("onDisconnect()");
  deviceConnected = false;
  last_conn_id = 0xffff;
  if (_isEnabled) adv_restart_time = millis() + ADVERT_RESTART_DELAY;
}

void SerialNimBLEInterface::onMTUChange(uint16_t MTU, ble_gap_conn_desc*) {
  BLE_DEBUG_PRINTLN("onMTUChange(), mtu=%d", MTU);
}

uint32_t SerialNimBLEInterface::onPassKeyRequest() { return _pin_code; }

bool SerialNimBLEInterface::onConfirmPIN(uint32_t) { return true; }

void SerialNimBLEInterface::onAuthenticationComplete(ble_gap_conn_desc* desc) {
  if (desc->sec_state.encrypted && desc->sec_state.authenticated) {
    BLE_DEBUG_PRINTLN(" - Authentication Success");
    deviceConnected = true;
  } else {
    BLE_DEBUG_PRINTLN(" - Authentication Failure");
    pServer->disconnect(desc->conn_handle);
    adv_restart_time = millis() + ADVERT_RESTART_DELAY;
  }
}

void SerialNimBLEInterface::onWrite(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc*) {
  NimBLEAttValue v = pCharacteristic->getValue();
  size_t len = v.length();
  if (len == 0 || len > MAX_FRAME_SIZE) {
    BLE_DEBUG_PRINTLN("ERROR: onWrite(), bad frame len=%d", (int)len);
    return;
  }
  Frame frame = {};
  frame.len = len;
  memcpy(frame.buf, v.data(), len);
  if (xQueueSend(recv_queue, &frame, 0) != pdTRUE) {
    BLE_DEBUG_PRINTLN("ERROR: onWrite(), recv_queue is full!");
  }
}

// -------- BaseSerialInterface

void SerialNimBLEInterface::clearBuffers() {
  xQueueReset(recv_queue);
  send_queue_len = 0;
}

void SerialNimBLEInterface::enable() {
  if (_isEnabled || !_started) return;
  _isEnabled = true;
  clearBuffers();
  pService->start();
  pServer->start();
  pServer->getAdvertising()->start();
  adv_restart_time = 0;
}

void SerialNimBLEInterface::disable() {
  _isEnabled = false;
  if (!_started) return;
  pServer->getAdvertising()->stop();
  if (last_conn_id != 0xffff) pServer->disconnect(last_conn_id);
  oldDeviceConnected = deviceConnected = false;
  adv_restart_time = 0;
}

size_t SerialNimBLEInterface::writeFrame(const uint8_t src[], size_t len) {
  if (len > MAX_FRAME_SIZE) return 0;
  if (deviceConnected && len > 0) {
    if (send_queue_len >= NIMBLE_FRAME_QUEUE_SIZE) return 0;
    send_queue[send_queue_len].len = len;
    memcpy(send_queue[send_queue_len].buf, src, len);
    send_queue_len++;
    return len;
  }
  return 0;
}

bool SerialNimBLEInterface::isWriteBusy() const {
  return millis() < _last_write + BLE_WRITE_MIN_INTERVAL;
}

size_t SerialNimBLEInterface::checkRecvFrame(uint8_t dest[]) {
  if (!_started) return 0;
  if (send_queue_len > 0 && millis() >= _last_write + BLE_WRITE_MIN_INTERVAL) {
    _last_write = millis();
    pTxCharacteristic->setValue(send_queue[0].buf, send_queue[0].len);
    pTxCharacteristic->notify();
    send_queue_len--;
    for (int i = 0; i < send_queue_len; i++) send_queue[i] = send_queue[i + 1];
  }

  Frame frame;
  if (xQueueReceive(recv_queue, &frame, 0) == pdTRUE) {
    memcpy(dest, frame.buf, frame.len);
    return frame.len;
  }

  if (deviceConnected != oldDeviceConnected) {
    if (!deviceConnected) {
      clearBuffers();
      adv_restart_time = millis() + ADVERT_RESTART_DELAY;
    } else {
      pServer->getAdvertising()->stop();
      adv_restart_time = 0;
    }
    oldDeviceConnected = deviceConnected;
  }

  if (adv_restart_time && millis() >= adv_restart_time) {
    if (pServer->getConnectedCount() == 0) pServer->getAdvertising()->start();
    adv_restart_time = 0;
  }
  return 0;
}
#endif
