# Release von HaldenFunk-MeshCore

> Diese Datei ersetzt die Upstream-Anleitung von MeshCore (Tags `companion-v…`, `repeater-v…`, `room-server-v…`
> und die zugehörigen Upstream-Workflows). Diese Workflows gibt es in diesem Repository **nicht**; Releases
> entstehen wie unten beschrieben.

## 1. Version setzen (einzige Quelle)

Die Version steht **nur** in `platformio.local.ini`:

```ini
[hf]
version = 0.9.1
```

Alle Firmwarekennungen (`-D FIRMWARE_VERSION='"v1.18.0-hf${hf.version}…"'`) in `platformio.local.ini` und
`platformio.devices.ini` lesen diesen Wert. `tools/version.py` gibt ihn für Skripte aus
(`tools/build_device.sh`, `tools/package.sh`, `tools/gen_devices.py`, `tools/device_table.py`,
`tools/build_pages.py` → Demo-Banner, `tools/verify_package.py`).

Von Hand nachziehen: README (Kopfzeile, Projektfamilie, Gerätetabelle über `python3 tools/device_table.py`),
`CHANGELOG.md` (Abschnitt `## <Version>`), Demo-Firmwarekennung in `web/index.html`
(danach `python3 tools/embed_web.py`). `python3 tools/check_version.py` prüft das (läuft auch im CI).

## 2. Bauen

- CI „Alle Geräte (MeshCore-Konfigurator-Liste)“ (`.github/workflows/devices.yml`) baut auf `main` alle 105 Geräte
  mit `tools/build_device.sh` und prüft jedes Paket mit `tools/verify_package.py`
  (factory.bin, Teile, `spiffs.bin` bei WLAN-Portal, `SHA256SUMS`, deutsche `ANLEITUNG.md`).
- Lokal: `tools/build_device.sh <Gerät>` – Firmware-Builds **nacheinander**, nie parallel.

## 3. Veröffentlichen

1. Artefakte des Devices-Laufs auf `main` laden: `gh run download <run-id> -R Pietwars/HaldenFunk-MeshCore`.
2. `python3 tools/verify_package.py <Ordner mit allen Zips>` – muss „0 fehlerhaft/fehlend“ melden.
3. Flyer neu erzeugen (Druck-PDF mit QR-Codes, Digital-PDF mit Links) und in `docs/flyer/` committen.
4. Tag `v<Version>` auf den geprüften Commit, Vorabversion anlegen:
   `gh release create v<Version> --prerelease -R Pietwars/HaldenFunk-MeshCore --notes-file … <Zips> docs/flyer/*.pdf`.
