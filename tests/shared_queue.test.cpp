// 0.9.3: Nachrichten zwischen Weboberflaeche (Portal) und MeshCore-App teilen.
#include <assert.h>
#include <string.h>
#include "../examples/companion_radio/SharedQueue.h"
#include "../src/helpers/MultiSerialInterface.h"
using namespace disaster;

struct F { uint8_t len; uint8_t buf[176]; uint8_t seen = 0; bool isChannelMsg() const { return buf[0] == 17; } };
struct Port : BaseSerialInterface {
  bool on = true; size_t queued = 0, written = 0; uint8_t last = 0;
  void enable() override { on = true; } void disable() override { on = false; }
  bool isEnabled() const override { return on; } bool isConnected() const override { return on; }
  bool isWriteBusy() const override { return false; }
  size_t writeFrame(const uint8_t *p, size_t n) override { written += n; last = p[0]; return n; }
  size_t checkRecvFrame(uint8_t *p) override { size_t n = queued; queued = 0; if (n) p[0] = 10; return n; }
};

int main() {
  F q[4]; int n = 0; uint8_t out[176];
  uint8_t dm[] = {16, 1, 2, 3}, ch[] = {17, 4, 5, 6}, ch2[] = {17, 7};
  // Eingang: beide Leser bekommen dieselbe Nachricht, entfernt erst nach beiden
  assert(sqAdd(q, n, 4, dm, 4, 0, true) && n == 1);
  assert(sqTake(q, n, READER_PORTAL, out) == 4 && out[0] == 16 && n == 1);
  assert(sqTake(q, n, READER_PORTAL, out) == 0);               // Portal: nichts Neues
  assert(sqPending(q, n, READER_APP) == 1);
  assert(sqTake(q, n, READER_APP, out) == 4 && out[3] == 3 && n == 0);
  // App zuerst, dann Portal
  sqAdd(q, n, 4, ch, 4, 0, true);
  assert(sqTake(q, n, READER_APP, out) == 4 && n == 1);
  assert(sqTake(q, n, READER_APP, out) == 0);
  assert(sqTake(q, n, READER_PORTAL, out) == 4 && n == 0);
  // Web-Echo nur fuer die App
  sqAdd(q, n, 4, ch, 4, READER_PORTAL, true);
  assert(sqTake(q, n, READER_PORTAL, out) == 0);
  assert(sqTake(q, n, READER_APP, out) == 4 && n == 0);
  // Reihenfolge bleibt erhalten, auch wenn ein Leser vorauslaeuft
  sqAdd(q, n, 4, dm, 4, 0, true); sqAdd(q, n, 4, ch, 4, 0, true);
  assert(sqTake(q, n, READER_PORTAL, out) == 4 && out[0] == 16);
  assert(sqTake(q, n, READER_PORTAL, out) == 4 && out[0] == 17);
  assert(sqTake(q, n, READER_APP, out) == 4 && out[0] == 16);
  assert(sqTake(q, n, READER_APP, out) == 4 && out[0] == 17 && n == 0);
  // Volle Warteschlange, App nie verbunden: Portal bekommt weiter neue Direktnachrichten
  for (int i = 0; i < 4; i++) { dm[1] = i; sqAdd(q, n, 4, dm, 4, 0, true); sqTake(q, n, READER_PORTAL, out); }
  assert(n == 4);
  dm[1] = 9; assert(sqAdd(q, n, 4, dm, 4, 0, true) && n == 4);
  assert(sqTake(q, n, READER_PORTAL, out) == 4 && out[1] == 9);
  assert(q[0].buf[1] == 1);                                     // aelteste (0) wurde verdraengt
  // Portal-gelesene Kanalnachricht wird vor Direktnachricht verdraengt
  n = 0; sqAdd(q, n, 4, dm, 4, READER_PORTAL, true); sqAdd(q, n, 4, ch, 4, READER_PORTAL, true);
  sqAdd(q, n, 4, dm, 4, READER_PORTAL, true); sqAdd(q, n, 4, dm, 4, READER_PORTAL, true);
  assert(sqAdd(q, n, 4, ch2, 2, 0, true) && n == 4);
  for (int i = 0; i < 3; i++) assert(q[i].buf[0] == 16);
  assert(q[3].buf[0] == 17 && q[3].len == 2);
  // Ohne Portal (Standard-MeshCore): volle Queue nur mit Direktnachrichten -> neue wird verworfen
  n = 0; for (int i = 0; i < 4; i++) sqAdd(q, n, 4, dm, 4, READER_PORTAL, false);
  // (READER_PORTAL gesetzt = nur App liest; Kanalnachrichten fehlen) -> Schritt 2 aus
  F r[2]; int m = 0; sqAdd(r, m, 2, dm, 4, 0, false); sqAdd(r, m, 2, dm, 4, 0, false);
  assert(!sqAdd(r, m, 2, dm, 4, 0, false) && m == 2);
  assert(!sqAdd(r, m, 2, dm, 0, 0, false));

  // Antworten auf Abruf/Senden nur an den anfragenden Client, Push an alle
  Port ble, portal; MultiSerialInterface mux; uint8_t frame[176] = {};
  mux.addInterface(InterfaceType::Bluetooth, &ble); mux.addInterface(InterfaceType::Portal, &portal); mux.enable();
  portal.queued = 1; assert(mux.checkRecvFrame(frame) == 1 && mux.isPortalTransport());
  mux.setReplyOnly(true);
  frame[0] = 16; mux.writeFrame(frame, 20); assert(portal.written == 20 && ble.written == 0);
  frame[0] = 0x83; mux.writeFrame(frame, 1); assert(ble.written == 1 && portal.written == 21);
  mux.setReplyOnly(false);
  frame[0] = 0; mux.writeFrame(frame, 1); assert(ble.written == 2 && portal.written == 22);
  ble.written = portal.written = 0;
  ble.queued = 1; assert(mux.checkRecvFrame(frame) == 1 && !mux.isPortalTransport() && mux.isBluetoothTransport());
  mux.setReplyOnly(true);
  frame[0] = 10; mux.writeFrame(frame, 1); assert(ble.written == 1 && portal.written == 0);
  mux.setReplyOnly(false);
  return 0;
}
