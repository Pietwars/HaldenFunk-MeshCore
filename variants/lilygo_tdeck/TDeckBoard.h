#pragma once

#include <Wire.h>
#include <Arduino.h>
#include "helpers/ESP32Board.h"

#define PIN_VBAT_READ 4
#define BATTERY_SAMPLES 8

class TDeckBoard : public ESP32Board {
public:
  void begin();

  #ifdef P_LORA_TX_LED
    void onBeforeTransmit() override{
      digitalWrite(P_LORA_TX_LED, LOW); // turn TX LED on - invert pin for SX1276
    }

    void onAfterTransmit() override{
      digitalWrite(P_LORA_TX_LED, HIGH); // turn TX LED off - invert pin for SX1276
    }
  #endif

  // Akkuspannung: Spannungsteiler 1:2 an GPIO 4 (einzige Akku-Messung des T-Deck Plus, kein Strom,
  // kein Ladestatus). analogReadMilliVolts nutzt die eFuse-Kalibrierung des ESP32-S3 (genauer als
  // Rohwert * 3,3 V / 4096 bei 11 dB Daempfung).
  uint16_t getBattMilliVolts() override {
    #if defined(PIN_VBAT_READ)
      analogReadResolution(12);
      uint32_t mv = 0;
      for (int i = 0; i < BATTERY_SAMPLES; i++) {
        mv += analogReadMilliVolts(PIN_VBAT_READ);
      }
      return (uint16_t)(2 * mv / BATTERY_SAMPLES);
    #else
      return 0;
    #endif
  }

  const char* getManufacturerName() const{
    return "LilyGo T-Deck";
  }
};