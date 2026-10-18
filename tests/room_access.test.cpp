// Room-Server: Gaeste ohne Passwort, nur Lesen; Admin-Anmeldung unveraendert.
#include <assert.h>
#include <string.h>
#include "../examples/simple_room_server/RoomAccess.h"
using namespace disaster;
int main() {
  // Gaeste: leeres oder beliebiges Passwort -> Leserecht
  assert(roomLoginPermission("", "a1b2c3d4e5f6a7") == ROOM_PERM_GUEST);
  assert(roomLoginPermission("hello", "a1b2c3d4e5f6a7") == ROOM_PERM_GUEST);
  assert(roomLoginPermission("a1b2c3d4e5f6a", "a1b2c3d4e5f6a7") == ROOM_PERM_GUEST);
  // Admin-Passwort -> Admin (keine Aenderung am Admin-Login)
  assert(roomLoginPermission("a1b2c3d4e5f6a7", "a1b2c3d4e5f6a7") == ROOM_PERM_ADMIN);
  // Leeres Admin-Passwort darf niemals Admin vergeben
  assert(roomLoginPermission("", "") == ROOM_PERM_GUEST);
  assert(roomLoginPermission(nullptr, "x") == ROOM_PERM_GUEST);
  // Update: altes Gast-Passwort wird geloescht, Gast-Lesezugriff eingeschaltet
  char guest[16]; memset(guest, 0, sizeof(guest)); strcpy(guest, "0123456789abcd");
  uint8_t ro = 0;
  assert(openRoomPrefs(guest, sizeof(guest), ro));
  for (char c : guest) assert(c == 0);
  assert(ro == 1);
  assert(!openRoomPrefs(guest, sizeof(guest), ro));  // zweiter Start: nichts zu speichern
  strcpy(guest, "hello"); assert(openRoomPrefs(guest, sizeof(guest), ro) && guest[0] == 0 && ro == 1);
  // CLI kann kein Gast-Passwort mehr setzen und den Gastzugang nicht abschalten
  assert(roomCliBlocked("set guest.password geheim"));
  assert(roomCliBlocked("set allow.read.only off"));
  assert(!roomCliBlocked("get guest.password"));
  assert(!roomCliBlocked("set password neu"));
  assert(!roomCliBlocked("password neu"));
  assert(!roomCliBlocked(nullptr));
  return 0;
}
