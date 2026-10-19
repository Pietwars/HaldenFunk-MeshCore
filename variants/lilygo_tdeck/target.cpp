#include <Arduino.h>
#include "target.h"

TDeckBoard board;

#if defined(P_LORA_SCLK)
  // One SPI controller shared by LoRa radio (NSS 9) and ST7789 display (TFT_CS 12).
  // Both drivers use beginTransaction()/endTransaction() with their own settings, and the
  // ESP32 core serialises transactions on one SPIClass instance via its internal lock.
  SPIClass tdeck_shared_spi(FSPI);
  static SPIClass& spi = tdeck_shared_spi;

  void tdeck_shared_spi_begin() {
    static bool started = false;
    if (started) return;
    started = true;
    // peripheral power must be on before talking to either device
    pinMode(PIN_PERF_POWERON, OUTPUT);
    digitalWrite(PIN_PERF_POWERON, HIGH);
    // deselect both devices before the bus starts toggling
    pinMode(P_LORA_NSS, OUTPUT);
    digitalWrite(P_LORA_NSS, HIGH);
  #ifdef PIN_TFT_CS
    pinMode(PIN_TFT_CS, OUTPUT);
    digitalWrite(PIN_TFT_CS, HIGH);
  #endif
    tdeck_shared_spi.begin(P_LORA_SCLK, P_LORA_MISO, P_LORA_MOSI); // SCK 40, MISO 38, MOSI 41, no HW CS
  }

  RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, spi);
#else
  RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY);
#endif

WRAPPER_CLASS radio_driver(radio, board);

ESP32RTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);
MicroNMEALocationProvider gps(Serial1, &rtc_clock);
EnvironmentSensorManager sensors(gps);

#ifdef DISPLAY_CLASS
  DISPLAY_CLASS display;
  MomentaryButton user_btn(PIN_USER_BTN, 1000, true);
#endif

bool radio_init() {
  fallback_clock.begin();
  rtc_clock.begin(Wire);
  Wire.begin(18, 8);

#if defined(P_LORA_SCLK)
  tdeck_shared_spi_begin(); // no-op if the display already started the shared bus
  return radio.std_init(&spi); // its spi->begin(...) is a no-op on an already started SPIClass
#else
  return radio.std_init();
#endif
}

mesh::LocalIdentity radio_new_identity() {
  RadioNoiseListener rng(radio);
  return mesh::LocalIdentity(&rng); // create new random identity
}
