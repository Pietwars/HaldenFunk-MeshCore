// SPDX-License-Identifier: MIT
#pragma once
#include <stddef.h>
#include <stdint.h>
// Fixed-slot record files on ESP32 SPIFFS.
// SPIFFS has no directories, but SPIFFS_opendir() accepts ANY name. arduino-esp32 therefore
// returns a truthy *directory* handle from SPIFFS.open(missingFile,"r") and ("r+"): a check
// like `if(!f) f=open(path,"w+")` never creates the file, every seek()/write() fails and the
// history stays empty. SPIFFS.exists() filters directories, so decide the mode with it.
namespace disaster {
template <class FSys> auto openSlotFile(FSys &fs, const char *path) -> decltype(fs.open(path, (const char *)nullptr)) {
  auto f = fs.exists(path) ? fs.open(path, "r+") : fs.open(path, "w+");
  if (f && f.isDirectory()) { f.close(); return decltype(f)(); }
  return f;
}
template <class FSys> auto openSlotFileRead(FSys &fs, const char *path) -> decltype(fs.open(path, (const char *)nullptr)) {
  if (!fs.exists(path)) return decltype(fs.open(path, (const char *)nullptr))();
  auto f = fs.open(path, "r");
  if (f && f.isDirectory()) { f.close(); return decltype(fs.open(path, (const char *)nullptr))(); }
  return f;
}
// SPIFFS refuses seeks beyond the end of a file. Zero-fill up to `offset` first; zeroed
// bytes are never a valid record (seq 0 / checksum mismatch), so readers skip them.
template <class File> bool seekSlot(File &f, size_t offset) {
  if (!f.seek(0, SeekEnd)) return false;
  size_t end = f.position();
  static const uint8_t zero[32] = {};
  while (end < offset) {
    size_t n = offset - end < sizeof(zero) ? offset - end : sizeof(zero);
    if (f.write(zero, n) != n) return false;
    end += n;
  }
  return f.seek(offset, SeekSet);
}
}
