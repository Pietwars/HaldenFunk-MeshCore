# HaldenFunk-MeshCore

**Glück Auf – Mesh fürs Mansfelder Land**

**🔴 Live-Demo (simulierte Daten, kein Gerät nötig): https://pietwars.github.io/HaldenFunk-MeshCore/**

**Version 0.1.0-alpha** · MeshCore **1.18** (Upstream `main` nach v1.18.0, Commit `81dbae59`) · Firmwarekennung `v1.18.0-hf0.1.0-alpha` · **Flyer (A5):** [Druck-PDF mit QR-Codes](docs/flyer/HaldenFunk-MeshCore-Flyer.pdf) · [Digital-PDF mit klickbaren Links](docs/flyer/HaldenFunk-MeshCore-Flyer-digital.pdf)

**HaldenFunk-MeshCore ist für [HaldenFunk](https://github.com/Pietwars/HaldenFunk), was [DisAHRsterRadio-MeshCore](https://github.com/Pietwars/DisAHRsterRadio-MeshCore) für DisAHRsterRadio ist:** der Code von DisAHRsterRadio-MeshCore (MeshCore 1.18, WLAN-Weboberfläche plus Bluetooth für die offizielle MeshCore-App, Gerätematrix der MeshCore-Konfigurator-Liste) mit dem regionalen Design und den Inhalten von HaldenFunk: Kupfer/Anthrazit, Haldenmotiv als Hintergrund, Orte und Offline-Karten des Mansfelder Landes (Sangerhausen, Lutherstadt Eisleben, Mansfeld/Hettstedt, Kelbra/Roßla/Kyffhäuser).

Das Board ist ein MeshCore-Companion und spannt ein eigenes WLAN **`HaldenFunk-XXXXXX`** (Zufallspasswort pro Gerät) auf. Man verbindet sich mit dem Handy und nutzt im Browser Chat, Schnellmeldungen, Brett, Karte und Standort – ohne App und ohne Internet. Parallel kann sich die MeshCore-App per Bluetooth verbinden.

> ⚠️ **Kein zertifiziertes Notrufsystem.** In Lebensgefahr immer **112** wählen, wenn irgendwie möglich.

> 📡 **Nicht funkkompatibel mit [HaldenFunk](https://github.com/Pietwars/HaldenFunk) und [DisAHRsterRadio](https://github.com/Pietwars/DisAHRsterRadio):** Diese beiden nutzen kein MeshCore, sondern LoRaLayer2 (disaster-radio). HaldenFunk-MeshCore spricht das normale MeshCore-Funkprotokoll und versteht sich mit anderen MeshCore-Knoten (z. B. DisAHRsterRadio-MeshCore, DisasterMesh) bei gleichem Funkprofil.

### Projektfamilie

| Repository | Inhalt | Funk | Version | Demo |
|---|---|---|---|---|
| [Pietwars/HaldenFunk-MeshCore](https://github.com/Pietwars/HaldenFunk-MeshCore) | **dieses Repo:** MeshCore 1.18, WLAN-Oberfläche im HaldenFunk-Design + Bluetooth für die MeshCore-App, Gerätematrix | MeshCore | 0.1.0-alpha | [Live-Demo](https://pietwars.github.io/HaldenFunk-MeshCore/) |
| [Pietwars/HaldenFunk](https://github.com/Pietwars/HaldenFunk) | Mansfelder-Land-Variante von DisAHRsterRadio, **ohne MeshCore** (LoRaLayer2) | LoRaLayer2 | 0.1.1-alpha | [Live-Demo](https://pietwars.github.io/HaldenFunk/) |
| [Pietwars/DisAHRsterRadio-MeshCore](https://github.com/Pietwars/DisAHRsterRadio-MeshCore) | Ahrtal-Original dieser Firmware: MeshCore 1.18, WLAN + Bluetooth, 105 Geräte | MeshCore | 0.9.0-alpha | [Live-Demo](https://pietwars.github.io/DisAHRsterRadio-MeshCore/) |
| [Pietwars/DisAHRsterRadio](https://github.com/Pietwars/DisAHRsterRadio) | Ahrtal, **ohne MeshCore**: modernisiertes disaster-radio | LoRaLayer2 | 0.1.1-dev | [Live-Demo](https://pietwars.github.io/DisAHRsterRadio/) |
| [Pietwars/DisasterMesh](https://github.com/Pietwars/DisasterMesh) | Original-Repo: MeshCore 1.18 mit großer Weboberfläche MeshAhrtal | MeshCore | 0.9.0-alpha | [Live-Demo](https://pietwars.github.io/DisasterMesh/demo/) |

Gleiche Spalte „Funk“ = können miteinander funken (gleiches Funkprofil vorausgesetzt). Alle Demos sind simuliert. Hardware-Abnahme steht für alle MeshCore-Varianten aus.

**Download:** noch kein Release. Firmware-Pakete gibt es als Artefakte der [GitHub Actions](https://github.com/Pietwars/HaldenFunk-MeshCore/actions) (ein Paket pro Gerät).

## Funktionen im Überblick

| Chat | Schnellmeldung | Brett |
|---|---|---|
| <img src="docs/images/funktion-chat.jpg" width="230" alt="Chat"> | <img src="docs/images/funktion-quick.jpg" width="230" alt="Schnellmeldung"> | <img src="docs/images/funktion-board.jpg" width="230" alt="Brett"> |
| **Karte** | **Standort** | **Gerät** |
| <img src="docs/images/funktion-map.jpg" width="230" alt="Karte"> | <img src="docs/images/funktion-loc.jpg" width="230" alt="Standort"> | <img src="docs/images/funktion-info.jpg" width="230" alt="Gerät"> |

(Aufnahmen der [Live-Demo](https://pietwars.github.io/HaldenFunk-MeshCore/) in Handygröße, Stand 0.1.0-alpha; alle Daten simuliert.)

- **Chat:** öffentlicher Kanal, eigene Kanäle und Direktnachrichten, Verlauf auf dem Board
- **Schnellmeldung:** Vorlagen wie Trinkwasser, Strom, Medikamente, Verletzt, Evakuierung mit Ortsangabe
- **Brett:** Schwarzes Brett über einen MeshCore-Room-Server
- **Karte:** Offline-Karten des Mansfelder Landes direkt aus der Firmware, Knoten und geteilte Positionen
- **Standort:** Handy-GPS oder Ortsliste, als Chat-Position oder Knoten-Standort (Advert)
- **Gerät:** Status, Funkzähler, Kontakte, Bekanntgabe, Admin (Name, Kanäle)
- **WLAN + Bluetooth gleichzeitig:** Auf ESP32-Geräten läuft das WLAN-Portal und parallel die Bluetooth-Verbindung für die offizielle MeshCore-App (siehe unten)

## Ehrlicher Stand (Oktober 2026)

| Punkt | Stand |
|---|---|
| Basis | MeshCore `main` nach **v1.18.0** (meshcore-dev/MeshCore), Git-Historie erhalten |
| Firmware baut | ✅ 105 Geräte (202 Builds, 36 Geräte mit WLAN-Portal) per GitHub Actions, Gerätematrix im CI (Status: [Actions](https://github.com/Pietwars/HaldenFunk-MeshCore/actions)) |
| Auf echter Hardware getestet | ❌ **diese Firmware noch nicht.** LILYGO T3 V1.6.1 lief beim Nutzer mit der Schwester-Firmware DisAHRsterRadio (gleiche Basis wie HaldenFunk) vollständig; Heltec V2 zeigte dort ein Strom-/Brownout-Problem (WLAN-AP fehlte), Gegenmaßnahme hier enthalten, ungeprüft |
| Kompatibel mit anderen MeshCore-Geräten/Apps | ✅ normales MeshCore-Funkprotokoll (gleiches Funkprofil vorausgesetzt) |

## Kartenbedienung und Speicheroptimierung

Die Navigation bleibt unten in zwei Reihen erreichbar (einschließlich iPhone-Safe-Area).
Die Kartenzeichenfläche ist auf ihren Container begrenzt; Zoomtasten sind 44 × 44 Pixel groß.
Im Browser werden maximal zwei Karten zwischengespeichert. Beim schnellen Wechsel gewinnt
immer die zuletzt gewählte Karte. Gleichzeitige Statusabfragen werden vermieden.

Die Weboberfläche wird beim Bauen gzip-komprimiert und direkt aus PROGMEM ausgeliefert.
Die genauen Roh- und gzip-Größen zeigt `python tools/embed_web.py` an (derzeit rund 19,6 kB
Ersparnis gegenüber derselben unkomprimierten Seite). `/api/state` überträgt Kontakte und
Kanäle stückweise, statt dafür einen großen Gesamt-String im Geräte-RAM anzulegen.
Unter „Gerät“ erscheinen freier RAM, bisheriges RAM-Minimum und größter freier Block.
Karten, Bluetooth, Kontaktlimit und Nachrichtenwarteschlange bleiben erhalten.

Regressionstest: `node tests/web_regression.cjs` (Node 22); prüft JavaScript-Syntax,
gzip-Roundtrip, CSS-Selektoren, Kartencache und überholte Ladeantworten. Dies ersetzt
keinen Safari-/Hardwaretest. Vor Freigabe auf iPhone Hoch-/Querformat, Tastatur,
Karte → Chat/Brett und auf dem Board mehrere WLAN-Clients sowie BLE parallel prüfen.

## Boards

| PlatformIO-Env | Board | App-Partition belegt (3 MB, `huge_app.csv`) |
|---|---|---|
| `disaster_t3s3_sx1262_wifi` | LILYGO T3-S3 SX1262 | ca. 62 % |
| `disaster_tlora_v2_1_sx1276_wifi` | LILYGO T3 V1.6.1 SX1276 | ca. 85 % (Karten verkleinert, ca. 470 KB frei) |
| `disaster_heltec_v2_sx1276_wifi` | Heltec WiFi LoRa 32 V2 SX1276 | ca. 70 % |
| `disaster_t3s3_sx1262_repeater` / `_room` | T3-S3 SX1262 als Repeater / Room-Server (Brett) | ca. 29 % |
| `disaster_ttgo_repeater` / `_room` | T3 V1.6.1 / TTGO LoRa32 als Repeater / Room-Server | ca. 44 % |

Weitere Profile (T3-S3 SX1276, SD-Archiv, Heltec-Diagnose) sind in `platformio.local.ini` enthalten, aber nicht im CI.

Heltec V2 mit WLAN-Fix: Brownout-Detektor früh aus, Startmarken `[DIAG …]` seriell, WLAN startet zuletzt und erst 2 s nach dem Funkmodul, WLAN-Sendeleistung 8,5 dBm.

Alle Boards: WLAN **`HaldenFunk-XXXXXX`** mit geräteindividuellem Zufallspasswort, gespeichert im NVS. Das Passwort steht beim Start und danach alle 30 s seriell (115200 Baud) und erscheint abwechselnd mit dem WLAN-Namen auf dem OLED. Startet der Zugangspunkt nicht, versucht es das Gerät alle 5 s erneut.

## WLAN-Portal und MeshCore-App gleichzeitig (Bluetooth)

Auf ESP32-Geräten mit Bluetooth sind Web-Portal und BLE-Companion beide am selben Companion angemeldet (MeshCore `MultiSerialInterface`): Jede Antwort und jede eingehende Nachricht geht an beide. Die offizielle MeshCore-App verbindet sich per Bluetooth, während Handys im WLAN die Weboberfläche nutzen.

- **Bluetooth-PIN:** pro Gerät zufällig (6-stellig), beim ersten Start erzeugt und gespeichert. Steht seriell (115200 Baud, Zeile `[BLE] MeshCore-App: Bluetooth-PIN …`, alle 30 s wiederholt) und auf dem Display.
- **Grenzen:** Ruft die App Nachrichten aus der Warteschlange ab, sieht das Portal sie im Verlauf mit, die App aber nicht umgekehrt die Abrufe des Portals. Gleichzeitige Befehle aus App und Portal können sich in seltenen Fällen kreuzen. WLAN + Bluetooth + Webserver brauchen viel RAM; auf klassischen ESP32 (T3 V1.6.1, Heltec V2) ist das auf Hardware **nicht geprüft**.
- **Speicher:** Portal-Profile nutzen jetzt die Partition `huge_app.csv` (3 MB App, kein OTA, 896 KB SPIFFS). T3 V1.6.1: ca. 85 % von 3 MB, T3-S3: 62 %, Heltec V2: 70 %. Nach dem Wechsel von 0.1.0/älter einmal komplett mit `factory.bin` flashen (Partitionstabelle ändert sich, Identität wird neu erzeugt).

## So wird die Oberfläche auf MeshCore abgebildet

| Reiter | MeshCore |
|---|---|
| **Chat** | Öffentlicher Kanal, weitere Kanäle (Slots 0–7) und Direktnachrichten an bekannte Chat-Kontakte |
| **Schnellmeldung** | Vorlagen (Trinkwasser, Strom, Medikamente …) mit Ort, gesendet an Kanal oder Kontakt |
| **Brett** | MeshCore-**Room-Server**: anmelden mit Raum-Passwort, Beiträge lesen und schreiben |
| **Karte** | Offline-OSM-Karten (Mansfelder Land, Sangerhausen, Lutherstadt Eisleben, Mansfeld/Hettstedt, Kelbra/Roßla/Kyffhäuser) aus der Firmware. Angezeigt werden Knoten mit Standort in ihrer Bekanntgabe (Advert), Positionen aus Nachrichten und die eigene Position |
| **Standort** | GPS des Handys oder Ortsliste. „Position in Chat senden“ (Format `!DP1\|lat,lon\|zeit`, kompatibel zu DisasterMesh) oder als Knoten-Standort setzen und per Advert bekanntgeben |
| **Gerät** | Status, Funkzähler, Kontakte neu lesen, Advert senden; Admin: Knotenname, Kanal mit Schlüssel |

Nachrichten sind bei MeshCore auf **100 Byte** begrenzt.

## Bauen und Flashen

```bash
pip install platformio
python tools/embed_web.py                       # nach Änderungen an web/
pio run -e disaster_t3s3_sx1262_wifi -t upload
pio run -e disaster_t3s3_sx1262_wifi -t uploadfs  # nur einmal! (Dateisystem, löscht Identität)
tools/package.sh                                 # Flashpakete HaldenFunk-MeshCore-0.1.0-alpha-<board>.zip
```

Die Weboberfläche liegt in `web/index.html`. `tools/embed_web.py` bettet die gzip-komprimierte Seite, Hintergrundbild (WebP, 20 KB) und Karten (gzip, 223 KB) in `examples/companion_radio/PortalPage.h` und `PortalAssets.h` ein. Demo ohne Gerät: `web/index.html?demo=1` lokal öffnen.

## Alle Geräte (MeshCore-Konfigurator-Liste)

**Funkprofil (alle Geräte, alle Rollen):** MeshCore-Standard-Preset **EU/UK (Narrow)** – **869,618 MHz, Bandbreite 62,5 kHz, SF8, CR 4/8**, MeshCore-Standard-Syncword (privat, 0x12). Gesetzt über die Build-Flags `LORA_FREQ=869.618`, `LORA_BW=62.5`, `LORA_SF=8`, `LORA_CR=8` in `platformio.ini` (gilt für alle 105 Geräte der Matrix und die eigenen Profile). Teilband 869,4–869,65 MHz (SRD, ETSI EN 300 220 / BNetzA): max. 500 mW ERP (27 dBm), 10 % Sendezeit; die Firmware bleibt bei der bisherigen boardsicheren Sendeleistung (nicht erhöht). Alle Knoten eines Netzes müssen dasselbe Profil nutzen – Geräte mit älterem Stand (CR 4/5) einmal in der Weboberfläche auf „EU/UK Narrow“ stellen oder per CLI `set radio 869.618,62.5,8,8`.

Gebaut werden alle 105 Geräte, für die MeshCore v1.18.0 Firmware anbietet (Quelle der Liste im [MeshCore-Konfigurator](https://meshcore.co.uk/configurator/): `flasher.meshcore.io/releases`, gespeichert in `tools/devices.upstream.json`, Envs aus `tools/gen_devices.py` → `platformio.devices.ini`). ESP32-Geräte mit Companion-Firmware bekommen das DisAHRsterRadio-WLAN-Portal (+ Bluetooth), alle anderen die normale MeshCore-Firmware **ohne Web-Oberfläche** (Companion, Repeater, Room-Server; Funkprofil MeshCore-Standard EU 869,618 MHz / 62,5 kHz / SF8). Nur der LILYGO T3 V1.6.1 ist auf Hardware geprüft (mit der Schwester-Firmware).

| Gerät | Chip | Web-Oberfläche | Inhalt | Datei |
|---|---|---|---|---|
| Ebyte_EoRa-S3 | esp32s3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-Ebyte_EoRa-S3.zip` |
| GAT562_30S_Mesh_Kit | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-GAT562_30S_Mesh_Kit.zip` |
| GAT562_Mesh_EVB_Pro | nrf52 | nein (ohne Web-Oberfläche) | Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-GAT562_Mesh_EVB_Pro.zip` |
| GAT562_Mesh_Tracker_Pro | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-GAT562_Mesh_Tracker_Pro.zip` |
| GAT562_Mesh_Watch13 | nrf52 | nein (ohne Web-Oberfläche) | Companion | `HaldenFunk-MeshCore-0.1.0-alpha-GAT562_Mesh_Watch13.zip` |
| Generic_E22_sx1262 | esp32 | nein (ohne Web-Oberfläche) | Repeater | `HaldenFunk-MeshCore-0.1.0-alpha-Generic_E22_sx1262.zip` |
| Generic_E22_sx1268 | esp32 | nein (ohne Web-Oberfläche) | Repeater | `HaldenFunk-MeshCore-0.1.0-alpha-Generic_E22_sx1268.zip` |
| Heltec_ct62 | esp32c3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-Heltec_ct62.zip` |
| Heltec_E213 | esp32s3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-Heltec_E213.zip` |
| Heltec_E290 | esp32s3 | nein (ohne Web-Oberfläche) | Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-Heltec_E290.zip` |
| Heltec_mesh_solar | nrf52 | nein (ohne Web-Oberfläche) | Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-Heltec_mesh_solar.zip` |
| heltec_rc32 | esp32s3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-heltec_rc32.zip` |
| heltec_rc32_without_display | esp32s3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-heltec_rc32_without_display.zip` |
| Heltec_t096 | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-Heltec_t096.zip` |
| Heltec_t1 | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-Heltec_t1.zip` |
| Heltec_t114 | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-Heltec_t114.zip` |
| Heltec_t114_without_display | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-Heltec_t114_without_display.zip` |
| Heltec_tower_v2 | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-Heltec_tower_v2.zip` |
| heltec_tracker_v2 | esp32s3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-heltec_tracker_v2.zip` |
| Heltec_v2 | esp32 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-Heltec_v2.zip` |
| Heltec_v3 | esp32s3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-Heltec_v3.zip` |
| heltec_v4 | esp32s3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-heltec_v4.zip` |
| heltec_v4_expansionkit | esp32s3 | nein (ohne Web-Oberfläche) | Repeater | `HaldenFunk-MeshCore-0.1.0-alpha-heltec_v4_expansionkit.zip` |
| heltec_v4_r8 | esp32s3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-heltec_v4_r8.zip` |
| heltec_v4_r8_tft | esp32s3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-heltec_v4_r8_tft.zip` |
| heltec_v4_tft | esp32s3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-heltec_v4_tft.zip` |
| Heltec_Wireless_Paper | esp32s3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-Heltec_Wireless_Paper.zip` |
| Heltec_Wireless_Tracker | esp32s3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-Heltec_Wireless_Tracker.zip` |
| Heltec_WSL3 | esp32s3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-Heltec_WSL3.zip` |
| ikoka_handheld_nrf_e22_30dbm | nrf52 | nein (ohne Web-Oberfläche) | Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-ikoka_handheld_nrf_e22_30dbm.zip` |
| ikoka_handheld_nrf_e22_30dbm_096 | nrf52 | nein (ohne Web-Oberfläche) | Companion | `HaldenFunk-MeshCore-0.1.0-alpha-ikoka_handheld_nrf_e22_30dbm_096.zip` |
| ikoka_handheld_nrf_e22_30dbm_096_rotated | nrf52 | nein (ohne Web-Oberfläche) | Companion | `HaldenFunk-MeshCore-0.1.0-alpha-ikoka_handheld_nrf_e22_30dbm_096_rotated.zip` |
| ikoka_nano_nrf_22dbm | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-ikoka_nano_nrf_22dbm.zip` |
| ikoka_nano_nrf_30dbm | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-ikoka_nano_nrf_30dbm.zip` |
| ikoka_nano_nrf_33dbm | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-ikoka_nano_nrf_33dbm.zip` |
| ikoka_stick_nrf_22dbm | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-ikoka_stick_nrf_22dbm.zip` |
| ikoka_stick_nrf_30dbm | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-ikoka_stick_nrf_30dbm.zip` |
| ikoka_stick_nrf_33dbm | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-ikoka_stick_nrf_33dbm.zip` |
| KeepteenLT1 | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-KeepteenLT1.zip` |
| LilyGo_T-Echo | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-LilyGo_T-Echo.zip` |
| LilyGo_T-Echo-Lite | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-LilyGo_T-Echo-Lite.zip` |
| LilyGo_T-Echo-Lite_non_shell | nrf52 | nein (ohne Web-Oberfläche) | Companion | `HaldenFunk-MeshCore-0.1.0-alpha-LilyGo_T-Echo-Lite_non_shell.zip` |
| LilyGo_T-Echo_Card | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-LilyGo_T-Echo_Card.zip` |
| LilyGo_T3S3_sx1262 | esp32s3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-LilyGo_T3S3_sx1262.zip` |
| LilyGo_T3S3_sx1276 | esp32s3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-LilyGo_T3S3_sx1276.zip` |
| LilyGo_T_Impulse_Plus | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater | `HaldenFunk-MeshCore-0.1.0-alpha-LilyGo_T_Impulse_Plus.zip` |
| LilyGo_TBeam_1W | esp32s3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-LilyGo_TBeam_1W.zip` |
| LilyGo_TDeck | esp32s3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-LilyGo_TDeck.zip` |
| LilyGo_TETH_Elite_sx1262 | esp32s3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-LilyGo_TETH_Elite_sx1262.zip` |
| LilyGo_TLora_V2_1_1_6 | esp32 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-LilyGo_TLora_V2_1_1_6.zip` |
| M5Stack_Unit_C6L | esp32c6 | nein (ohne Web-Oberfläche: ESP32-C6: WLAN-Bibliothek (Network.h) im Portal-Build nicht auflösbar) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-M5Stack_Unit_C6L.zip` |
| Mesh_pocket | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-Mesh_pocket.zip` |
| Meshadventurer_sx1262 | esp32 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-Meshadventurer_sx1262.zip` |
| Meshadventurer_sx1268 | esp32 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-Meshadventurer_sx1268.zip` |
| meshnology_w12 | esp32s3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-meshnology_w12.zip` |
| Meshtiny | nrf52 | nein (ohne Web-Oberfläche) | Companion | `HaldenFunk-MeshCore-0.1.0-alpha-Meshtiny.zip` |
| MeshTracker_X1 | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-MeshTracker_X1.zip` |
| Minewsemi_me25ls01 | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-Minewsemi_me25ls01.zip` |
| muzi_base_duo | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-muzi_base_duo.zip` |
| muzi_base_duo_superIO | nrf52 | nein (ohne Web-Oberfläche) | Companion | `HaldenFunk-MeshCore-0.1.0-alpha-muzi_base_duo_superIO.zip` |
| muzi_base_uno | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-muzi_base_uno.zip` |
| muzi_base_uno_superIO | nrf52 | nein (ohne Web-Oberfläche) | Companion | `HaldenFunk-MeshCore-0.1.0-alpha-muzi_base_uno_superIO.zip` |
| Nano_G2_Ultra | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-Nano_G2_Ultra.zip` |
| PicoW | rp2040 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-PicoW.zip` |
| ProMicro | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-ProMicro.zip` |
| R1Neo | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-R1Neo.zip` |
| RAK_11310 | rp2040 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-RAK_11310.zip` |
| RAK_3112 | esp32s3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-RAK_3112.zip` |
| RAK_3401 | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-RAK_3401.zip` |
| RAK_3x72 | stm32 | nein (ohne Web-Oberfläche) | Companion, Repeater | `HaldenFunk-MeshCore-0.1.0-alpha-RAK_3x72.zip` |
| RAK_4631 | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-RAK_4631.zip` |
| RAK_WisMesh_Tag | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-RAK_WisMesh_Tag.zip` |
| SenseCap_Solar | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-SenseCap_Solar.zip` |
| Station_G2 | esp32s3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-Station_G2.zip` |
| Station_G2_logging | esp32s3 | nein (ohne Web-Oberfläche) | Repeater | `HaldenFunk-MeshCore-0.1.0-alpha-Station_G2_logging.zip` |
| Station_G3_ESP32 | esp32s3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-Station_G3_ESP32.zip` |
| Station_G3_ESP32_logging | esp32s3 | nein (ohne Web-Oberfläche) | Repeater | `HaldenFunk-MeshCore-0.1.0-alpha-Station_G3_ESP32_logging.zip` |
| t1000e | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-t1000e.zip` |
| T_Beam_S3_Supreme_SX1262 | esp32s3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-T_Beam_S3_Supreme_SX1262.zip` |
| Tbeam_SX1262 | esp32 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-Tbeam_SX1262.zip` |
| Tbeam_SX1276 | esp32 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-Tbeam_SX1276.zip` |
| Tenstar_C3_sx1262 | esp32c3 | nein (ohne Web-Oberfläche) | Repeater | `HaldenFunk-MeshCore-0.1.0-alpha-Tenstar_C3_sx1262.zip` |
| Tenstar_C3_sx1268 | esp32c3 | nein (ohne Web-Oberfläche) | Repeater | `HaldenFunk-MeshCore-0.1.0-alpha-Tenstar_C3_sx1268.zip` |
| ThinkNode_M1 | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-ThinkNode_M1.zip` |
| ThinkNode_M2 | esp32s3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-ThinkNode_M2.zip` |
| ThinkNode_M3 | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-ThinkNode_M3.zip` |
| ThinkNode_M4 | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-ThinkNode_M4.zip` |
| ThinkNode_M5 | esp32s3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-ThinkNode_M5.zip` |
| ThinkNode_M6 | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-ThinkNode_M6.zip` |
| ThinkNode_M7 | esp32s3 | nein (ohne Web-Oberfläche: eigene Display-Oberfläche ohne Hinweisfunktion (showAlert) – Portal baut nicht) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-ThinkNode_M7.zip` |
| ThinkNode_M8 | nrf52 | nein (ohne Web-Oberfläche: Repeater/Room-Server: Upstream-Linkfehler (EnvironmentSensorManager fehlt im Build)) | Companion | `HaldenFunk-MeshCore-0.1.0-alpha-ThinkNode_M8.zip` |
| Tiny_Relay | stm32 | nein (ohne Web-Oberfläche) | Companion, Repeater | `HaldenFunk-MeshCore-0.1.0-alpha-Tiny_Relay.zip` |
| waveshare_rp2040_lora | rp2040 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-waveshare_rp2040_lora.zip` |
| Waveshare_S3_LR1121 | esp32s3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-Waveshare_S3_LR1121.zip` |
| wio-e5 | stm32 | nein (ohne Web-Oberfläche) | Companion, Repeater | `HaldenFunk-MeshCore-0.1.0-alpha-wio-e5.zip` |
| wio-e5-mini | stm32 | nein (ohne Web-Oberfläche) | Companion, Repeater | `HaldenFunk-MeshCore-0.1.0-alpha-wio-e5-mini.zip` |
| wio_wm1110 | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-wio_wm1110.zip` |
| WioTrackerL1 | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-WioTrackerL1.zip` |
| WioTrackerL1-1W | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-WioTrackerL1-1W.zip` |
| WioTrackerL1Eink | nrf52 | nein (ohne Web-Oberfläche) | Companion | `HaldenFunk-MeshCore-0.1.0-alpha-WioTrackerL1Eink.zip` |
| Xiao_C3 | esp32c3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-Xiao_C3.zip` |
| Xiao_nrf52 | nrf52 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-Xiao_nrf52.zip` |
| Xiao_rp2040 | rp2040 | nein (ohne Web-Oberfläche) | Companion, Repeater, Room-Server | `HaldenFunk-MeshCore-0.1.0-alpha-Xiao_rp2040.zip` |
| Xiao_S3 | esp32s3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-Xiao_S3.zip` |
| Xiao_S3_WIO | esp32s3 | ja | WLAN-Portal | `HaldenFunk-MeshCore-0.1.0-alpha-Xiao_S3_WIO.zip` |

## Grenzen / offen

- Nicht auf Hardware getestet, auch nicht das Zusammenspiel mit MeshCore 1.18
- Karten für den Flash verkleinert (`tools/shrink_maps.py`: Linien vereinfacht, Raster ca. 7 m in der Übersicht / 2 m in den Detailkarten, kleine Wald-/Wasserflächen und kurze Wegstücke weggelassen; Orte, Halden und Schächte vollständig): 633 KB → 223 KB, Übersichtskarte 335 KB → 90 KB. T3 V1.6.1 damit bei ca. 85 % (vorher 98 %)
- Die Brett-Funktion braucht einen erreichbaren Room-Server (`disaster_t3s3_sx1262_room` bzw. `disaster_ttgo_room`, im CI gebaut, nicht auf Hardware geprüft)
- Positionsfreigabe: Wer einen Standort als Knoten-Standort setzt, schaltet das Teilen der Position in Adverts ein (bei 0,0 wieder aus)
- Funkprofil-Auswahl (Deutschland-Presets) aus DisasterMesh ist in der neuen Oberfläche nicht enthalten. Die API `/api/radio` existiert weiter
- Karten haben keine Navigation; Datenstand 17.10.2026 (aus HaldenFunk übernommen)

## Lizenz und Danksagung

- **MeshCore** © Scott Powell / rippleradios.com und Mitwirkende, **MIT-Lizenz** (`license.txt`). Original-README: [README.meshcore.md](README.meshcore.md).
- WLAN-Portal, Zustell-Warteschlange, Room-/Repeater-Anpassungen: aus [DisasterMesh](https://github.com/Pietwars/DisasterMesh) (MIT).
- Code-Basis: [DisAHRsterRadio-MeshCore](https://github.com/Pietwars/DisAHRsterRadio-MeshCore) (MIT).
- Regionales Design, Hintergrundbild, Orte und Karten: aus [HaldenFunk](https://github.com/Pietwars/HaldenFunk), vom selben Autor, hier unter MIT.
- Kartendaten © OpenStreetMap-Mitwirkende, ODbL 1.0 (`web/map/ODbL-1.0.txt`).
