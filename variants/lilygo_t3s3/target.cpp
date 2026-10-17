#include <Arduino.h>
#include "target.h"

ESP32Board board;

static SPIClass spi;
RADIO_CLASS radio = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, spi);
WRAPPER_CLASS radio_driver(radio, board);

ESP32RTCClock fallback_clock;
AutoDiscoverRTCClock rtc_clock(fallback_clock);
#if ENV_INCLUDE_GPS
  // Optional external NMEA GPS on UART1 (HaldenFunk-MeshCore *_wifi_gps profile).
  #include <helpers/sensors/MicroNMEALocationProvider.h>
  MicroNMEALocationProvider nmea = MicroNMEALocationProvider(Serial1, &rtc_clock);
  EnvironmentSensorManager sensors = EnvironmentSensorManager(nmea);
#else
  SensorManager sensors;
#endif

#ifdef DISPLAY_CLASS
  DISPLAY_CLASS display;
  MomentaryButton user_btn(PIN_USER_BTN, 1000, true);
#endif

#ifndef LORA_CR
  #define LORA_CR      5
#endif

bool radio_init() {
  fallback_clock.begin();
  rtc_clock.begin(Wire);
  
  return radio.std_init(&spi);
}

mesh::LocalIdentity radio_new_identity() {
  RadioNoiseListener rng(radio);
  return mesh::LocalIdentity(&rng);  // create new random identity
}

