// SPDX-License-Identifier: MIT
#pragma once
#include "HistoryRecord.h"
#include "ArchivePolicy.h"
#ifdef DISASTER_SD
#include <SD.h>
#include <SPI.h>
#include "ArchiveBusConfig.h"
#ifdef DISASTER_SD_SHARED_BUS
extern SPIClass tdeck_shared_spi;   // variants/lilygo_tdeck/target.cpp
void tdeck_shared_spi_begin();
#endif
#endif
class ArchiveSd {
#ifdef DISASTER_SD
#ifdef DISASTER_SD_SHARED_BUS
 SPIClass &bus=tdeck_shared_spi;    // shared with LoRa and display; SD only uses its own chip select
#else
 SPIClass bus{disaster::sdBus};
#endif
#endif
 bool mounted=false;
public:
 void begin(){
#ifdef DISASTER_SD
#ifdef DISASTER_SD_SHARED_BUS
  pinMode(disaster::sdCs,OUTPUT);digitalWrite(disaster::sdCs,HIGH);
  tdeck_shared_spi_begin();          // no-op if display/radio started it already
#else
  bus.begin(disaster::sdSck,disaster::sdMiso,disaster::sdMosi,disaster::sdCs);
#endif
  mounted=SD.begin(disaster::sdCs,bus,4000000,"/sd",2,false);
  if(mounted&&!SD.exists("/disaster"))mounted=SD.mkdir("/disaster");
  Serial.println(mounted?"SD archive ready":"SD archive unavailable; internal history remains active");
#endif
 }
 bool ready()const{return mounted;}
 void append(const disaster::HistoryRecord &r){
#ifdef DISASTER_SD
  if(!mounted)return;
  auto f=SD.open("/disaster/current.bin",FILE_APPEND);
  if(!f){mounted=false;return;}
  size_t bytes=f.size();f.close();
  if(bytes>disaster::ARCHIVE_LIMIT){mounted=false;return;}
  // Preserve a torn tail in the previous segment; never append misaligned records.
  bool rotate=disaster::archiveRotate(bytes,sizeof(r));
  if(rotate){if(SD.exists("/disaster/previous.bin")&&!SD.remove("/disaster/previous.bin")){mounted=false;return;}
   if(!SD.rename("/disaster/current.bin","/disaster/previous.bin")){mounted=false;return;}}
  f=SD.open("/disaster/current.bin",FILE_APPEND);
  if(!f||f.write((const uint8_t*)&r,sizeof(r))!=sizeof(r))mounted=false;
  f.close();
#else
  (void)r;
#endif
 }
 File open(bool previous){
#ifdef DISASTER_SD
  if(mounted){File f=SD.open(previous?"/disaster/previous.bin":"/disaster/current.bin",FILE_READ);if(f&&f.size()<=disaster::ARCHIVE_LIMIT)return f;f.close();}
#else
  (void)previous;
#endif
  return File();
 }
};
