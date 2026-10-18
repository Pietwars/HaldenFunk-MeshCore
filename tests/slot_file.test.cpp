// Host test: history/outbox files on arduino-esp32 SPIFFS semantics.
// Models two documented quirks of the ESP32 core (framework 2.0.x):
//  1) SPIFFS_opendir() ignores the name, so open(missing,"r"/"r+") yields a truthy DIRECTORY handle
//     (SPIFFSImpl::exists() filters that case: `(f == true) && !f.isDirectory()`).
//  2) SPIFFS_lseek() beyond the end of a file fails.
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <map>
#include <memory>
#include <string>
#include <vector>
enum SeekMode { SeekSet = 0, SeekCur = 1, SeekEnd = 2 };
struct FakeFs;
struct FakeFile {
  std::vector<uint8_t> *data = nullptr; bool dir = false, valid = false; size_t pos = 0;
  explicit operator bool() const { return valid; }
  bool isDirectory() const { return dir; }
  void close() { valid = false; data = nullptr; }
  bool seek(size_t off, SeekMode m = SeekSet) {
    if (!valid || dir) return false;
    size_t target = m == SeekEnd ? data->size() + off : m == SeekCur ? pos + off : off;
    if (target > data->size()) return false;  // SPIFFS: no seek past EOF
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
  FakeFile open(const char *path, const char *mode) {
    FakeFile f; auto it = files.find(path);
    if (it == files.end()) {
      if (mode[0] == 'r') { f.valid = true; f.dir = true; return f; }  // quirk 1
      it = files.emplace(path, std::vector<uint8_t>()).first;
    } else if (mode[0] == 'w') it->second.clear();
    f.valid = true; f.data = &it->second; return f;
  }
  bool exists(const char *path) { auto f = open(path, "r"); return f && !f.isDirectory(); }
};
#include "../examples/companion_radio/HistoryRecord.h"
#include "../examples/companion_radio/SlotFile.h"
using disaster::HistoryRecord;
static HistoryRecord rec(uint32_t seq) {
  HistoryRecord r{}; r.seq = seq; r.len = 12; r.data[0] = 0xf3; r.crc = disaster::checksum(r); return r;
}
static bool oldAppend(FakeFs &fs, const HistoryRecord &r) {  // pre-fix DisasterPortal::append
  FakeFile f = fs.open("/dr-history.bin", "r+");
  if (!f) f = fs.open("/dr-history.bin", "w+");
  bool ok = f && f.seek(disaster::offset(r.seq)) && f.write((const uint8_t *)&r, sizeof(r)) == sizeof(r);
  f.close(); return ok;
}
static bool newAppend(FakeFs &fs, const HistoryRecord &r) {
  FakeFile f = disaster::openSlotFile(fs, "/dr-history.bin");
  bool ok = f && disaster::seekSlot(f, disaster::offset(r.seq)) && f.write((const uint8_t *)&r, sizeof(r)) == sizeof(r);
  if (f) f.close();
  return ok;
}
static int countValid(FakeFs &fs) {
  FakeFile f = disaster::openSlotFileRead(fs, "/dr-history.bin"); HistoryRecord r; int n = 0;
  while (f && f.read((uint8_t *)&r, sizeof(r)) == sizeof(r)) if (disaster::valid(r)) n++;
  return n;
}
int main() {
  { FakeFs fs;  // reproduces the empty web chat: file is never created
    assert(!oldAppend(fs, rec(1)) && !oldAppend(fs, rec(2)));
    assert(!fs.exists("/dr-history.bin") && countValid(fs) == 0); }
  { FakeFs fs;
    for (uint32_t s = 1; s <= 200; ++s) assert(newAppend(fs, rec(s)));  // wraps after 96 slots
    assert(countValid(fs) == (int)disaster::HISTORY_SLOTS);
    assert(fs.files["/dr-history.bin"].size() == disaster::HISTORY_SLOTS * sizeof(HistoryRecord)); }
  { FakeFs fs;  // after /api/history-wipe the sequence keeps counting: offset > 0 on a new file
    assert(newAppend(fs, rec(42)) && countValid(fs) == 1);
    assert(fs.files["/dr-history.bin"].size() == 42 * sizeof(HistoryRecord)); }
  { FakeFs fs; assert(!disaster::openSlotFileRead(fs, "/missing")); }
  puts("PASS: slot files on SPIFFS semantics (old pattern reproduces empty history; fix stores, wraps, pads)");
}
