#pragma once
// Seam between the companion core (main.cpp) and the optional extra frontend of a build.
// Exactly one implementation is linked:
//   WebFrontend.cpp    DAR-MC web layer (DISASTER_PORTAL): WLAN access point, DisasterPortal, web history
//   PlainFrontend.cpp  every other build without web layer: upstream companion behaviour
//   StandaloneFrontend.cpp  T-Deck Plus standalone (DISASTER_STANDALONE, Issue #35): local companion client for
//                      the display pages, SD settings/log, "Standort teilen"; WebFrontend.cpp is dropped via
//                      build_src_filter (platformio.standalone.ini)
// main.cpp knows nothing about WLAN or the web portal.
#include "DarBuild.h"

class MultiSerialInterface;

namespace frontend {
#ifdef ESP32
  // Mount the ESP32 file system used by DataStore. false = fatal (main halts).
  bool mountFilesystem();
#endif
  // After the mesh and the built-in interfaces (BLE, USB, ...) are set up, before startInterface().
  void begin(MultiSerialInterface& interfaces);
  // After sensors.begin() and the saved GPS preferences have been applied.
  void sensorsReady();
  // Once per main loop, right after the_mesh.loop().
  void loop();
}
