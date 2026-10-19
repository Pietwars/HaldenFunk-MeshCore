# Release von HaldenFunk-MeshCore

> Diese Datei ersetzt die Upstream-Anleitung von MeshCore (Tags `companion-v…`, `repeater-v…`, `room-server-v…`
> und die zugehörigen Upstream-Workflows). Diese Workflows gibt es in diesem Repository **nicht**; Releases
> entstehen wie unten beschrieben.

## 1. Version setzen (einzige Quelle)

Die Version steht **nur** in `platformio.local.ini`:

```ini
[hf]
version = 0.9.2
```

Alle Firmwarekennungen (`-D FIRMWARE_VERSION='"v1.18.0-hf${hf.version}…"'`) in `platformio.local.ini` und
`platformio.devices.ini` lesen diesen Wert. `tools/version.py` gibt ihn für Skripte aus
(`tools/build_device.sh`, `tools/package.sh`, `tools/gen_devices.py`, `tools/device_table.py`,
`tools/build_pages.py` → Demo-Banner, `tools/verify_package.py`).

Von Hand nachziehen: README (Kopfzeile, Projektfamilie, Gerätetabelle über `python3 tools/device_table.py`),
`CHANGELOG.md` (Abschnitt `## <Version>`), Demo-Firmwarekennung in `web/index.html`
(danach `python3 tools/embed_web.py`). `python3 tools/check_version.py` prüft das (läuft auch im CI).

## 2. Bauen

- CI „Alle Geräte (MeshCore-Konfigurator-Liste)“ (`.github/workflows/devices.yml`) baut auf `main` die Release-Geräte (`KERN`: T-Deck Plus, T-Watch S3, Heltec V2; alle Geräte mit `alle = true`)
  mit `tools/build_device.sh` und prüft jedes Paket mit `tools/verify_package.py`
  (factory.bin, firmware.bin, Teile, `spiffs.bin` bei WLAN-Portal, `SHA256SUMS`, deutsche `ANLEITUNG.md`).
- Paketaufbau ab 0.9.2: bei einer Rolle `NAME-factory.bin`, `NAME-firmware.bin`, `teile/`; bei mehreren Rollen
  `NAME-<Rolle>-…`, `teile-<Rolle>/`. Geräte mit Marken-Anzeige bekommen `vorschau/*.png`.
- Release-Boards (`tools/release_boards.json`: T-Deck Plus, T-Watch S3) werden zusätzlich im Job `release-boards`
  von „Build“ mit `tools/validate_memory.py` geprüft (Flash-/RAM-Grenzen, `memory-report.md/json`, auch im Paket).
- Lokal: `tools/build_device.sh <Gerät>` – Firmware-Builds **nacheinander**, nie parallel.

## 3. Veröffentlichen

1. Artefakte des Devices-Laufs auf `main` laden: `gh run download <run-id> -R Pietwars/HaldenFunk-MeshCore`.
2. `python3 tools/verify_package.py <Ordner mit allen Zips>` – muss „0 fehlerhaft/fehlend“ melden.
3. Flyer neu erzeugen mit `python3 tools/flyer/flyer.py` (Konfiguration `tools/flyer/flyer.json`; Druck-PDF mit QR-Codes, Digital-PDF mit Links, Vorschau) und in `docs/flyer/` committen.
4. Tag `v<Version>` auf den geprüften Commit, Vorabversion anlegen:
   `gh release create v<Version> --prerelease -R Pietwars/HaldenFunk-MeshCore --notes-file … <Zips> docs/flyer/*.pdf`.
5. Erst **nach** dem Release den Web-Flasher umstellen (ab 0.9.2 mit Update/Neuinstallation je Gerät):
   `python3 tools/flasher-next/aktualisieren.py <TDeck-Zip> <TWatchS3-Zip>` → `python3 tools/flasher_fetch.py --cfg tools/flasher-next --check`
   → Inhalt von `tools/flasher-next/` nach `docs/flasher/` übernehmen, per PR auf `main`, Pages-Lauf abwarten.
