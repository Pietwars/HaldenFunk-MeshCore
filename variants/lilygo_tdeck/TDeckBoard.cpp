#include <Arduino.h>
#include "TDeckBoard.h"

uint32_t deviceOnline = 0x00;

void TDeckBoard::begin() {
  // ESP32Board starts Wire. Set the physical pins BEFORE that first begin():
  // another Wire.begin(18, 8) on a running ESP32 bus does not change its pins.
  Wire.setPins(18, 8);
  ESP32Board::begin();
  
  // Enable peripheral power
  pinMode(PIN_PERF_POWERON, OUTPUT);
  digitalWrite(PIN_PERF_POWERON, HIGH);

  // Shared SPI bus (display + LoRa): keep both chip selects inactive from the start
  pinMode(P_LORA_NSS, OUTPUT);
  digitalWrite(P_LORA_NSS, HIGH);
#ifdef PIN_TFT_CS
  pinMode(PIN_TFT_CS, OUTPUT);
  digitalWrite(PIN_TFT_CS, HIGH);
#endif
  delay(10); // let peripheral rail settle

  // Configure user button
  pinMode(PIN_USER_BTN, INPUT);

  // Configure LoRa Pins
  pinMode(P_LORA_MISO, INPUT_PULLUP);
  // pinMode(P_LORA_DIO_1, INPUT_PULLUP);

  #ifdef P_LORA_TX_LED
    digitalWrite(P_LORA_TX_LED, HIGH); // inverted pin for SX1276 - HIGH for off
  #endif

  esp_reset_reason_t reason = esp_reset_reason();
  if (reason == ESP_RST_DEEPSLEEP) {
    uint64_t wakeup_source = esp_sleep_get_ext1_wakeup_status();
    if (wakeup_source & (1ULL << P_LORA_DIO_1)) {
      startup_reason = BD_STARTUP_RX_PACKET; // received a LoRa packet (while in deep sleep)
    }

    rtc_gpio_hold_dis((gpio_num_t)P_LORA_NSS);
    rtc_gpio_deinit((gpio_num_t)P_LORA_DIO_1);
  }
}
