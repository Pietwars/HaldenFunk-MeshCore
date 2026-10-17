# Verbinden ohne Passwort

Dieser Entwicklungsstand öffnet den lokalen WLAN-Zugang. Bestehende Releases und
deren Flyer beschreiben teilweise noch den bisherigen Zugang mit Passwort.

1. Gerät einschalten und am Handy `DisAHRsterRadio-XXXXXX` beziehungsweise
   `HaldenFunk-XXXXXX` auswählen. Es wird kein WLAN-Passwort eingegeben.
2. Bei „Kein Internet“ mit diesem WLAN verbunden bleiben.
3. `http://192.168.4.1` öffnen. Chat, Karte und Meldungen brauchen kein Benutzerkonto.

Der Zugangspunkt stellt lokale Kommunikation bereit, keinen Internetzugang.
Das WLAN ist unverschlüsselt. Alle Nutzer teilen denselben Knoten und dessen
Nachrichten/Kanäle. Für öffentliche Kommunikation einen eigenen Knoten verwenden.
Gespeicherte frühere AP-Passwörter werden beim Start entfernt; Identität,
Nachrichten und Betreiberpasswörter werden dadurch nicht zurückgesetzt.

## MeshCore: Bluetooth ohne PIN

Auf BLE-fähigen Companion-Geräten in der MeshCore-App den Knoten auswählen.
Die Firmware verwendet Just Works ohne PIN-Eingabe; das Betriebssystem kann
eine normale Kopplungsbestätigung anzeigen. Keine PIN `000000` eingeben.
Bestehende Kopplungen bei Verbindungsproblemen am Handy entfernen und neu verbinden.
Die gespeicherte frühere BLE-PIN wird einmalig auf 0 gesetzt.

Konfiguration, Neustart, Werkseinstellungen, private Schlüssel und beliebige CLI-
Befehle sind über offene Bluetooth-/Netzwerk-Companion-Zugänge gesperrt.
Nachrichten, Kontakte lesen/importieren, Raumanmeldung und Status bleiben verfügbar.
Konfiguration erfolgt über USB; bei WLAN-Portalen außerdem über den bestehenden
Web-Admin. Dessen Passwort ist unabhängig und bleibt erhalten. Private-Key-Antworten
von USB werden nicht an die offenen Funkschnittstellen weitergereicht.
Auf nRF52 wird der DFU-Dienst auf der offenen Companion-Verbindung nicht angeboten;
Firmwareaktualisierung erfolgt über USB. Repeater-/Room-Server-Administration bleibt
unverändert. Ein Room-Server kann weiterhin ein eigenes Raum-Passwort verlangen.

## LoRaLayer2: kombinierte WLAN-/BLE-Testprofile

DisAHRsterRadio und HaldenFunk ohne MeshCore verwenden weiterhin ihre eigenen
Funk- und Nachrichtenformate. Die Standardprofile öffnen WLAN ohne Passwort.
Die neuen Profile `t3s3-sx1262-ble`, `t3-v161-sx1276-ble` und
`heltec-v2-sx1276-ble` aktivieren zusätzlich BLE-UART ohne PIN, gleichzeitig mit WLAN.
Das ist kein MeshCore-Companion: Die MeshCore-App ist hierfür nicht kompatibel.
Der UART-Dienst verwendet Nordic-UART-UUIDs (6E400001/2/3-B5A3-F393-E0A9-E50E24DCCA9E).
Schreiben: vollständige disaster-radio-Nachricht, 4–233 Bytes, beispielsweise
`00c|<Name> Hallo`; Notifikationen bilden einen Bytestrom und können nach MTU
aufgeteilt werden. Nur Chat (`c`) und Brett (`b`) werden angenommen.
Der alte BLE-Befehl zum Abschalten des WLANs/Neustarten ist entfernt.

Diese kombinierten Profile nutzen `huge_app.csv` und eine andere Partitionierung.
Vor dem Wechsel Firmware und Daten sichern; vollständige Partitionstabelle,
Anwendung und Dateisystem zusammen flashen. Kein reines App-Update über eine alte
Partitionierung. WLAN-Standardprofile behalten ihre bisherige Partitionstabelle.
Telnet auf Port 23 ist deaktiviert; die Betreiberkonsole bleibt über USB verfügbar.

## Abnahme vor Freigabe

- Frisches Handy ohne gespeichertes WLAN/Kopplung: ohne Passwort/PIN verbinden.
- Update eines vorhandenen Geräts mit bisherigem WLAN-Passwort/BLE-PIN:
  danach ebenfalls offen, Identität und Nachrichten bleiben erhalten.
- Zwei Geräte: Chat und Brett in beide Richtungen, Neustart und Wiederverbindung.
- iPhone und Android: WLAN ohne Internet beibehalten, Bluetooth neu koppeln,
  Karte → Chat/Brett und Bildschirmtastatur prüfen.
- Vier WLAN-Clients plus BLE: wiederholte Nachrichten, RAM-Minimum, keine Resets.
- Über BLE Funkparameter, CLI, Schlüssel-Export und Reset versuchen:
  gesperrt. Danach dieselbe zulässige Konfiguration über USB/Web-Admin prüfen.
- Verkürzte Radio-/Reset-/Kontaktbefehle senden: Fehlerantwort, keine Änderung.
- LoRaLayer2-BLE: MTU 23 und 260, überlange/kurze Frames, Queue voll,
  Abmelden/Neuanmelden und ausgeschaltetes Handy während der Übertragung prüfen.

Die neuen Zugangswege sind ohne Hardwareabnahme Entwicklungsstand.
