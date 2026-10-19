#!/usr/bin/env python3
"""Prueft Release-Pakete HaldenFunk-MeshCore-<Version>-<Geraet>.zip auf Vollstaendigkeit.
Aufruf: tools/verify_package.py <zip>... | <Ordner>   (Exit 1 bei Fehlern)
ESP32 (ab 0.9.2, wie DisAHRsterRadio-MeshCore): eine Rolle -> <Paket>-factory.bin, <Paket>-firmware.bin (Update an 0x10000)
       + teile/{bootloader,partitions,boot_app0}.bin; mehrere Rollen -> <Paket>-<Rolle>-factory.bin, <Paket>-<Rolle>-firmware.bin
       + teile-<Rolle>/...; WLAN-Portal zusaetzlich spiffs.bin in den Teilen; nRF52/RP2040: .uf2 je Rolle; STM32: .bin oder .hex.
Immer: ANLEITUNG.md (deutsch) und SHA256SUMS, deren Pruefsummen stimmen muessen."""
import hashlib, json, os, sys, zipfile
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent))
from version import version

VER = os.environ.get("HF_VERSION") or version()
DEVS = {d['device']: d for d in json.load(open(Path(__file__).parent / 'devices.json'))}


def check(zp):
    err = []
    name = zp.name[:-4]
    prefix = f'HaldenFunk-MeshCore-{VER}-'
    if not name.startswith(prefix):
        return [f'falscher Name/Version (erwartet {prefix}<Geraet>.zip)']
    dev = name[len(prefix):]
    if dev not in DEVS:
        return [f'unbekanntes Geraet {dev}']
    e = DEVS[dev]
    with zipfile.ZipFile(zp) as z:
        files = {i.filename[len(name) + 1:]: i for i in z.infolist() if not i.is_dir() and i.filename.startswith(name + '/')}
        def need(f, minsize=1):
            if f not in files: err.append(f'fehlt: {f}')
            elif files[f].file_size < minsize: err.append(f'zu klein: {f} ({files[f].file_size} B)')
        need('ANLEITUNG.md'); need('SHA256SUMS')
        if 'ANLEITUNG.md' in files:
            t = z.read(f'{name}/ANLEITUNG.md').decode('utf-8')
            if VER not in t or 'Antenne' not in t or '112' not in t: err.append('ANLEITUNG.md unvollstaendig (Version/Antenne/112)')
        single = len(e['envs']) == 1
        for role in e['envs']:
            if e['chip'].startswith('esp32'):
                suf, teile = ('', 'teile') if single else (f'-{role}', f'teile-{role}')
                need(f'{name}{suf}-factory.bin', 100_000)
                need(f'{name}{suf}-firmware.bin', 100_000)
                for p in ('bootloader.bin', 'partitions.bin', 'boot_app0.bin'): need(f'{teile}/{p}')
                if role == 'wifi': need(f'{teile}/spiffs.bin', 100_000)
            elif e['chip'] in ('nrf52', 'rp2040'):
                need(f'{name}-{role}.uf2', 10_000)
            else:
                if f'{name}-{role}.bin' not in files and f'{name}-{role}.hex' not in files: err.append(f'fehlt: {role}.bin/.hex')
        if 'SHA256SUMS' in files:
            listed = set()
            for ln in z.read(f'{name}/SHA256SUMS').decode().splitlines():
                h, f = ln.split(None, 1); f = f.lstrip('*').removeprefix('./'); listed.add(f)
                if f not in files: err.append(f'SHA256SUMS nennt fehlende Datei {f}')
                elif hashlib.sha256(z.read(f'{name}/{f}')).hexdigest() != h: err.append(f'Pruefsumme falsch: {f}')
            for f in files:
                if f != 'SHA256SUMS' and f not in listed: err.append(f'nicht in SHA256SUMS: {f}')
    return err


args = [Path(a) for a in sys.argv[1:]]
zips = sorted(z for a in args for z in (a.glob('*.zip') if a.is_dir() else [a]))
bad = 0
for zp in zips:
    err = check(zp)
    if err:
        bad += 1; print(f'FEHLER {zp.name}: ' + '; '.join(err))
seen = {z.name for z in zips}
if any(a.is_dir() for a in args):
    for d in DEVS:
        if f'HaldenFunk-MeshCore-{VER}-{d}.zip' not in seen:
            bad += 1; print(f'FEHLT Paket fuer {d}')
print(f'{len(zips)} Pakete geprueft, {bad} fehlerhaft/fehlend (Version {VER}, {len(DEVS)} Geraete)')
sys.exit(1 if bad else 0)
