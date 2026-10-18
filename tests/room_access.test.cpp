// Room-Server: oeffentliches Gast-Passwort "hello" (Lesen/Schreiben), ohne Passwort nur Lesen,
// Admin-Anmeldung unveraendert.
#include <assert.h>
#include <string.h>
#include "../examples/simple_room_server/RoomAccess.h"
using namespace disaster;
int main() {
  const char *admin = "a1b2c3d4e5f6a7";
  assert(strcmp(DISASTER_ROOM_GUEST_PASSWORD, "hello") == 0);
  // Gast mit "hello" -> Lesen und Schreiben (wie Upstream)
  assert(roomLoginPermission("hello", admin, "hello") == ROOM_PERM_READ_WRITE);
  // Ohne oder mit falschem Passwort -> nur lesen
  assert(roomLoginPermission("", admin, "hello") == ROOM_PERM_GUEST);
  assert(roomLoginPermission("Hello", admin, "hello") == ROOM_PERM_GUEST);
  assert(roomLoginPermission("hell", admin, "hello") == ROOM_PERM_GUEST);
  assert(roomLoginPermission(nullptr, admin, "hello") == ROOM_PERM_GUEST);
  // Admin-Passwort -> Admin (unveraendert), hat Vorrang
  assert(roomLoginPermission(admin, admin, "hello") == ROOM_PERM_ADMIN);
  assert(roomLoginPermission("hello", "hello", "hello") == ROOM_PERM_ADMIN);
  // Leere Passwoerter vergeben niemals Rechte
  assert(roomLoginPermission("", "", "") == ROOM_PERM_GUEST);
  // Start/Update: leeres oder zufaelliges Gast-Passwort wird "hello", Lesezugriff an
  char guest[16]; memset(guest, 0, sizeof(guest)); uint8_t ro = 0;
  assert(roomGuestPrefs(guest, sizeof(guest), ro) && strcmp(guest, "hello") == 0 && ro == 1);
  for (size_t i = 5; i < sizeof(guest); i++) assert(guest[i] == 0);
  assert(!roomGuestPrefs(guest, sizeof(guest), ro));  // zweiter Start: nichts zu speichern
  strcpy(guest, "0123456789abcd");
  assert(roomGuestPrefs(guest, sizeof(guest), ro) && strcmp(guest, "hello") == 0);
  for (size_t i = 5; i < sizeof(guest); i++) assert(guest[i] == 0);
  strcpy(guest, "hello2"); assert(roomGuestPrefs(guest, sizeof(guest), ro) && strcmp(guest, "hello") == 0);
  // CLI: Gast-Passwort und Gast-Lesezugriff nicht aenderbar, Admin-Befehle frei
  assert(roomCliBlocked("set guest.password geheim"));
  assert(roomCliBlocked("set allow.read.only off"));
  assert(!roomCliBlocked("get guest.password"));
  assert(!roomCliBlocked("set password neu"));
  assert(!roomCliBlocked("password neu"));
  assert(!roomCliBlocked(nullptr));
  return 0;
}
