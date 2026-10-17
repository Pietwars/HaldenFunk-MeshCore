#!/usr/bin/env python3
"""Schreibt ANLEITUNG.md fuer ein Geraete-Paket."""
import json,sys
from pathlib import Path
dev,d,ver=sys.argv[1],Path(sys.argv[2]),sys.argv[3]
e={x['device']:x for x in json.load(open(Path(__file__).parent/'devices.json'))}[dev]
off={}
if (d/'.offsets').exists():
    for l in (d/'.offsets').read_text().split():
        r,ch,b,fs=l.split('|'); off[r]=(ch,b,fs)
files=sorted(p.name for p in d.iterdir() if p.is_file() and not p.name.startswith('.'))
L=[f'# HaldenFunk-MeshCore {ver} für {dev}','',f'Chip: **{e["chip"]}** · Web-Oberfläche: **{"ja" if e["webui"] else "nein – ohne Web-Oberfläche"}** · https://github.com/Pietwars/HaldenFunk-MeshCore','',
'> ⚠️ Kein zertifiziertes Notrufsystem. In Lebensgefahr immer **112** wählen. Diese Firmware ist auf diesem Gerät **nicht auf echter Hardware getestet** (getestet ist nur LILYGO T3 V1.6.1 mit der Schwester-Firmware).','',
'**Antenne anschrauben, bevor das Gerät Strom bekommt.** Funkprofil: MeshCore-Standard EU/UK 869,618 MHz, 62,5 kHz, SF8 – auf allen Geräten gleich einstellen.','']
if e['chip'].startswith('esp32'):
    for r,(ch,b,fs) in off.items():
        L+=[f'## {"WLAN-Portal mit Web-Oberfläche" if r=="wifi" else r}','',
            '1. Chrome/Edge, https://espressif.github.io/esptool-js/ öffnen, **Connect**, Port wählen (ggf. BOOT gedrückt halten und RST drücken).',
            f'2. Flash Address **`0x0`**, Datei `*-{r}-factory.bin`, **Program**, danach RST.',
            f'3. Update ohne Datenverlust: nur `teile-{r}/firmware.bin` an **`0x10000`**.','',
            f'Teile: bootloader.bin {b}, partitions.bin 0x8000, boot_app0.bin 0xe000, firmware.bin 0x10000'+(f', spiffs.bin {fs}' if fs!='-' else '')+'.','']
        if r=='wifi': L+=['**Update von 0.1.0-alpha:** gleiche Partitionstabelle – es reicht `teile-wifi/firmware.bin` an `0x10000`; Einstellungen, Identität, Kontakte, WLAN-Passwort und Bluetooth-PIN bleiben erhalten. Von älteren Versionen (vor 0.1.0-alpha) einmal komplett mit der factory-Datei an `0x0`.'+(' **Klassischer ESP32:** Bluetooth läuft ab 0.1.1-alpha mit NimBLE – die alte Kopplung in den Bluetooth-Einstellungen des Handys entfernen und die MeshCore-App einmal neu koppeln (gleiche PIN).' if ch=='esp32' else ''),'']
        if r=='wifi': L+=['Danach mit dem WLAN `HaldenFunk-XXXXXX` verbinden. Das Passwort steht seriell (115200 Baud) und auf dem Display (falls vorhanden). Im Browser http://192.168.4.1 öffnen.','','Parallel kann sich die offizielle MeshCore-App per **Bluetooth** verbinden (falls das Gerät BLE hat). Die Bluetooth-PIN ist pro Gerät zufällig und steht seriell (`[BLE] … PIN`) und auf dem Display.','']
elif e['chip'] in('nrf52','rp2040'):
    L+=['## Flashen per UF2','','1. Gerät per USB anschließen und **zweimal schnell Reset** drücken (RP2040: BOOTSEL gedrückt halten beim Einstecken). Es erscheint ein USB-Laufwerk.',
        '2. Die passende `.uf2`-Datei auf das Laufwerk kopieren. Das Gerät startet neu.','',
        'nRF52: Die `.zip` ist ein DFU-Paket (z. B. für die Nordic-DFU-App oder `adafruit-nrfutil`).','',
        'Rollen: `companion` = MeshCore-Companion für die MeshCore-App (Bluetooth bzw. USB), `repeater` = Repeater, `room_server` = Room-Server (Schwarzes Brett).','',
        'Konfiguration über die offizielle MeshCore-App bzw. https://config.meshcore.dev / https://meshcore.co.uk/configurator/ – **keine** HaldenFunk-Web-Oberfläche (kein WLAN auf diesem Chip).','']
else:
    L+=['## STM32','','`.bin`/`.hex` mit STM32CubeProgrammer oder einem SWD-Programmer flashen. Ohne Web-Oberfläche.','']
L+=['## Dateien','']+[f'- `{f}`' for f in files]+['','Prüfsummen: `SHA256SUMS`.']
(d/'ANLEITUNG.md').write_text('\n'.join(L)+'\n',encoding='utf-8')
