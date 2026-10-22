// Frontend for every build without web layer (see Frontend.h): nothing besides the companion core.
// The T-Deck Plus standalone build (DISASTER_STANDALONE) has its own frontend (StandaloneFrontend.cpp).
#include "Frontend.h"
#if !defined(DISASTER_PORTAL) && !defined(DISASTER_STANDALONE)

#ifdef ESP32
#include <SPIFFS.h>
bool frontend::mountFilesystem() {
  SPIFFS.begin(true);   // format on first boot; no prebuilt file system image needed
  return true;
}
#endif

void frontend::begin(MultiSerialInterface&) {}
void frontend::sensorsReady() {}
void frontend::loop() {}

#endif
