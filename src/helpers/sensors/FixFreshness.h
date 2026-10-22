#pragma once
#include <stdint.h>
namespace gpsfix {
inline bool fresh(bool valid, bool seen, uint32_t now, uint32_t last, uint32_t max_age) {
  return valid && seen && uint32_t(now - last) <= max_age;
}
}
