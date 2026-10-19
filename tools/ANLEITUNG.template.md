> Offenes WLAN, MeshCore-Bluetooth ohne Zahlencode – einfach verbinden. Alte Kopplungen bei Bedarf am Handy entfernen. Details: https://github.com/Pietwars/HaldenFunk-MeshCore/blob/main/docs/OFFENER_ZUGANG.md

# @PREFIX@ für @BOARD@

MeshCore-Companion mit WLAN-Weboberfläche · Version @VERSION@ · Commit @COMMIT@ · @REPO@

> ⚠️ Kein zertifiziertes Notrufsystem. In Lebensgefahr immer **112** wählen. Diese Firmware ist **noch nicht auf echter Hardware getestet**.

## 1. Zuerst die Antenne!

**Antenne anschrauben, bevor das Board Strom bekommt.**

## Neu in @VERSION@ (HaldenFunk-MeshCore)

- Klassische ESP32 (T3 V1.6.1): Bootschleife beim Start von WLAN + Bluetooth behoben (NimBLE statt Bluedroid, WLAN-Portal zuerst, Bluetooth nur bei genug Speicher). Von 0.1.0-alpha reicht ein Update von `firmware.bin` an `0x10000` (Einstellungen bleiben); die MeshCore-App einmal neu koppeln.
- Regionalvariante fürs Mansfelder Land: Kupfer/Anthrazit-Design, Orte und Offline-Karten (Sangerhausen, Eisleben, Mansfeld/Hettstedt, Kelbra/Kyffhäuser), WLAN `HaldenFunk-XXXXXX`.

- Parallel zum WLAN-Portal kann sich die offizielle MeshCore-App per **Bluetooth** verbinden. Kopplung ohne Zahlencode; Funkkonfiguration bleibt USB/Admin-Bereich vorbehalten.
- Neue Partitionstabelle (3 MB App, kein OTA): beim Umstieg von einer älteren Version **einmal komplett mit der factory-Datei** flashen.

## 2. Erstinstallation

1. Board per USB-Datenkabel anschließen, Chrome oder Edge am PC/Mac.
2. https://espressif.github.io/esptool-js/ öffnen, **Connect**, Port wählen (T3-S3 ggf. mit BOOT gedrückt + RST einschalten).
3. Optional **Erase Flash**.
4. Flash Address **`0x0`**, Datei `@PREFIX@-@ZIP@-factory.bin`, **Program**.
5. Danach **RST** drücken.

Enthalten: Bootloader (@BOOT@), Partitionen (0x8000), boot_app0 (0xe000), Firmware mit Weboberfläche (0x10000), SPIFFS-Dateisystem (@FSOFF@).

## 3. Verbinden

Am Handy das offene WLAN `HaldenFunk-XXXXXX` auswählen und verbinden (höchstens 4 Handys, nichts eingeben). Bei „Kein Internet“ verbunden bleiben, dann `http://192.168.4.1` öffnen. Achtung: offenes, unverschlüsseltes WLAN – jede Person in Reichweite kann mitlesen und senden. Bluetooth (MeshCore-App) koppelt ohne Zahlencode; bei alten Kopplungen am Handy entkoppeln und neu verbinden.

Nur für Betreiber: Die Admin-Zugangsdaten für den Admin-Bereich (Knotenname, Kanäle, Funkprofil) erscheinen einmalig beim ersten Start über USB (115200 Baud); neu erzeugen mit 10 s BOOT-Taste.

## 4. Update (nur Firmware)

**Nur `teile/firmware.bin` an `0x10000`** flashen. **Nicht** die factory-Datei: sie überschreibt das SPIFFS mit Schlüssel (Knotenidentität), Kontakten und Verlauf.

## 5. Prüfsummen

`SHA256SUMS` – Linux/Mac `sha256sum -c SHA256SUMS`, Windows `certutil -hashfile Datei SHA256`.

| Datei | Adresse |
|---|---|
| factory.bin | 0x0 |
| teile/bootloader.bin | @BOOT@ |
| teile/partitions.bin | 0x8000 |
| teile/boot_app0.bin | 0xe000 |
| teile/firmware.bin | 0x10000 |
| teile/spiffs.bin | @FSOFF@ |

MeshCore © Scott Powell / rippleradios.com u. a. (MIT). Karten © OpenStreetMap-Mitwirkende (ODbL).

