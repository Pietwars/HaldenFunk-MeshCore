# Flashen (HaldenFunk-MeshCore 0.9.2)

> ⚠️ Kein zertifiziertes Notrufsystem. Im Notfall immer **112**.

**Antenne anschrauben, bevor das Gerät Strom bekommt.** Zwei Wege:

- **Neuinstallation:** `…-factory.bin` an **0x0** – **überschreibt Einstellungen, Knotenidentität, Kontakte und Chatverlauf**.
- **Update ohne Datenverlust:** nur `…-firmware.bin` an **0x10000** (T-Deck Plus: gleiche Partitionstabelle wie 0.9.1 und 0.1.x-alpha).

Paketaufbau ab 0.9.2 (wie DisAHRsterRadio-MeshCore): `HaldenFunk-MeshCore-0.9.2-<Gerät>-factory.bin`, `HaldenFunk-MeshCore-0.9.2-<Gerät>-firmware.bin`, Einzelteile in `teile/`, Vorschau der Anzeige in `vorschau/`, bei T-Deck Plus und T-Watch S3 zusätzlich `memory-report.md` (Flash-/RAM-Prüfung). Bis 0.9.1 hießen die Dateien `…-wifi-factory.bin` und `teile-wifi/firmware.bin`.

## Web-Flasher (T-Deck Plus und T-Watch S3)

**https://pietwars.github.io/HaldenFunk-MeshCore/flasher/** in Chrome oder Edge am Computer öffnen, Gerät per USB-C-Datenkabel anschließen, Knopf wählen, Port wählen, bestätigen, danach neu starten. Der Flasher nutzt die Dateien aus dem [Release v0.9.2](https://github.com/Pietwars/HaldenFunk-MeshCore/releases/tag/v0.9.2) und hat je Gerät **„Update auf 0.9.2 (Einstellungen bleiben)“** (`firmware.bin` an 0x10000) und **„Neuinstallation (alles löschen)“** (`factory.bin` an 0x0).

Heltec WiFi LoRa 32 V2 ist ab 0.9.2 nicht mehr unterstützt; das letzte Paket liegt im [Release v0.9.1](https://github.com/Pietwars/HaldenFunk-MeshCore/releases/tag/v0.9.1).

## Manuell mit esptool

Paket aus dem Release laden und entpacken, Prüfsummen kontrollieren (`sha256sum -c SHA256SUMS`, Windows: `certutil -hashfile Datei SHA256`). esptool installieren: `python -m pip install esptool` (Version 4.x oder 5.x).

### Port finden

| System | So findest du den Port |
|---|---|
| Windows | Geräte-Manager → „Anschlüsse (COM & LPT)“, z. B. `COM5` („USB Serial Device“/„USB JTAG/serial debug unit“). |
| macOS | `ls /dev/cu.*` – T-Deck Plus / T-Watch S3 `/dev/cu.usbmodem…` |
| Linux | `ls /dev/ttyACM*` – T-Deck Plus / T-Watch S3 meist `/dev/ttyACM0`; Benutzer in Gruppe `dialout` (`sudo usermod -aG dialout $USER`, neu anmelden) |

Kein Port bzw. „Failed to connect“: anderes (Daten-)Kabel; Download-Modus erzwingen – T-Deck Plus: **Trackball gedrückt halten** und einschalten bzw. Reset; T-Watch S3: USB-C neu einstecken (ggf. Seitentaste kurz drücken, falls die Uhr aus ist).

### Neuinstallation (löscht Einstellungen)

`PORT` ersetzen (z. B. `COM5`, `/dev/ttyACM0`):

```
# LILYGO T-Deck Plus (ESP32-S3):
python -m esptool --chip esp32s3 --port PORT --baud 921600 write_flash 0x0 HaldenFunk-MeshCore-0.9.2-LilyGo_TDeck-factory.bin
# LILYGO T-Watch S3 (ESP32-S3):
python -m esptool --chip esp32s3 --port PORT --baud 921600 write_flash 0x0 HaldenFunk-MeshCore-0.9.2-LilyGo_TWatchS3-factory.bin
```

### Update ohne Datenverlust

```
python -m esptool --chip esp32s3 --port PORT --baud 921600 write_flash 0x10000 HaldenFunk-MeshCore-0.9.2-LilyGo_TDeck-firmware.bin
python -m esptool --chip esp32s3 --port PORT --baud 921600 write_flash 0x10000 HaldenFunk-MeshCore-0.9.2-LilyGo_TWatchS3-firmware.bin
```

Die T-Watch S3 ist neu in 0.9.2 – beim ersten Mal (andere Firmware vorher) die Neuinstallation verwenden. Bei Problemen nach einem Update die Neuinstallation machen.

### Ohne Installation: esptool-js im Browser

https://espressif.github.io/esptool-js/ in Chrome/Edge öffnen → **Connect** → Port wählen → Flash Address `0x0`, Datei `…-factory.bin` → **Program** → danach Reset. (Update: `…-firmware.bin` an `0x10000`.)

## Danach

Offenes WLAN `HaldenFunk-XXXXXX` wählen (ohne Passwort) und **http://192.168.4.1** öffnen. MeshCore-App per Bluetooth ohne PIN. Details zu Bluetooth, Anzeige und Zugängen: [README](../README.md) und die `ANLEITUNG.md` im Paket.
