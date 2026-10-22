// SPDX-License-Identifier: MIT
#pragma once
// SPI wiring of the optional SD card (DISASTER_SD).
// TTGO / T3-S3: radio uses SPIClass() (FSPI on S3); SD must use the other physical controller.
// LilyGO T-Deck Plus (standalone, Issue #35): the SD slot sits on the one shared FSPI bus of LoRa (NSS 9) and
// display (CS 12) (variants/lilygo_tdeck/target.cpp, tdeck_shared_spi); only its chip select differs.
// SD CS = GPIO 39 per the LilyGO T-Deck pinout; still to be confirmed on hardware (STANDALONE.md).
namespace disaster {
#if defined(LILYGO_TDECK)
#ifndef PIN_SD_CS
#error "T-Deck SD needs PIN_SD_CS (platformio.standalone.ini)"
#endif
#define DISASTER_SD_SHARED_BUS 1
constexpr int sdSck = P_LORA_SCLK, sdMiso = P_LORA_MISO, sdMosi = P_LORA_MOSI, sdCs = PIN_SD_CS;
static_assert(sdCs != P_LORA_NSS && sdCs != PIN_TFT_CS && sdCs != PIN_TFT_DC, "SD chip select must be its own pin");
static_assert(sdCs != P_LORA_RESET && sdCs != P_LORA_BUSY && sdCs != P_LORA_DIO_1, "SD chip select conflicts with LoRa");
static_assert(sdSck == PIN_TFT_SCL && sdMosi == PIN_TFT_SDA, "T-Deck SD shares the display/LoRa bus");
#elif defined(CONFIG_IDF_TARGET_ESP32S3) && defined(LILYGO_T3S3)
constexpr int sdBus = HSPI;
constexpr int sdSck = 14, sdMiso = 2, sdMosi = 11, sdCs = 13;
#elif defined(CONFIG_IDF_TARGET_ESP32)
constexpr int sdBus = VSPI;
constexpr int sdSck = 14, sdMiso = 2, sdMosi = 15, sdCs = 13;
#else
#error "SD archive requires a verified ESP32 TTGO V1.6, ESP32-S3 T3-S3 or T-Deck Plus profile"
#endif
#ifndef DISASTER_SD_SHARED_BUS
static_assert(P_LORA_RESET != sdSck, "LoRa reset conflicts with SD clock");
static_assert(P_LORA_SCLK != sdSck && P_LORA_MOSI != sdMosi && P_LORA_NSS != sdCs,
              "Radio and SD pins must be separate");
#endif
}
