#!/usr/bin/env python3
"""Web-Flasher (GitHub Pages): holt die Firmware aus dem GitHub-Release, prüft SHA-256 und schreibt manifest.json.

Aufruf:  python3 tools/flasher_fetch.py <Pages-Ordner>/flasher   (nur im Pages-Workflow, braucht Internet)
         python3 tools/flasher_fetch.py --check                  (offline: Konfiguration prüfen)
Konfiguration: docs/flasher/flasher.json (Release-Tag, Asset, ggf. Datei im ZIP, SHA-256, ESP-Web-Tools-Manifest).
"""
import hashlib, io, json, re, sys, urllib.request, zipfile
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
CFG = json.loads((ROOT / 'docs/flasher/flasher.json').read_text(encoding='utf-8'))

def check():
    m = CFG['manifest']; parts = [p for b in m['builds'] for p in b['parts']]
    assert re.fullmatch(r'v\d+\.\d+\.\d+', CFG['tag']), 'tag'
    assert re.fullmatch(r'[0-9a-f]{64}', CFG['sha256']), 'sha256'
    assert m['version'] == CFG['tag'][1:], 'Manifest-Version passt nicht zum Tag'
    assert [b['chipFamily'] for b in m['builds']] == ['ESP32-S3'], 'nur T-Deck Plus (ESP32-S3)'
    assert parts == [{'path': CFG['out'], 'offset': 0}], 'factory.bin muss an 0x0'
    html = (ROOT / 'docs/flasher/index.html').read_text(encoding='utf-8')
    assert 'manifest="manifest.json"' in html and CFG['sha256'] in html and 'Heltec V2' in html
    print('Flasher-Konfiguration OK:', CFG['tag'], CFG['asset'])

def fetch(out):
    url = f"https://github.com/{CFG['repo']}/releases/download/{CFG['tag']}/{CFG['asset']}"
    data = urllib.request.urlopen(url, timeout=120).read()
    if CFG.get('member'):
        data = zipfile.ZipFile(io.BytesIO(data)).read(CFG['member'])
    got = hashlib.sha256(data).hexdigest()
    if got != CFG['sha256']: sys.exit(f'SHA-256 stimmt nicht: {got} != {CFG["sha256"]}')
    out = Path(out); out.mkdir(parents=True, exist_ok=True)
    (out / CFG['out']).write_bytes(data)
    (out / 'manifest.json').write_text(json.dumps(CFG['manifest'], ensure_ascii=False, indent=1) + '\n', encoding='utf-8')
    (out / 'flasher.json').unlink(missing_ok=True)
    print('Flasher-Firmware:', out / CFG['out'], len(data), 'Bytes, SHA-256 ok')

if __name__ == '__main__':
    check()
    if sys.argv[1:] and sys.argv[1] != '--check': fetch(sys.argv[1])
