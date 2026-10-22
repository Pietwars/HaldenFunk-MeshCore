#pragma once
// HaldenFunk-MeshCore build kinds (set by the PlatformIO env, never in code):
//   DISASTER_PORTAL      companion with web layer: WLAN access point + DisasterPortal (WebFrontend.cpp)
//   DISASTER_STANDALONE  LILYGO T-Deck Plus standalone (platformio.standalone.ini): MeshCore over PIN Bluetooth,
//                        display/keyboard on the device, no WLAN, no web server. WebFrontend.cpp is removed from
//                        that build by build_src_filter, so no web code is compiled in.
// Both share the same HaldenFunk-MC mesh policy (DAR_MESH_POLICY): TX power capped at 10 dBm, fixed 10 % airtime budget,
// only repeaters/room servers are added automatically, an explicitly set location is shared in adverts.
#if defined(DISASTER_PORTAL) && defined(DISASTER_STANDALONE)
#error "DISASTER_STANDALONE must not contain the web layer (DISASTER_PORTAL)"
#endif
#if defined(DISASTER_PORTAL) || defined(DISASTER_STANDALONE)
#define DAR_MESH_POLICY 1
// Both also own a local companion client (LocalCompanion.h: web portal or T-Deck display) which shares the
// offline queue with the MeshCore app (SharedQueue.h) and mirrors its own sends into the app.
#define DAR_LOCAL_COMPANION 1
#endif

// T-Deck Plus standalone: native pages on the display (ui-brand/StandaloneUI, keyboard + trackball).
#if defined(DISASTER_STANDALONE) && defined(LILYGO_TDECK)
#define DAR_STANDALONE_UI 1
#endif

// Boot diagnostics (DISASTER_DIAG builds only): serial marker with free heap.
#ifdef DISASTER_DIAG
#include <Arduino.h>
#define DIAG(m) do { Serial.printf("[DIAG %lu] %s heap=%u\n", millis(), m, (unsigned)ESP.getFreeHeap()); Serial.flush(); } while (0)
#else
#define DIAG(m) do {} while (0)
#endif
