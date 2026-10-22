#pragma once
#include <stdint.h>

// Auto-add contact policy (bits as in CMD_SET_AUTOADD_CONFIG / NodePrefs::autoadd_config).
namespace autoadd {
static const uint8_t OVERWRITE_OLDEST = 1 << 0;  // overwrite oldest non-favourite when full
static const uint8_t CHAT             = 1 << 1;  // ADV_TYPE_CHAT (1, companions)
static const uint8_t REPEATER         = 1 << 2;  // ADV_TYPE_REPEATER (2)
static const uint8_t ROOM_SERVER      = 1 << 3;  // ADV_TYPE_ROOM (3)
static const uint8_t SENSOR           = 1 << 4;  // ADV_TYPE_SENSOR (4)

// DisAHRsterRadio factory default: selective mode, only repeaters and room servers are
// added automatically; chat users (companions), sensors and unknown types stay manual.
static const uint8_t DISASTER_DEFAULT_MANUAL = 1;
static const uint8_t DISASTER_DEFAULT_CONFIG = REPEATER | ROOM_SERVER;

// manual_add_contacts bit 0 clear = add everything (MeshCore "auto add all").
inline bool shouldAdd(uint8_t manual_add_contacts, uint8_t autoadd_config, uint8_t adv_type) {
  if ((manual_add_contacts & 1) == 0) return true;
  uint8_t bit = 0;
  switch (adv_type) {
    case 1: bit = CHAT; break;
    case 2: bit = REPEATER; break;
    case 3: bit = ROOM_SERVER; break;
    case 4: bit = SENSOR; break;
    default: return false;  // unknown type, don't auto-add
  }
  return (autoadd_config & bit) != 0;
}
}
