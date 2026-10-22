#pragma once
// In-memory SPIFFS with the arduino-esp32 quirks modelled in tests/slot_file.test.cpp.
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include "Arduino.h"
enum SeekMode { SeekSet = 0, SeekCur = 1, SeekEnd = 2 };
#define FILE_READ "r"
#define FILE_APPEND "a"
struct File {
  std::vector<uint8_t> *data = nullptr; bool dir = false, valid = false; size_t pos = 0;
  explicit operator bool() const { return valid; }
  bool isDirectory() const { return dir; }
  void close() { valid = false; data = nullptr; }
  size_t size() const { return data ? data->size() : 0; }
  bool seek(size_t off, SeekMode m = SeekSet) {
    if (!valid || dir) return false;
    size_t target = m == SeekEnd ? data->size() + off : m == SeekCur ? pos + off : off;
    if (target > data->size()) return false;
    pos = target; return true;
  }
  size_t position() const { return pos; }
  size_t write(const uint8_t *p, size_t n) {
    if (!valid || dir) return 0;
    if (data->size() < pos + n) data->resize(pos + n);
    memcpy(data->data() + pos, p, n); pos += n; return n;
  }
  size_t read(uint8_t *p, size_t n) {
    if (!valid || dir) return 0;
    size_t k = pos >= data->size() ? 0 : std::min(n, data->size() - pos);
    memcpy(p, data->data() + pos, k); pos += k; return k;
  }
};
struct FakeFs {
  std::map<std::string, std::vector<uint8_t>> files;
  File open(const char *path, const char *mode) {
    File f; auto it = files.find(path);
    if (it == files.end()) {
      if (mode[0] == 'r') { f.valid = true; f.dir = true; return f; }
      it = files.emplace(path, std::vector<uint8_t>()).first;
    } else if (mode[0] == 'w') it->second.clear();
    f.valid = true; f.data = &it->second; return f;
  }
  bool exists(const char *path) { auto f = open(path, "r"); return f && !f.isDirectory(); }
  bool remove(const char *path) { return files.erase(path) > 0; }
};
extern FakeFs SPIFFS;
