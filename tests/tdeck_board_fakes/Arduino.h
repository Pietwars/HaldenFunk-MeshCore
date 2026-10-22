#pragma once
#include <stdint.h>
#define OUTPUT 1
#define INPUT 0
#define INPUT_PULLUP 2
#define HIGH 1
#define LOW 0
#define PIN_PERF_POWERON 10
#define PIN_USER_BTN 0
#define P_LORA_NSS 9
#define P_LORA_MISO 38
#define P_LORA_DIO_1 45
using gpio_num_t = int;
using esp_reset_reason_t = int;
constexpr int ESP_RST_DEEPSLEEP=5, BD_STARTUP_RX_PACKET=2;
inline void pinMode(int,int) {}
inline void digitalWrite(int,int) {}
inline void delay(int) {}
inline void analogReadResolution(int) {}
inline uint32_t analogReadMilliVolts(int) { return 2000; }
inline int esp_reset_reason() { return 0; }
inline long esp_sleep_get_ext1_wakeup_status() { return 0; }
inline void rtc_gpio_hold_dis(int) {}
inline void rtc_gpio_deinit(int) {}
