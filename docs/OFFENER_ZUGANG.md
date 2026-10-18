# Verbinden – offen, ohne Eingabe

## WLAN-Portal (ESP32-Geräte mit Weboberfläche)

1. Gerät einschalten und am Handy das WLAN `HaldenFunk-XXXXXX` auswählen – es ist offen, es muss nichts eingegeben werden.
2. Bei „Kein Internet“ mit diesem WLAN verbunden bleiben.
3. `http://192.168.4.1` öffnen. Chat, Karte und Meldungen brauchen kein Benutzerkonto.

Der Zugangspunkt stellt lokale Kommunikation bereit, keinen Internetzugang.
Das WLAN ist unverschlüsselt: Jede Person in Reichweite kann mitlesen und senden.
Alle Nutzer teilen denselben Knoten und dessen Nachrichten/Kanäle. Höchstens 4 Handys je Gerät.
Beim Update von einer älteren Firmware wird der früher gespeicherte WLAN-Schlüssel beim Start gelöscht;
Identität, Kontakte und Verlauf bleiben erhalten.

## MeshCore-App über Bluetooth – ohne PIN

Auf BLE-fähigen Companion-Geräten in der offiziellen MeshCore-App den Knoten auswählen.
Die Firmware koppelt per „Just Works“ (verschlüsselt, ohne Zahlencode). Das Handy zeigt
höchstens eine normale Kopplungsbestätigung („Koppeln?“) – einfach bestätigen, nichts eintippen.
War das Gerät früher mit Zahlencode gekoppelt: Kopplung am Handy entfernen und neu verbinden.
Ein früher gespeicherter Code wird beim Start auf 0 (aus) gesetzt; die App kann keinen neuen setzen.

Über die offene Bluetooth-Verbindung funktionieren Nachrichten (Kanäle, Direkt), Kontakte lesen/
teilen/importieren, Uhrzeit-Abgleich, Bekanntgabe (Advert), Raumanmeldung, Status und Telemetrie.
Gesperrt (Antwort „deaktiviert“) sind Betreiberfunktionen: Funkparameter, Knotenname, Kanäle
anlegen, Standort setzen, Neustart, Werkseinstellungen, private Schlüssel, Signieren, Rohpakete
und CLI. Diese laufen über USB oder den Admin-Bereich der Weboberfläche (nur Betreiber).
Private-Key-Antworten an USB werden nie an Funk-/Bluetooth-Clients weitergereicht.
Auf nRF52 wird das Bluetooth-DFU nicht angeboten; Firmware-Updates per USB.

## Schwarzes Brett (Room-Server) – öffentliches Gast-Passwort `hello`

Der Room-Server (`_room`-Firmware) nutzt das öffentlich bekannte MeshCore-Standard-Gast-Passwort **`hello`**.
In der MeshCore-App den Raum auswählen und mit `hello` anmelden: Aushänge lesen und schreiben.
Ohne Passwort angemeldet kann man nur lesen. Im Portal unter „Brett“ auf „Beitreten“ tippen – das Portal
meldet sich automatisch mit `hello` an, es muss nichts eingetippt werden. Ein von älteren Versionen
gespeichertes (leeres oder zufälliges) Gast-Passwort wird beim Start durch `hello` ersetzt.
Verwaltung bleibt dem Betreiber über die Admin-Anmeldung vorbehalten (unverändert).

## Abnahme vor Freigabe (Hardware)

- Frisches Handy ohne gespeichertes WLAN/Kopplung: WLAN verbinden ohne Eingabe; MeshCore-App
  (Android und iPhone) koppelt ohne Zahlencode, lädt Geräteinfo, Kontakte, Kanäle, synchronisiert die Uhr.
- Update eines Geräts mit älterer Firmware (früher mit WLAN-Schlüssel/Bluetooth-Code):
  danach ebenfalls offen, Identität und Nachrichten bleiben erhalten; alte Kopplung entfernen.
- Zwei Geräte: Kanal- und Direktnachrichten in beide Richtungen, Neustart und Wiederverbindung.
- Room-Server nach Update: in der MeshCore-App mit `hello` anmelden, Aushänge lesen und schreiben; ohne
  Passwort nur lesen. Admin-Anmeldung mit dem bisherigen Admin-Passwort funktioniert weiter.
- Vier WLAN-Clients plus BLE: wiederholte Nachrichten, RAM-Minimum, keine Resets.
- Über die App Funkparameter, Name, Schlüssel-Export und Reset versuchen: „deaktiviert“, keine Änderung.
  Danach dieselbe Konfiguration über USB bzw. Admin-Bereich prüfen.

Die offenen Zugangswege sind ohne Hardwareabnahme Entwicklungsstand.
