# Änderungen

## 0.9.1 · 18.10.2026 · Release

Versionsangleich der Projektfamilie auf 0.9.1 (von 0.1.1-alpha). Weboberfläche und alle Funktionen von HaldenFunk-MeshCore **unverändert**; Firmwarekennung `v1.18.0-hf0.9.1`. Die Firmware-Änderungen von DisAHRsterRadio-MeshCore 0.9.1 (Bluetooth-PIN, Web ohne Admin usw.) sind hier **nicht** übernommen.

- CI und Release vorerst nur **LILYGO T-Deck / T-Deck Plus** (`LilyGo_TDeck`) und **Heltec V2** (`Heltec_v2`); übrige Geräte bleiben im Code (`devices.yml`: alle Geräte nur bei manuellem Start mit `alle=true`).
- README (Version, Projektfamilie, Geräteliste, Download) und README-Bilder aus der Live-Demo auf 0.9.1.
- Enthält die bisher unveröffentlichten Punkte unten (offener Zugang). Auf Hardware ist HaldenFunk-MeshCore 0.9.1 nicht abgenommen.

### Offener Zugang (seit 0.1.1-alpha)

- **Offenes WLAN:** `softAP(ssid, nullptr, 1, false, 4)`, gespeichertes `ap-pass` wird beim Start gelöscht; seriell/OLED „offen“.
- **Bluetooth ohne PIN:** Just Works für Bluedroid **und NimBLE** (klassische ESP32: ohne MITM/Passkey, Kopplung akzeptiert verschlüsselte, nicht PIN-authentifizierte Verbindungen), ebenso nRF52/RP2040; gespeicherte BLE-PIN wird auf 0 gesetzt, keine Zufalls-PIN mehr, App kann keine PIN setzen; nRF52 ohne Bluetooth-DFU.
- Offene Bluetooth-/Netzwerk-Companions: Nachrichten, Kontakte, Uhrzeit-Abgleich, Status; Betreiberbefehle (Funk, Name, Kanäle, Schlüssel, Reset, CLI) nur USB/Admin-Bereich. Portal als eigener Transporttyp. Längen-/Pfadprüfungen für Companion-Befehle.
- Doku, Flash-Anleitung, LIESMICH und Flyer ohne Zugangsdaten-Hinweise; Hosttest `tests/open_access.test.cpp`. Hardwareabnahme steht aus.
- **Room-Server mit öffentlichem Gast-Passwort `hello`** (MeshCore-Standard): mit `hello` lesen und schreiben, ohne Passwort nur lesen (`allow.read.only` an). Beim Start wird das Gast-Passwort auf `hello` gesetzt (ersetzt leere/zufällige Werte älterer Versionen); `set guest.password` und `set allow.read.only` werden abgelehnt. Admin-Login unverändert. Portal: kein Eingabefeld, „Beitreten“ meldet automatisch mit `hello` an – Aushängen im Portal wieder möglich. Hosttest `tests/room_access.test.cpp`.

## 0.1.1-alpha · 17.10.2026 · Fix Bootschleife klassischer ESP32, schlanke Firmware

[Release v0.1.1-alpha](https://github.com/Pietwars/HaldenFunk-MeshCore/releases/tag/v0.1.1-alpha) – Vorabversion, auf keinem Board abgenommen. Firmwarekennung `v1.18.0-hf0.1.1-alpha`, Pakete `HaldenFunk-MeshCore-0.1.1-alpha-<Gerät>.zip`.

**Upgrade von 0.1.0-alpha:** Partitionstabelle unverändert – `teile-wifi/firmware.bin` an `0x10000` reicht, Einstellungen bleiben. Klassische ESP32 (NimBLE): MeshCore-App einmal neu koppeln.

### Bootschleife (PR #1)

- **Fehler:** Auf klassischen ESP32 ohne PSRAM endete der Start nach `WLAN … Passwort … Browser` in `abort()` (`operator new` → `__cxa_allocate_exception` → `abort`, aus `DisasterPortal::begin()` beim ersten `http.on("/")`) und das Gerät startete endlos neu. Ursache: Bluedroid-BLE (Controller im Dual-Mode inkl. Classic-BT) + WLAN-softAP haben den internen Heap (~195 KB) vollständig verbraucht.
- Klassische ESP32-Portal-Profile (T3 V1.6.1, Heltec V2, T-Beam, Meshadventurer) nutzen jetzt **NimBLE** (`h2zero/NimBLE-Arduino 1.4.3`, `-D DISASTER_NIMBLE`): Controller nur BLE, Classic-BT-Speicher (~15 KB) freigegeben, deutlich kleinerer Host; Flash T3 V1.6.1 85 % → 70 %. Gleicher Dienst, gleiche UUIDs, PIN-Kopplung (Bonding+MITM+SC). Bereits gekoppelte Handys einmal neu koppeln.
- WLAN-Portal startet **vor** Bluetooth; Bluetooth nur bei genug freiem Heap (`DISASTER_BLE_MIN_HEAP`, Standard 70 000 Bytes, und `DISASTER_BLE_MIN_BLOCK`).
- Start-Wächter (`DisasterBootGuard.h`): Startstufe im RTC-RAM; nach einem Absturz beim/kurz nach dem Bluetooth-Start bleibt Bluetooth bis zum Aus-/Einschalten aus, das Portal läuft weiter – keine Bootschleife mehr. Serielle `[BOOT]`/`[HEAP]`-Zeilen zeigen den Speicher nach jeder Stufe.
- Mit Bluetooth bleibt WLAN-Modem-Sleep an (Pflicht bei WLAN+BT-Koexistenz).
- **MeshCore-App bekam nach dem Koppeln keine Geräteinfo (Timeout, NimBLE):** TX-Characteristic ohne `READ_ENC/READ_AUTHEN` (sonst startet NimBLE beim Abo selbst eine Kopplung, und der erste App-Befehl scheitert), eigenes Senden über `ble_gattc_notify_custom()` mit Wiederholung bis Abo/MTU-Austausch, Sendewarteschlange 8; Bluetooth wieder vor dem Portal in `MultiSerialInterface`; `[BLE]`-Diagnose seriell.

### Speicher / schlanker Code

- **T3 V1.6.1 / TLora V2.1-1.6 wurde ohne Optimierung gebaut (-O0):** Die Upstream-Variante entfernt mit `build_unflags = -Os` die Optimierung, und das eigene `-Os` in `build_flags` wird dabei mit entfernt. Die HaldenFunk-Profile (`disaster_ttgo_wifi`, `disaster_tlora_v2_1_sx1276_wifi`, `_repeater`, `_room`) übernehmen dieses `build_unflags` nicht mehr → `-Os` wie bei allen anderen Boards (−515 KB).
- Sensorbibliotheken aus `sensor_base` (INA3221/219/226/260, AHTX0, BME280/680, BMP280/085, SHTC3, SHT4x, LPS22HB, MLX90614, VL53L0X, MicroNMEA, BSEC) werden für diese Profile nicht mehr gebaut (`lib_deps` neu, `lib_ignore` in `[hf_classic_slim]`). `ENV_INCLUDE_*` waren dort schon aus; der Sensorcode in `EnvironmentSensorManager` fällt beim Kompilieren weg, Batterie-Telemetrie bleibt.
- NimBLE-Profile (T3 V1.6.1, Heltec V2, T-Beam, Meshadventurer) bauen die Bluedroid-Bibliothek `ESP32 BLE Arduino` und `SerialBLEInterface.cpp` nicht mehr mit (`[hf_no_bluedroid]`, ca. −10 KB).
- Flash (3-MB-App-Partition): **T3 V1.6.1 85 % (0.1.0) → 69,6 % (NimBLE) → 52,4 %**; Heltec V2 70 % → 52,6 % (NimBLE) → 52,3 %; T-Beam SX1262 54,6 % → 54,3 %; Meshadventurer 53,0 % → 52,7 % (Werte ab „NimBLE“ = Stand PR #1); TTGO Repeater/Room 44 % → 29,5 %.

### Sonstiges

- **Einzige Versionsquelle** `[hf] version` in `platformio.local.ini`; alle `FIRMWARE_VERSION` nutzen `${hf.version}`, Skripte lesen `tools/version.py`; `tools/check_version.py` prüft README/CHANGELOG/Demo im CI.
- `tools/verify_package.py`: prüft jedes Geräte-Paket (factory.bin, Teile, `spiffs.bin` bei WLAN-Portal, `SHA256SUMS`, deutsche `ANLEITUNG.md`), läuft im Geräte-CI.
- Workflows: `if-no-files-found: error` bei allen `upload-artifact`-Schritten.
- Ungenutztes `-D FW_VARIANT_T3V161=1` entfernt; `RELEASE.md` beschreibt jetzt den HaldenFunk-Release statt der Upstream-Tags.
- `ANLEITUNG.md` der Pakete: Hinweis zum Upgrade von 0.1.0-alpha und zum neuen Koppeln (NimBLE).
- README: Download-Link auf das Release, Upgrade-Hinweis 0.1.0 → 0.1.1, Flash-Tabelle; Demo-Banner mit Versionsnummer; Flyer 0.1.1-alpha.
- `docs/nrf52_power_management.md`: tote infocenter-Links durch docs.nordicsemi.com ersetzt.

### Funkprofil EU/UK (Narrow) (PR #3)

- Standard-Funkprofil vereinheitlicht auf das MeshCore-Preset **EU/UK (Narrow): 869,618 MHz, 62,5 kHz, SF8, CR 4/8**. Neu: `-D LORA_CR=8` global in `platformio.ini` – bisher bekamen nur die eigenen Profile CR 4/8, die übrigen Geräte der Matrix fielen auf CR 4/5 zurück. Sendeleistung unverändert (nicht erhöht), Syncword MeshCore-Standard.
- Doku (`docs/cli_commands.md` u. a.): Standardwerte `869.618,62.5,8,8`.

## 0.1.0-alpha · 17.10.2026 · erste Vorabversion HaldenFunk-MeshCore

[Release v0.1.0-alpha](https://github.com/Pietwars/HaldenFunk-MeshCore/releases/tag/v0.1.0-alpha) – Vorabversion, auf keinem Board abgenommen.

- **Karten verkleinert** (`tools/shrink_maps.py`): Linien vereinfacht (Douglas-Peucker), Koordinaten gerastert, auf den Kartenausschnitt zugeschnitten, kleine Wald-/Wasserflächen und kurze Wegstücke weggelassen; Orte, Halden und Schächte vollständig. Alle Karten 633 KB → 223 KB, Übersicht Mansfelder Land 335 KB → 90 KB. Flash T3 V1.6.1: 98 % → 85 %, T3-S3 62 %, Heltec V2 70 %. Demo und Vorschaubild der Karte aktualisiert.
- Neues Repository, abgeleitet von [DisAHRsterRadio-MeshCore](https://github.com/Pietwars/DisAHRsterRadio-MeshCore) 0.9.0-alpha (Git-Historie erhalten): **MeshCore 1.18**, WLAN-Weboberfläche + Bluetooth für die offizielle MeshCore-App, Gerätematrix der MeshCore-Konfigurator-Liste.
- Regionales Design und Inhalte aus [HaldenFunk](https://github.com/Pietwars/HaldenFunk): Kupfer/Anthrazit-Farben, Hintergrundbild (Halden, Förderturm, Funkwellen), App-Symbol, Name **HaldenFunk**, Untertitel „Glück Auf – Mesh fürs Mansfelder Land“.
- Ortsliste Mansfelder Land (Sangerhausen, Hohe Linde, Eisleben, Mansfeld, Hettstedt, Allstedt, Wallhausen, Kelbra, Roßla, Kyffhäuser, Stolberg, Wippra) und 5 Offline-Karten (OSM, ODbL) statt Ahrtal.
- WLAN-Name `HaldenFunk-XXXXXX` mit geräteindividuellem Zufallspasswort; Knotennamen `HaldenFunk-…`; Firmwarekennung `v1.18.0-hf0.1.0-alpha`; Paketnamen `HaldenFunk-MeshCore-0.1.0-alpha-<Gerät>.zip`.
- Live-Demo mit Beispieldaten aus dem Mansfelder Land: https://pietwars.github.io/HaldenFunk-MeshCore/
- README: Projektfamilie inkl. HaldenFunk und HaldenFunk-MeshCore; Hinweis: nicht funkkompatibel mit HaldenFunk/DisAHRsterRadio (LoRaLayer2).

**Im Notfall immer 112.**

---

Historie der Basis DisAHRsterRadio-MeshCore:

### 0.9.0-alpha-Nachtrag · 17.10.2026 · Vereinheitlichung

- README: Abschnitt „Projektfamilie DisAHRsterRadio“ (in allen drei Repositorys identisch) mit Versionen, Live-Demos, Release, Flyern und Namensregel (DisAHRsterRadio = Projekt/Marke, MeshAhrtal = Weboberfläche, DisasterMesh = Original-Repo).
- Vorschaubilder aller sechs Bereiche neu aus der Live-Demo aufgenommen (Handygröße, aktuelles Vollbild-Design) als `docs/images/funktion-*.jpg`; alte Bilder `darmc-*.png` und PNG-Fassungen entfernt.
- Flyer `docs/flyer/DisAHRsterRadio-MeshCore-Flyer.pdf` (Druck, QR-Codes zu Release und Demo) und `…-Flyer-digital.pdf` (klickbare Links zu Repository, Release, Live-Demo, Projektseite); in der Live-Demo unter `/flyer/` verlinkt.
- Boardtabelle mit Flash-Belegung der 3-MB-App-Partition, Build-Stand 105 Geräte.

### 0.9.0-alpha · 16.10.2026 · erste Vorabversion

[Release v0.9.0-alpha](https://github.com/Pietwars/DisAHRsterRadio-MeshCore/releases/tag/v0.9.0-alpha) – Vorabversion, auf keinem Board abgenommen.

- MeshCore auf **1.18.0** (Upstream `main`, Commit `81dbae59`), Firmwarekennung `v1.18.0-dar0.9.0-alpha`.
- Builds für alle 105 Geräte der MeshCore-Konfigurator-Liste (202 Builds): ESP32-Geräte mit WLAN-Portal (36), nRF52/RP2040/STM32 als Standard-Companion, Repeater und Room-Server ohne Web-Oberfläche; Ausnahmen mit Begründung in der Gerätetabelle (ThinkNode M7, M5Stack Unit C6L, ThinkNode M8 Repeater/Room).
- WLAN-Portal und Bluetooth für die offizielle MeshCore-App gleichzeitig; zufällige Bluetooth-PIN pro Gerät (seriell/OLED).
- Partition `huge_app.csv` (3 MB App); `factory.bin` einmalig an Adresse 0x0 flashen.
- Heltec V2: gestaffelter WLAN-Start gegen Brownout (ungeprüft).
- Weboberfläche MeshAhrtal (kompakt): Chat, Schnellmeldung, Brett, Karte, Standort, Gerät; Hintergrundbild in voller Breite.
- Live-Demo auf GitHub Pages: https://pietwars.github.io/DisAHRsterRadio-MeshCore/

**Im Notfall immer 112.**

