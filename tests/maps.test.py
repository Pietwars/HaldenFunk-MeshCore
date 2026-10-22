#!/usr/bin/env python3
"""Prüft die Offline-Karten in web/static/map: lesbar, mit Grenzen (bbox/view), Orten und POIs, im Größenbudget."""
import gzip, json, sys
from pathlib import Path
MAP = Path(__file__).resolve().parent.parent / "web" / "map"
BUDGET = 260 * 1024  # zusammen, gzip
fehler, summe = [], 0
for f in sorted(MAP.glob('*.json.gz')):
    summe += f.stat().st_size
    d = json.loads(gzip.decompress(f.read_bytes()))
    for k in ('name', 'attr', 'bbox', 'view', 'l', 'p'):
        if not d.get(k): fehler.append(f'{f.name}: Feld {k} fehlt oder leer')
    if d.get('bbox') and len(d['bbox']) != 4: fehler.append(f'{f.name}: bbox hat nicht 4 Werte')
    if not any(p[1] for p in d.get('p', [])): fehler.append(f'{f.name}: keine benannten Orte/POIs')
    print(f'{f.name}: {f.stat().st_size} Bytes, {len(d.get("l", []))} Linien, {len(d.get("p", []))} Punkte')
if summe > BUDGET: fehler.append(f'zusammen {summe} Bytes > Budget {BUDGET}')
print(f'zusammen {summe} Bytes')
if fehler: print('FEHLER:\n  ' + '\n  '.join(fehler)); sys.exit(1)
print('Karten OK')
