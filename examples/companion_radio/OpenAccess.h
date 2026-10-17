#pragma once
#include <stddef.h>
#include <stdint.h>

namespace disaster {
// Validate received bytes before any dispatcher access (including memcmp).
inline size_t companionMinimumLength(uint8_t cmd) {
  switch (cmd) {
    case 1: return 8;  // app start
    case 2: return 14; // direct text
    case 3: return 7;  // channel text
    case 6: case 37: return 5;
    case 8: case 12: case 22: case 31: case 38: case 54: case 55: case 56: case 58: return 2;
    case 11: return 11; // radio: freq, bandwidth, SF, CR
    case 13: case 15: case 16: case 26: case 27: case 28: case 29: case 30: return 33;
    case 14: case 21: return 9;
    case 19: return 7;  // reboot + magic
    case 24: return 65;
    case 25: return 6;
    case 32: return 50;
    case 34: return 2;
    case 36: return 11;
    case 39: case 52: case 57: return 34;
    case 41: return 4;
    case 42: return 34;
    case 50: return 35;
    case 51: return 6; // factory reset + magic
    case 61: case 66: return 3;
    case 62: return 5; // group data: command, channel, path, 16-bit type
    case 65: return 4;
    default: return 1;
  }
}
inline bool validCompanionLength(const uint8_t* frame,size_t len,size_t capacity) {
  return frame && len && len<=capacity && len>=companionMinimumLength(frame[0]);
}
// Open Bluetooth/network clients can communicate and inspect status. Configuration,
// raw packets, signing, private-key export and CLI require USB or web-admin.
// Unknown/new commands are denied until explicitly reviewed here.
inline bool publicCompanionCommand(uint8_t cmd) {
  switch(cmd) {
    case 1: case 2: case 3: case 4: case 5: case 7: case 10:
    case 6:  // device time: MeshCore only moves the clock forward; the app syncs it on connect
    case 13: case 16: case 17: case 18: case 20: case 22:
    case 26: case 27: case 28: case 29: case 30: case 31:
    case 40: case 64: // read-only (custom vars, default flood scope), queried by the app
    case 39: case 42: case 43: case 52: case 56: case 59: case 60:
      return true;
    default: return false;
  }
}
}
