# Flashen (HaldenFunk-MeshCore 0.9.1)

> ⚠️ Kein zertifiziertes Notrufsystem. Im Notfall immer **112**.

**Antenne anschrauben, bevor das Gerät Strom bekommt.** Die Erstinstallation (`factory.bin` an **0x0**) **überschreibt Einstellungen, Knotenidentität, Kontakte und Chatverlauf**. Ein Update ohne Datenverlust schreibt nur die Anwendung (`teile-wifi/firmware.bin` an **0x10000**).

## LILYGO T-Deck Plus: Web-Flasher (empfohlen)

**https://pietwars.github.io/HaldenFunk-MeshCore/flasher/** in Chrome oder Edge am Computer öffnen, T-Deck Plus per USB-C-Datenkabel anschließen, „Firmware auf T-Deck Plus installieren“ klicken, Port wählen, bestätigen, danach neu starten. Der Flasher schreibt genau die `factory.bin` aus dem [Release v0.9.1](https://github.com/Pietwars/HaldenFunk-MeshCore/releases/tag/v0.9.1) an 0x0 (SHA-256 `ac292078282cba703c5c0f09d9a0a3872e1334d9e4106330ea576af04759a853`). HaldenFunk-MeshCore 0.9.1 ist auf Hardware noch nicht abgenommen (gebaut vom CI, Datei aus dem Release v0.9.1).

Nur für das T-Deck Plus. Heltec V2 und andere Boards: vorerst manuell (unten).

## Manuell mit esptool (alle Boards im Release)

Paket aus dem [Release v0.9.1](https://github.com/Pietwars/HaldenFunk-MeshCore/releases/tag/v0.9.1) laden und entpacken, Prüfsummen kontrollieren (`sha256sum -c SHA256SUMS`, Windows: `certutil -hashfile Datei SHA256`). esptool installieren: `python -m pip install esptool` (Version 4.x oder 5.x).

### Port finden

| System | So findest du den Port |
|---|---|
| Windows | Geräte-Manager → „Anschlüsse (COM & LPT)“, z. B. `COM5` (Heltec V2: „Silicon Labs CP210x“; T-Deck Plus: „USB Serial Device“/„USB JTAG/serial debug unit“). Fehlt der Port: Treiber CP210x von Silicon Labs installieren (Heltec V2). |
| macOS | `ls /dev/cu.*` – T-Deck Plus `/dev/cu.usbmodem…`, Heltec V2 `/dev/cu.SLAB_USBtoUART` oder `/dev/cu.usbserial-…` |
| Linux | `ls /dev/ttyACM* /dev/ttyUSB*` – T-Deck Plus meist `/dev/ttyACM0`, Heltec V2 `/dev/ttyUSB0`; Benutzer in Gruppe `dialout` (`sudo usermod -aG dialout $USER`, neu anmelden) |

Kein Port bzw. „Failed to connect“: anderes (Daten-)Kabel; Download-Modus erzwingen – T-Deck Plus: **Trackball gedrückt halten** und einschalten bzw. Reset; Heltec V2: **PRG** gedrückt halten und **RST** kurz drücken.

### Erstinstallation (löscht Einstellungen)

`PORT` ersetzen (z. B. `COM5`, `/dev/ttyACM0`):

```
# LILYGO T-Deck Plus (ESP32-S3), Bootloader liegt bei 0x0:
python -m esptool --chip esp32s3 --port PORT --baud 921600 write_flash 0x0 HaldenFunk-MeshCore-0.9.1-LilyGo_TDeck-wifi-factory.bin
```

Heltec WiFi LoRa 32 V2 (klassischer ESP32): ebenfalls die `…-factory.bin` aus dem Heltec-Paket an **0x0** (sie enthält den Bootloader bereits an der richtigen Stelle 0x1000):

```
python -m esptool --chip esp32 --port PORT --baud 460800 write_flash 0x0 <Heltec-Paket>-factory.bin
```

### Update ohne Datenverlust

```
python -m esptool --chip esp32s3 --port PORT --baud 921600 write_flash 0x10000 teile-wifi/firmware.bin   # T-Deck Plus
python -m esptool --chip esp32 --port PORT --baud 460800 write_flash 0x10000 teile-wifi/firmware.bin   # Heltec V2
```

Nur innerhalb derselben Version/Partitionierung verwenden; bei Problemen die Erstinstallation machen.

### Ohne Installation: esptool-js im Browser

https://espressif.github.io/esptool-js/ in Chrome/Edge öffnen → **Connect** → Port wählen → Flash Address `0x0`, Datei `…-factory.bin` → **Program** → danach Reset. (Update: `firmware.bin` an `0x10000`.)

## Danach

Offenes WLAN `HaldenFunk-XXXXXX` wählen (ohne Passwort) und **http://192.168.4.1** öffnen. Details zu Bluetooth und Zugängen: [README](../README.md) und die `ANLEITUNG.md` im Paket.
