#include <Arduino.h>
#include "TWatchS3Board.h"

void TWatchS3Board::begin() {
  ESP32Board::begin();
  power_init();

  esp_reset_reason_t reason = esp_reset_reason();
  if (reason == ESP_RST_DEEPSLEEP) {
    long wakeup_source = esp_sleep_get_ext1_wakeup_status();
    if (wakeup_source & (1 << P_LORA_DIO_1)) {
      startup_reason = BD_STARTUP_RX_PACKET;
    }
    rtc_gpio_hold_dis((gpio_num_t)P_LORA_NSS);
    rtc_gpio_deinit((gpio_num_t)P_LORA_DIO_1);
  }
}

bool TWatchS3Board::power_init() {
  PMU = new XPowersAXP2101(Wire, PIN_BOARD_SDA, PIN_BOARD_SCL, I2C_PMU_ADD);
  if (!PMU->init()) {
    MESH_DEBUG_PRINTLN("Warning: Failed to find AXP2101 power management");
    delete PMU;
    PMU = NULL;
    return false;
  }

  PMU->setChargingLedMode(XPOWERS_CHG_LED_CTRL_CHG);

  // Power rails (matches LilyGo / Meshtastic T-Watch S3)
  PMU->setPowerChannelVoltage(XPOWERS_ALDO3, 3300);  // LoRa radio
  PMU->enablePowerOutput(XPOWERS_ALDO3);
  PMU->setPowerChannelVoltage(XPOWERS_ALDO2, 3300);  // sensors, display, PCF8563 RTC
  PMU->enablePowerOutput(XPOWERS_ALDO2);
  PMU->setPowerChannelVoltage(XPOWERS_ALDO1, 3300);  // 6-axis sensor / display rail
  PMU->enablePowerOutput(XPOWERS_ALDO1);
  PMU->setPowerChannelVoltage(XPOWERS_BLDO2, 3300);  // DRV2605 haptic
  PMU->enablePowerOutput(XPOWERS_BLDO2);
  PMU->setPowerChannelVoltage(XPOWERS_ALDO4, 3300);  // must stay on or the radio loses power
  PMU->enablePowerOutput(XPOWERS_ALDO4);

  PMU->disablePowerOutput(XPOWERS_DCDC2);
  PMU->disablePowerOutput(XPOWERS_DCDC3);
  PMU->disablePowerOutput(XPOWERS_DCDC4);
  PMU->disablePowerOutput(XPOWERS_DCDC5);
  PMU->disablePowerOutput(XPOWERS_BLDO1);
  PMU->disablePowerOutput(XPOWERS_DLDO1);
  PMU->disablePowerOutput(XPOWERS_DLDO2);
  PMU->disablePowerOutput(XPOWERS_VBACKUP);

  PMU->disableIRQ(XPOWERS_AXP2101_ALL_IRQ);
  PMU->clearIrqStatus();

  PMU->setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_500MA);
  PMU->setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);

  PMU->disableTSPinMeasure();
  PMU->enableSystemVoltageMeasure();
  PMU->enableVbusVoltageMeasure();
  PMU->enableBattVoltageMeasure();
  ((XPowersAXP2101*)PMU)->enableBattDetection();   // Akku erkennen (fuer isBatteryConnect)
  ((XPowersAXP2101*)PMU)->enableGauge();           // Ladestandsanzeige (%) des AXP2101

  // Seitentaste (PEKEY des AXP2101):
  //   kurz (< 2 s)  -> Kurzdruck-IRQ  (Display an/aus)
  //   ab 2 s        -> Langdruck-IRQ  (Flood-Advert), Uhr bleibt an
  //   ab 6 s        -> harte Abschaltung durch den PMU selbst (Sicherheits-Rueckfall)
  ((XPowersAXP2101*)PMU)->setIrqLevelTime(XPOWERS_AXP2101_IRQ_TIME_2S);
  PMU->setPowerKeyPressOffTime(XPOWERS_POWEROFF_6S);
  PMU->enableIRQ(XPOWERS_AXP2101_PKEY_SHORT_IRQ | XPOWERS_AXP2101_PKEY_LONG_IRQ);
  PMU->clearIrqStatus();
  pinMode(PIN_PMU_IRQ, INPUT_PULLUP);
  return true;
}

int TWatchS3Board::pollPowerKey() {
  if (!PMU) return PKEY_NONE;
  unsigned long now = millis();
  if (now - _last_key_poll < 20) return PKEY_NONE;   // I2C nur alle 20 ms
  _last_key_poll = now;
  if (digitalRead(PIN_PMU_IRQ) != LOW) return PKEY_NONE;   // IRQ-Leitung aktiv low
  XPowersAXP2101* axp = (XPowersAXP2101*)PMU;
  axp->getIrqStatus();
  int ev = PKEY_NONE;
  if (axp->isPekeyLongPressIrq()) ev = PKEY_LONG;
  else if (axp->isPekeyShortPressIrq()) ev = PKEY_SHORT;
  axp->clearIrqStatus();
  if (ev != PKEY_NONE) {             // Entprellen: nach einem Ereignis 300 ms nichts Neues
    if (now - _last_key_event < 300) ev = PKEY_NONE;
    else _last_key_event = now;
  }
  return ev;
}
