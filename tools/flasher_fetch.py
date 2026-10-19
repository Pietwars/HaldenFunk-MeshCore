#!/usr/bin/env python3
"""Web-Flasher (GitHub Pages): holt die Firmware aus dem GitHub-Release, prüft SHA-256 und schreibt die Manifeste.

Aufruf:  python3 tools/flasher_fetch.py <Pages-Ordner>/flasher        (nur im Pages-Workflow, braucht Internet)
         python3 tools/flasher_fetch.py --check                       (offline: Konfiguration prüfen)
         python3 tools/flasher_fetch.py --cfg tools/flasher-next --check   (vorbereitete Konfiguration prüfen)
Konfiguration: <Ordner>/flasher.json + <Ordner>/index.html (Standard docs/flasher). Formate:
  alt:   ein Gerät  {repo, tag, asset, member, sha256, out, manifest}             -> manifest.json
  v2:    {repo, tag, devices:[{id, label, asset, member, sha256, out, manifest}]}  -> manifest-<id>.json
  v3:    {repo, tag, devices:[{id, label, asset, files:{neu, update}, modes:{neu, update}}]}
         files.<art> = {member, sha256, out}; modes.<art> = ESP-Web-Tools-Manifest -> manifest-<id>-<art>.json
         neu    = Neuinstallation: <Name>-factory.bin an 0x0, Flash wird immer gelöscht
         update = Update: nur <Name>-firmware.bin an 0x10000, ohne Löschen (Identität, Kontakte,
                  Einstellungen im spiffs-Bereich und nvs bleiben erhalten)
"""
import hashlib, io, json, re, sys, urllib.request, zipfile
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
args = sys.argv[1:]
CFGDIR = ROOT / 'docs/flasher'
if '--cfg' in args:
    i = args.index('--cfg'); CFGDIR = (ROOT / args[i + 1]).resolve(); del args[i:i + 2]
CFG = json.loads((CFGDIR / 'flasher.json').read_text(encoding='utf-8'))

def jobs():
    """je Manifest: (Gerät, Manifestdatei, Manifest, [Dateien])"""
    if 'devices' not in CFG:
        return [(dict(CFG), 'manifest.json', CFG['manifest'], [CFG], None)]
    out = []
    for d in CFG['devices']:
        d = dict(d, repo=CFG['repo'], tag=CFG['tag'])
        if 'modes' in d:
            for art, m in d['modes'].items():
                out.append((d, f"manifest-{d['id']}-{art}.json", m, [d['files'][art]], art))
        else:
            out.append((d, f"manifest-{d['id']}.json", d['manifest'], [d], None))
    return out

def check():
    html = (CFGDIR / 'index.html').read_text(encoding='utf-8')
    assert 'FLASHEN.md' in html, 'Link auf FLASHEN.md fehlt'
    outs = {}
    for d, mfile, m, files, art in jobs():
        parts = [p for b in m['builds'] for p in b['parts']]
        assert re.fullmatch(r'v\d+\.\d+\.\d+', d['tag']), 'tag'
        assert m['version'] == d['tag'][1:], 'Manifest-Version passt nicht zum Tag'
        assert [b['chipFamily'] for b in m['builds']] == ['ESP32-S3'], 'nur ESP32-S3-Geräte'
        for f in files:
            assert re.fullmatch(r'[0-9a-f]{64}', f['sha256']), 'sha256'
            assert outs.setdefault(f['out'], f['sha256']) == f['sha256'], f'Dateiname doppelt: {f["out"]}'
            assert f['sha256'] in html, f'SHA-256 von {f["out"]} fehlt in index.html'
        if art in (None, 'neu'):
            assert parts == [{'path': files[0]['out'], 'offset': 0}], 'Neuinstallation: factory.bin muss an 0x0'
            assert files[0].get('member', '').endswith('-factory.bin') or art is None, 'Neuinstallation braucht factory.bin'
            if art == 'neu':
                assert m.get('new_install_prompt_erase') is False, 'Neuinstallation muss immer löschen (prompt_erase false)'
        else:
            assert art == 'update', f'unbekannte Art {art}'
            assert parts == [{'path': files[0]['out'], 'offset': 0x10000}], 'Update: nur firmware.bin an 0x10000'
            assert files[0]['member'].endswith('-firmware.bin'), 'Update braucht firmware.bin'
            assert m.get('new_install_prompt_erase') is True, 'Update: Löschen nur auf Nachfrage (prompt_erase true)'
            assert 'spiffs' not in json.dumps(m), 'Update darf spiffs (Identität, Kontakte) nicht schreiben'
        if art: assert m.get('new_install_improv_wait_time') == 0, 'Firmware ohne Improv: new_install_improv_wait_time 0'
        assert f'manifest="{mfile}"' in html, f'index.html verweist nicht auf {mfile}'
        print('Flasher-Konfiguration OK:', d['tag'], mfile, ', '.join(f.get('member', d['asset']).split('/')[-1] for f in files))

def fetch(out):
    out = Path(out); out.mkdir(parents=True, exist_ok=True)
    cache = {}
    for d, mfile, m, files, art in jobs():
        url = f"https://github.com/{d['repo']}/releases/download/{d['tag']}/{d['asset']}"
        if url not in cache: cache[url] = urllib.request.urlopen(url, timeout=120).read()
        for f in files:
            data = cache[url]
            if f.get('member'):
                data = zipfile.ZipFile(io.BytesIO(data)).read(f['member'])
            got = hashlib.sha256(data).hexdigest()
            if got != f['sha256']: sys.exit(f'SHA-256 stimmt nicht ({d["asset"]} {f.get("member", "")}): {got} != {f["sha256"]}')
            (out / f['out']).write_bytes(data)
            print('Flasher-Firmware:', out / f['out'], len(data), 'Bytes, SHA-256 ok')
        (out / mfile).write_text(json.dumps(m, ensure_ascii=False, indent=1) + '\n', encoding='utf-8')
    (out / 'flasher.json').unlink(missing_ok=True)

if __name__ == '__main__':
    check()
    if args and args[0] != '--check': fetch(args[0])
