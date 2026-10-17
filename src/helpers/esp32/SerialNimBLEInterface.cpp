#ifdef DISASTER_NIMBLE
#include "SerialNimBLEInterface.h"
#include "esp_mac.h"

#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"
#define ADVERT_RESTART_DELAY  1000   // millis
#define BLE_WRITE_MIN_INTERVAL  60
#define BLE_SUBSCRIBE_WAIT    8000   // so lange nach der Kopplung auf das CCCD-Abo der App warten
#define BLE_MTU_WAIT          2500   // so lange nach dem Verbinden auf den MTU-Austausch warten
#define BLE_TX_MAX_RETRIES      20   // ENOMEM/EBUSY: so oft erneut versuchen (je 60 ms)
#define BLE_LOG_FRAMES          12   // je Verbindung die ersten N RX/TX-Frames seriell protokollieren

// Ereignis-Bits (Host-Task -> Loop-Task)
#define EV_CONNECT     0x01
#define EV_DISCONNECT  0x02
#define EV_MTU         0x04
#define EV_AUTH        0x08
#define EV_SUB         0x10
#define EV_RX_BAD      0x20
#define EV_RX_FULL     0x40

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
  // Offener Zugang wie Bluedroid-Variante: Just Works (Bonding + Secure Connections, ohne MITM/PIN).
  // Verschluesselt, aber kein Zahlencode; das Handy zeigt hoechstens eine Kopplungsbestaetigung.
  (void)pin_code;
  NimBLEDevice::setSecurityAuth(true, false, true);
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(this, false);
  pServer->advertiseOnDisconnect(false);  // Neustart der Werbung wie im Original verzoegert in checkRecvFrame()

  pService = pServer->createService(SERVICE_UUID);
  // TX: READ + NOTIFY wie im Original, aber OHNE READ_ENC/READ_AUTHEN. Mit diesen Flags startet
  // NimBLE beim CCCD-Abo (noch unverschluesselt) selbst eine Security-Request; das kollidiert mit der
  // Kopplung, die die App bzw. das Handy wegen RX (WRITE_ENC) ohnehin startet -> erster Befehl
  // (CMD_DEVICE_QUERY) geht verloren, App meldet "Device Info" Timeout. Vertraulichkeit bleibt:
  // gesendet wird nur an gekoppelte Verbindungen (deviceConnected), der TX-Wert bleibt leer.
  pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX,
      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  pTxCharacteristic->setCallbacks(this);
  // RX: nur "write with response" (wie Original). Kein WRITE_NR: ein Write-Command auf ein
  // verschluesseltes Attribut wuerde vor der Kopplung stillschweigend verworfen.
  NimBLECharacteristic *pRx = pService->createCharacteristic(CHARACTERISTIC_UUID_RX,
      NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_ENC);
  pRx->setCallbacks(this);
  pServer->getAdvertising()->addServiceUUID(SERVICE_UUID);
  pServer->getAdvertising()->setScanResponse(true);
  _started = true;
  Serial.printf("[BLE] NimBLE bereit: Name \"%s\", Adresse %s\n", dev_name, NimBLEDevice::getAddress().toString().c_str());
}

// -------- Callbacks (NimBLE-Host-Task): nur Zustand setzen, Ausgabe in logEvents()

void SerialNimBLEInterface::onConnect(NimBLEServer*, ble_gap_conn_desc* desc) {
  last_conn_id = desc->conn_handle;
  _conn_time = millis();
  _subscribed = false;
  ev_conn = desc->conn_handle;
  ev_enc = desc->sec_state.encrypted; ev_bonded = desc->sec_state.bonded;
  __atomic_fetch_or(&ev_flags, EV_CONNECT, __ATOMIC_SEQ_CST);
}

void SerialNimBLEInterface::onDisconnect(NimBLEServer*, ble_gap_conn_desc*) {
  deviceConnected = false;
  _subscribed = false;
  last_conn_id = 0xffff;
  if (_isEnabled) adv_restart_time = millis() + ADVERT_RESTART_DELAY;
  __atomic_fetch_or(&ev_flags, EV_DISCONNECT, __ATOMIC_SEQ_CST);
}

void SerialNimBLEInterface::onMTUChange(uint16_t MTU, ble_gap_conn_desc*) {
  ev_mtu = MTU;
  __atomic_fetch_or(&ev_flags, EV_MTU, __ATOMIC_SEQ_CST);
}

uint32_t SerialNimBLEInterface::onPassKeyRequest() { return _pin_code; }

bool SerialNimBLEInterface::onConfirmPIN(uint32_t) { return true; }

void SerialNimBLEInterface::onAuthenticationComplete(ble_gap_conn_desc* desc) {
  ev_enc = desc->sec_state.encrypted; ev_authn = desc->sec_state.authenticated; ev_bonded = desc->sec_state.bonded;
  __atomic_fetch_or(&ev_flags, EV_AUTH, __ATOMIC_SEQ_CST);
  // Just Works ist verschluesselt, aber nicht "authenticated" (kein MITM-Schutz) -> nur Verschluesselung pruefen.
  if (desc->sec_state.encrypted) {
    _auth_time = millis();
    deviceConnected = true;
  } else {
    pServer->disconnect(desc->conn_handle);
    adv_restart_time = millis() + ADVERT_RESTART_DELAY;
  }
}

void SerialNimBLEInterface::onSubscribe(NimBLECharacteristic* pChr, ble_gap_conn_desc* desc, uint16_t subValue) {
  if (pChr != pTxCharacteristic) return;
  if (desc == nullptr || desc->conn_handle == last_conn_id) _subscribed = (subValue & 0x0001) != 0 || (subValue & 0x0002) != 0;
  ev_sub = subValue;
  __atomic_fetch_or(&ev_flags, EV_SUB, __ATOMIC_SEQ_CST);
}

void SerialNimBLEInterface::onWrite(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc*) {
  NimBLEAttValue v = pCharacteristic->getValue();
  size_t len = v.length();
  if (len == 0 || len > MAX_FRAME_SIZE) {
    ev_rx_bad = len > 255 ? 255 : len;
    __atomic_fetch_or(&ev_flags, EV_RX_BAD, __ATOMIC_SEQ_CST);
    return;
  }
  Frame frame;
  frame.len = len;
  memcpy(frame.buf, v.data(), len);
  if (xQueueSend(recv_queue, &frame, 0) != pdTRUE) {
    ev_rx_full++;
    __atomic_fetch_or(&ev_flags, EV_RX_FULL, __ATOMIC_SEQ_CST);
  }
}

// -------- Diagnose (Loop-Task)

void SerialNimBLEInterface::logEvents() {
  if (!ev_flags) return;
  uint32_t f = __atomic_exchange_n(&ev_flags, 0, __ATOMIC_SEQ_CST);
  if (f & EV_CONNECT) {
    _rx_logged = _tx_logged = 0;
    Serial.printf("[BLE] verbunden (conn %u, MTU %u, %s)\n", (unsigned)ev_conn,
      (unsigned)pServer->getPeerMTU(ev_conn), ev_bonded ? "bekanntes Handy" : "neu");
  }
  if (f & EV_MTU) Serial.printf("[BLE] MTU ausgehandelt: %u (max. Frame %u Bytes)\n", (unsigned)ev_mtu, (unsigned)(ev_mtu > 3 ? ev_mtu - 3 : 0));
  if (f & EV_AUTH) {
    if (ev_enc) Serial.printf("[BLE] Kopplung OK (verschluesselt, ohne PIN%s)\n", ev_bonded ? ", gespeichert" : "");
    else Serial.printf("[BLE] Kopplung FEHLGESCHLAGEN (verschluesselt=%u) -> getrennt. Handy: Geraet entfernen, neu koppeln\n",
      (unsigned)ev_enc);
  }
  if (f & EV_SUB) Serial.printf("[BLE] App-Abo TX-Notify: %s\n", (ev_sub & 3) ? "AN" : "aus");
  if (f & EV_RX_BAD) Serial.printf("[BLE] FEHLER: Befehl mit ungueltiger Laenge %u verworfen\n", (unsigned)ev_rx_bad);
  if (f & EV_RX_FULL) Serial.printf("[BLE] FEHLER: Empfangspuffer voll, %u Befehl(e) verworfen\n", (unsigned)ev_rx_full);
  if (f & EV_DISCONNECT) {
    Serial.printf("[BLE] getrennt (empfangen %lu, gesendet %lu, verworfen %lu)\n",
      (unsigned long)_rx_ok, (unsigned long)_tx_ok, (unsigned long)_tx_drop);
    _rx_ok = _tx_ok = _tx_drop = 0;
  }
}

// -------- BaseSerialInterface

void SerialNimBLEInterface::clearBuffers() {
  xQueueReset(recv_queue);
  send_queue_len = 0;
  _tx_retries = 0;
}

void SerialNimBLEInterface::enable() {
  if (_isEnabled || !_started) return;
  _isEnabled = true;
  clearBuffers();
  pService->start();
  pServer->start();
  pServer->getAdvertising()->start();
  adv_restart_time = 0;
  Serial.println("[BLE] Werbung aktiv - MeshCore-App kann verbinden");
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
    if (send_queue_len >= NIMBLE_SEND_QUEUE_SIZE) {
      _tx_drop++;
      if (millis() - _last_err_log > 2000) { _last_err_log = millis(); Serial.printf("[BLE] FEHLER: Sendepuffer voll, Frame 0x%02X verworfen\n", src[0]); }
      return 0;
    }
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

void SerialNimBLEInterface::popSendQueue() {
  send_queue_len--;
  for (int i = 0; i < send_queue_len; i++) send_queue[i] = send_queue[i + 1];
  _tx_retries = 0;
}

void SerialNimBLEInterface::serviceSendQueue() {
  if (send_queue_len == 0 || millis() < _last_write + BLE_WRITE_MIN_INTERVAL) return;
  uint16_t conn = last_conn_id;
  if (!deviceConnected || conn == 0xffff) { send_queue_len = 0; return; }

  Frame &f = send_queue[0];
  if (!_subscribed) {
    // App hat das TX-Abo noch nicht gesetzt: Frame halten statt (wie notify()) still zu verwerfen.
    if (millis() - _auth_time < BLE_SUBSCRIBE_WAIT) return;
    _tx_drop++;
    if (millis() - _last_err_log > 2000) { _last_err_log = millis(); Serial.printf("[BLE] FEHLER: App hat TX-Notify nicht abonniert, Frame 0x%02X verworfen\n", f.buf[0]); }
    popSendQueue();
    return;
  }
  uint16_t mtu = pServer->getPeerMTU(conn);
  size_t max_len = mtu > 3 ? mtu - 3 : 0;
  if (f.len > max_len && millis() - _conn_time < BLE_MTU_WAIT) return;  // MTU-Austausch abwarten
  size_t len = f.len;
  if (len > max_len) {
    // MeshCore kennt keine Fragmentierung: ein Frame muss in eine Notification passen.
    Serial.printf("[BLE] FEHLER: Frame 0x%02X hat %u Bytes, MTU %u erlaubt nur %u -> abgeschnitten\n",
      f.buf[0], (unsigned)f.len, (unsigned)mtu, (unsigned)max_len);
    len = max_len;
  }
  _last_write = millis();
  os_mbuf *om = ble_hs_mbuf_from_flat(f.buf, len);
  int rc = om ? ble_gattc_notify_custom(conn, pTxCharacteristic->getHandle(), om) : BLE_HS_ENOMEM;
  if (rc == 0) {
    _tx_ok++;
    if (_tx_logged < BLE_LOG_FRAMES) { _tx_logged++; Serial.printf("[BLE] -> App: Frame 0x%02X (%u Bytes)\n", f.buf[0], (unsigned)len); }
    popSendQueue();
  } else if ((rc == BLE_HS_ENOMEM || rc == BLE_HS_EBUSY) && ++_tx_retries < BLE_TX_MAX_RETRIES) {
    // spaeter erneut versuchen (mbuf-Pool/Controller gerade voll)
  } else {
    _tx_drop++;
    Serial.printf("[BLE] FEHLER: Notify Frame 0x%02X fehlgeschlagen (rc=%d), verworfen\n", f.buf[0], rc);
    popSendQueue();
  }
}

size_t SerialNimBLEInterface::checkRecvFrame(uint8_t dest[]) {
  if (!_started) return 0;
  logEvents();
  serviceSendQueue();

  Frame frame;
  if (xQueueReceive(recv_queue, &frame, 0) == pdTRUE) {
    memcpy(dest, frame.buf, frame.len);
    _rx_ok++;
    if (_rx_logged < BLE_LOG_FRAMES) { _rx_logged++; Serial.printf("[BLE] <- App: Befehl %u (%u Bytes)\n", (unsigned)frame.buf[0], (unsigned)frame.len); }
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
