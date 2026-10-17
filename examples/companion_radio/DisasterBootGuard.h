// SPDX-License-Identifier: MIT
#pragma once
// Start-Waechter fuer klassische ESP32 ohne PSRAM (T3 V1.6.1, Heltec V2, ...):
// WLAN-Portal und Bluetooth teilen sich ~160 KB internen Heap. Reicht er nicht,
// endet operator new in abort() -> Neustart -> Bootschleife. Der Waechter
//  1. protokolliert den Heap nach jeder Startstufe ([HEAP] ...),
//  2. startet Bluetooth nur, wenn danach noch genug Heap fuer das Portal bleibt,
//  3. merkt sich die Startstufe im RTC-RAM (ueberlebt Panic-Neustarts) und schaltet
//     Bluetooth beim naechsten Start ab, wenn der letzte Start dort abgestuerzt ist.
#include <Arduino.h>
#include <esp_system.h>
#include <esp_attr.h>
#include <esp_heap_caps.h>

#ifndef DISASTER_BLE_MIN_HEAP
// Mindest-Heap (Bytes) VOR dem Bluetooth-Start: NimBLE braucht ~25-35 KB, danach sollen
// fuer Webserver, Verlauf und /api/state noch >= 30 KB frei bleiben.
#define DISASTER_BLE_MIN_HEAP 70000
#endif
#ifndef DISASTER_BLE_MIN_BLOCK
#define DISASTER_BLE_MIN_BLOCK 24000   // groesster zusammenhaengender Block
#endif
#ifndef DISASTER_STABLE_MS
#define DISASTER_STABLE_MS 120000UL    // so lange nach dem Start zaehlt ein Absturz als Startfehler
#endif

namespace disaster {

enum BootStage : uint32_t { STAGE_NONE=0, STAGE_PORTAL=1, STAGE_BLE=2, STAGE_RUN_BLE=3, STAGE_STABLE=4 };

struct BootGuard {
  bool bleBlocked=false;      // in diesem Start kein Bluetooth (Absturz zuvor)
  const char *reason="";
  uint32_t lastStage=STAGE_NONE;
  bool stableMarked=false;

  // RTC_NOINIT: bleibt bei Panic-/Watchdog-Neustart erhalten, nicht bei Stromausfall.
  static uint32_t &magic(){static RTC_NOINIT_ATTR uint32_t v;return v;}
  static uint32_t &stage(){static RTC_NOINIT_ATTR uint32_t v;return v;}
  static uint32_t &crashes(){static RTC_NOINIT_ATTR uint32_t v;return v;}
  static uint32_t &blocked(){static RTC_NOINIT_ATTR uint32_t v;return v;}  // Bluetooth bis zum Aus-/Einschalten aus

  static bool crashReset(esp_reset_reason_t r){
    return r==ESP_RST_PANIC||r==ESP_RST_INT_WDT||r==ESP_RST_TASK_WDT||r==ESP_RST_WDT||r==ESP_RST_BROWNOUT;
  }
  static void heap(const char *where){
    Serial.printf("[HEAP] %-14s frei=%u  groesster Block=%u  Minimum=%u\n",where,
      (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT),
      (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT),
      (unsigned)heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT));
    Serial.flush();
  }
  void begin(){
    esp_reset_reason_t r=esp_reset_reason();
    if(magic()!=0x44415242UL || r==ESP_RST_POWERON){magic()=0x44415242UL;stage()=STAGE_NONE;crashes()=0;blocked()=0;}
    lastStage=stage();
    if(crashReset(r)) crashes()++; else crashes()=0;
    if(crashReset(r) && (lastStage==STAGE_BLE||lastStage==STAGE_RUN_BLE)){
      blocked()=1;
      reason=lastStage==STAGE_BLE?"Absturz beim Bluetooth-Start":"Absturz kurz nach dem Start mit Bluetooth";
    } else if(crashReset(r) && lastStage==STAGE_PORTAL && crashes()>1){
      // Portal stuerzt wiederholt ab: Bluetooth bleibt aus, Portal wird trotzdem versucht.
      blocked()=1; reason="wiederholter Absturz beim WLAN-Start";
    } else if(blocked()) reason="nach frueherem Absturz bis zum Aus-/Einschalten";
    bleBlocked=blocked()!=0;
    Serial.printf("[BOOT] Reset-Grund %d, letzte Startstufe %u, Abstuerze in Folge %u\n",(int)r,(unsigned)lastStage,(unsigned)crashes());
    if(bleBlocked)Serial.printf("[BOOT] %s -> Bluetooth in diesem Start AUS (WLAN-Portal laeuft weiter)\n",reason);
    heap("Start");
  }
  void enter(BootStage s){stage()=s;}
  // Darf Bluetooth gestartet werden? Prueft Absturzmerker und Heap.
  bool allowBle(){
    if(bleBlocked)return false;
    uint32_t freeB=heap_caps_get_free_size(MALLOC_CAP_8BIT), blk=heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
    if(freeB<DISASTER_BLE_MIN_HEAP||blk<DISASTER_BLE_MIN_BLOCK){
      reason="zu wenig freier Heap";
      Serial.printf("[BOOT] Bluetooth AUS: frei %u < %u oder Block %u < %u Bytes (WLAN-Portal hat Vorrang)\n",
        (unsigned)freeB,(unsigned)DISASTER_BLE_MIN_HEAP,(unsigned)blk,(unsigned)DISASTER_BLE_MIN_BLOCK);
      bleBlocked=true;return false;
    }
    return true;
  }
  void loop(){
    if(!stableMarked && millis()>DISASTER_STABLE_MS){stableMarked=true;stage()=STAGE_STABLE;crashes()=0;heap("stabil");}
  }
};

}  // namespace disaster
