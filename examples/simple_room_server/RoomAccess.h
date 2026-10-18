// SPDX-License-Identifier: MIT
#pragma once
// Zugangsregeln des Notfall-Room-Servers (nur DISASTER_ROOM).
// Gaeste treten ohne Passwort bei und duerfen nur lesen. Es gibt kein Gast-/Raum-Passwort.
// Verwaltung nur ueber das Admin-Passwort (unveraendert); Schreiben nur Admins oder per setperm.
#include <stddef.h>
#include <stdint.h>
#include <string.h>

namespace disaster {

// Rollen wie in src/helpers/ClientACL.h (PERM_ACL_GUEST / PERM_ACL_ADMIN).
static const uint8_t ROOM_PERM_GUEST = 0;
static const uint8_t ROOM_PERM_ADMIN = 3;

// Rolle fuer eine neue Anmeldung: Admin-Passwort -> Admin, alles andere
// (leer oder beliebig) -> Gast mit Leserecht. Ein leeres Admin-Passwort passt nie.
inline uint8_t roomLoginPermission(const char *given, const char *admin) {
  if (given && admin && admin[0] && strcmp(given, admin) == 0) return ROOM_PERM_ADMIN;
  return ROOM_PERM_GUEST;
}

// Beim Start (auch nach Update): ein gespeichertes Gast-Passwort loeschen und
// Gast-Lesezugriff einschalten. Liefert true, wenn die Einstellungen gespeichert werden muessen.
inline bool openRoomPrefs(char *guest_password, size_t len, uint8_t &allow_read_only) {
  bool changed = false;
  if (guest_password && len && guest_password[0]) { memset(guest_password, 0, len); changed = true; }
  if (!allow_read_only) { allow_read_only = 1; changed = true; }
  return changed;
}

// CLI-Befehle, die wieder ein Gast-Passwort setzen oder den Gastzugang abschalten wuerden.
// Liefert die Antwort fuer gesperrte Befehle, sonst nullptr.
inline const char *roomCliBlocked(const char *command) {
  if (!command) return nullptr;
  if (strncmp(command, "set guest.password", 18) == 0)
    return "ERR - kein Gast-Passwort: Gaeste lesen ohne Passwort";
  if (strncmp(command, "set allow.read.only", 19) == 0)
    return "ERR - Gast-Lesezugriff ist immer an";
  return nullptr;
}

}  // namespace disaster
