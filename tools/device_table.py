#!/usr/bin/env python3
"""Markdown-Geraetetabelle. Aufruf: device_table.py [Ordner mit gebauten Zips]"""
import json,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).parent))
from version import version
V=version()
devs=json.load(open(Path(__file__).parent/'devices.json'))
built=None
if len(sys.argv)>1: built={p.name for p in Path(sys.argv[1]).glob('*.zip')}
ROLE={'wifi':'WLAN-Portal','companion':'Companion','repeater':'Repeater','room_server':'Room-Server'}
L=['| Gerät | Chip | Web-Oberfläche | Inhalt | Datei |','|---|---|---|---|---|']
for e in devs:
    z=f'HaldenFunk-MeshCore-{V}-{e["device"]}.zip'
    f=f'`{z}`' if built is None or z in built else '❌ Build fehlgeschlagen'
    L.append(f'| {e["device"]} | {e["chip"]} | {"ja" if e["webui"] else "nein (ohne Web-Oberfläche"+(": "+e["note"] if e.get("note") else "")+")"} | {", ".join(ROLE[k] for k in e["envs"])} | {f} |')
print('\n'.join(L))
