#!/usr/bin/env python3
"""Prüft den T-Deck-Plus-Standalone-Build (Issue #35): keine WLAN-/Web-Schicht in der gelinkten Firmware.

  python3 tools/check_standalone.py --build-dir .pio/build/LilyGo_TDeck_standalone --build-log build.log \
      --output .pio/build/LilyGo_TDeck_standalone/standalone-report \
      [--compare-build-dir .pio/build/dar_LilyGo_TDeck_wifi --compare-log wifi.log]

Geprüft wird das Ergebnis, nicht die Quelltexte:
  1. firmware.map: aus WLAN-Treiber, IP-Stack und Webserver-Bibliotheken landet kein Byte in einem geladenen
     Abschnitt (Flash/IRAM/DRAM). Erlaubt ist nur der kleine Koexistenz-Adapter aus libesp_wifi.a, den der
     Bluetooth-Controller des ESP32-S3 für das gemeinsame 2,4-GHz-Funkteil braucht (Grenze 2 KiB).
  2. firmware.elf (nm, falls vorhanden): keine WLAN-/Webserver-/Portal-Symbole.
  3. firmware.bin: keine eingebettete Weboberfläche, keine Portal-Texte; Firmwarekennung "-standalone".
  4. ESP32-S3-Abbild und Partitionstabelle wie beim normalen T-Deck-Build (Funktionen aus validate_memory.py,
     Boardvertrag LilyGo_TDeck nur gelesen; der Standalone-Build ist kein Release-Board).
Mit --compare-* wird zusätzlich die Ersparnis gegenüber dem normalen Build berechnet.
"""
import argparse, collections, glob, hashlib, json, os, re, shutil, subprocess, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import validate_memory as vm  # noqa: E402

FORBIDDEN_UNITS = [  # Bibliothek bzw. Objektdatei -> darf 0 geladene Bytes haben
    'libWiFi.a', 'libWiFiClientSecure.a', 'libWebServer.a', 'libDNSServer.a', 'libESPmDNS.a',
    'libHTTPClient.a', 'libHTTPUpdate.a', 'libAsyncTCP.a', 'libESPAsyncWebServer.a',
    'libnet80211.a', 'libpp.a', 'libwpa_supplicant.a', 'liblwip.a', 'libesp_netif.a', 'libsmartconfig.a',
    'libespnow.a', 'libmesh.a', 'WebFrontend.cpp.o', 'ESPNOWRadio.cpp.o', 'SerialWifiInterface.cpp.o',
]
ALLOWED_SMALL = {'libesp_wifi.a': 2048}  # nur Koexistenz-Adapter für Bluetooth
FORBIDDEN_SYMBOLS = re.compile(
    r'WiFiClass|WiFiGenericClass|WiFiSTAClass|WiFiAPClass|WiFiServer|WiFiClient|WebServer|DNSServer|'
    r'AsyncWebServer|DisasterPortal|SerialWifiInterface|ESPNOWRadio|\besp_wifi_init\b|\besp_wifi_start\b|'
    r'\besp_wifi_set_mode\b|\besp_netif_init\b|\besp_now_init\b|\blwip_\w+|\btcpip_adapter\w*')
FORBIDDEN_BYTES = [b'<!DOCTYPE html', b'<!doctype html', b'WLAN offen', b'MeshAhrtal', b'/api/']
LOADED = re.compile(r'^\.(iram0|dram0|flash|rtc|noinit|ext_ram)')


def loaded_bytes(map_text):
    """Bytes pro Bibliothek/Objektdatei in geladenen Ausgabeabschnitten der Linker-Map."""
    body = map_text.split('Linker script and memory map', 1)[-1]
    out, pending = None, None
    size = collections.Counter()
    members = collections.defaultdict(collections.Counter)
    for line in body.split('\n'):
        m = re.match(r'^(\.\S+)', line)
        if m:
            out = m.group(1); pending = None; continue
        m = re.match(r'^ (\.\S+)\s*$', line)
        if m:
            pending = m.group(1); continue
        m = re.match(r'^ (\.\S+)?\s+0x([0-9a-f]+)\s+0x([0-9a-f]+)\s+(\S+)$', line)
        pending = None
        if m and out and LOADED.match(out):
            n = int(m.group(3), 16)
            if not n:
                continue
            path = m.group(4)
            unit = re.sub(r'\(.*', '', path).split('/')[-1]
            size[unit] += n
            mm = re.search(r'\(([^)]+)\)$', path)
            members[unit][mm.group(1) if mm else unit] += n
    return size, members


def find_nm():
    for pat in ('~/.platformio/packages/toolchain-xtensa-esp32s3/bin/xtensa-esp32s3-elf-nm',):
        hits = glob.glob(os.path.expanduser(pat))
        if hits:
            return hits[0]
    return shutil.which('xtensa-esp32s3-elf-nm')


def sizes(log_text):
    clean = re.sub(r'\x1b\[[0-9;]*m', '', log_text)
    r = {}
    for name in ('RAM', 'Flash'):
        m = re.findall(name + r':[^\n]*\(used (\d+) bytes from (\d+) bytes\)', clean)
        vm.require(m, f'build log: missing {name} size report')
        used, total = map(int, m[-1]); r[name.lower()] = dict(used=used, total=total)
    return r


def check(build_dir, log_text):
    problems = []
    size, members = loaded_bytes((build_dir / 'firmware.map').read_text(errors='replace'))
    linked = {u: size[u] for u in FORBIDDEN_UNITS if size.get(u)}
    for u, n in linked.items():
        problems.append(f'{u}: {n} Bytes in der Firmware')
    small = {}
    for u, limit in ALLOWED_SMALL.items():
        small[u] = dict(bytes=size.get(u, 0), members=dict(members.get(u, {})))
        if size.get(u, 0) > limit:
            problems.append(f'{u}: {size[u]} Bytes > {limit} (mehr als Bluetooth-Koexistenz)')
    nm, symbols = find_nm(), None
    if nm:
        out = subprocess.run([nm, '-C', str(build_dir / 'firmware.elf')], capture_output=True, text=True, check=True).stdout
        symbols = sorted({l.split(' ', 2)[-1] for l in out.splitlines() if FORBIDDEN_SYMBOLS.search(l)})
        problems += [f'Symbol: {s}' for s in symbols[:20]]
    fw = (build_dir / 'firmware.bin').read_bytes()
    for b in FORBIDDEN_BYTES:
        if b in fw:
            problems.append(f'firmware.bin enthält {b!r}')
    if b'-standalone' not in fw:
        problems.append('firmware.bin: Firmwarekennung ohne "-standalone"')
    board = json.loads((ROOT / 'tools/release_boards.json').read_text())['LilyGo_TDeck']
    header = vm.image_header(fw, board, 'firmware.bin')
    vm.image_header((build_dir / 'bootloader.bin').read_bytes(), board, 'bootloader.bin')
    parts = vm.partitions((build_dir / 'partitions.bin').read_bytes(), board)
    app = [p for p in parts if p['type'] == 0 and p['offset'] == board['app_offset']]
    vm.require(len(app) == 1, 'missing application partition at expected flash offset')
    vm.require(len(fw) <= app[0]['size'], 'firmware.bin exceeds application partition')
    return problems, dict(memory=sizes(log_text), firmware_bytes=len(fw), app_partition_bytes=app[0]['size'],
                          header=header, partitions=parts, forbidden_linked=linked, allowed_small=small,
                          nm_checked=bool(nm), forbidden_symbols=symbols or [],
                          sha256=hashlib.sha256(fw).hexdigest())


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--build-dir', type=Path, required=True)
    ap.add_argument('--build-log', type=Path, required=True)
    ap.add_argument('--output', type=Path, required=True, help='Berichtspfad ohne Endung')
    ap.add_argument('--compare-build-dir', type=Path)
    ap.add_argument('--compare-log', type=Path)
    a = ap.parse_args(argv)
    try:
        problems, res = check(a.build_dir, a.build_log.read_text(errors='replace'))
    except (ValueError, OSError, KeyError, subprocess.CalledProcessError) as exc:
        print(f'FEHLER: {exc}', file=sys.stderr); return 1
    rep = dict(schema_version=1, env=a.build_dir.name, commit=os.environ.get('GITHUB_SHA', 'local'), result=res)
    md = [f'# Standalone-Prüfung: {a.build_dir.name}', '', f'Commit: `{rep["commit"]}`', '',
          '| Messwert | Bytes |', '|---|---:|',
          f'| Flash laut Linker | {res["memory"]["flash"]["used"]} |',
          f'| Statisches RAM (.data/.bss) laut Linker | {res["memory"]["ram"]["used"]} |',
          f'| firmware.bin | {res["firmware_bytes"]} |',
          f'| Frei in App-Partition | {res["app_partition_bytes"] - res["firmware_bytes"]} |', '']
    if a.compare_build_dir and a.compare_log:
        base = sizes(a.compare_log.read_text(errors='replace'))
        bfw = (a.compare_build_dir / 'firmware.bin').stat().st_size
        bfs = a.compare_build_dir / 'spiffs.bin'
        cmp = dict(env=a.compare_build_dir.name, flash_used=base['flash']['used'], ram_used=base['ram']['used'],
                   firmware_bytes=bfw,
                   saved_flash=base['flash']['used'] - res['memory']['flash']['used'],
                   saved_ram=base['ram']['used'] - res['memory']['ram']['used'],
                   saved_firmware_bytes=bfw - res['firmware_bytes'])
        rep['compare'] = cmp
        md += [f'## Vergleich mit `{cmp["env"]}`', '', '| Messwert | normal | standalone | Ersparnis |', '|---|---:|---:|---:|',
               f'| Flash laut Linker | {cmp["flash_used"]} | {res["memory"]["flash"]["used"]} | {cmp["saved_flash"]} |',
               f'| Statisches RAM | {cmp["ram_used"]} | {res["memory"]["ram"]["used"]} | {cmp["saved_ram"]} |',
               f'| firmware.bin | {bfw} | {res["firmware_bytes"]} | {cmp["saved_firmware_bytes"]} |', '']
    md += ['## WLAN/Web', '', 'Verbotene Bibliotheken/Objekte mit geladenen Bytes: ' +
           (', '.join(f'{k} ({v})' for k, v in res['forbidden_linked'].items()) or 'keine'),
           f'WLAN-/Web-Symbole (nm): {"nicht geprüft (nm fehlt)" if not res["nm_checked"] else (len(res["forbidden_symbols"]) or "keine")}',
           'Erlaubter Rest: ' + ', '.join(f'{k} {v["bytes"]} B {sorted(v["members"])}' for k, v in res['allowed_small'].items()),
           '', 'Ergebnis: ' + ('OK' if not problems else 'FEHLER'), '']
    md += [f'- {p}' for p in problems]
    a.output.parent.mkdir(parents=True, exist_ok=True)
    a.output.with_suffix('.json').write_text(json.dumps(rep, indent=2) + '\n')
    a.output.with_suffix('.md').write_text('\n'.join(md) + '\n')
    if problems:
        print('FEHLER: Standalone-Build enthält WLAN/Web:', *problems, sep='\n  ', file=sys.stderr); return 1
    print(f'{a.build_dir.name}: keine WLAN-/Web-Schicht gelinkt; Bericht {a.output}.md')
    return 0


if __name__ == '__main__':
    sys.exit(main())
