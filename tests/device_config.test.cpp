// SD settings file of the T-Deck Plus standalone build (Issue #35): strict keys, no secrets.
#include <assert.h>
#include <stdio.h>
#include <string>
#include "../examples/companion_radio/DeviceConfig.h"
using namespace devcfg;

int main() {
  Config c;
  const char *ok = "# Kommentar\r\nname = DAR-Lager 3\r\ngps_teilen=1\nkarte=dernau\nlog=aus\n\n";
  assert(parse(ok, strlen(ok), c));
  assert(c.hasName && !strcmp(c.name, "DAR-Lager 3") && c.hasShare && c.share && c.hasMap && !strcmp(c.map, "dernau"));
  assert(c.hasLog && !c.log && !c.rejected && !c.unknown && !c.secrets);
  const char *bad = "ble_pin=123456\nPIN=1\nchannel_key=00ff\nPasswort=hello\nname=\nkarte=../etc\ngps_teilen=vielleicht\nfoo=bar\nkaputt\n";
  assert(parse(bad, strlen(bad), c));
  assert(c.secrets == 4 && c.rejected == 4 && c.unknown == 1 && !c.hasName && !c.hasMap && !c.hasShare);
  std::string big(MAX_BYTES + 1, 'x');
  assert(!parse(big.c_str(), big.size(), c));
  std::string longName = "name=" + std::string(NAME_BYTES + 1, 'a') + "\n";
  assert(parse(longName.c_str(), longName.size(), c) && !c.hasName && c.rejected == 1);
  // round trip; the written file has no secrets
  Config w; snprintf(w.name, sizeof(w.name), "T-Deck 7"); w.share = true; snprintf(w.map, sizeof(w.map), "sinzig"); w.log = true;
  char out[512]; size_t n = format(w, out, sizeof(out)); assert(n > 0);
  Config r; assert(parse(out, n, r));
  assert(!strcmp(r.name, "T-Deck 7") && r.share && !strcmp(r.map, "sinzig") && r.log && !r.secrets && !r.rejected && !r.unknown);
  assert(!strstr(out, "123456"));
  assert(format(w, out, 20) == 0);
  puts("device_config: ok");
  return 0;
}
