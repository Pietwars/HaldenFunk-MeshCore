#!/usr/bin/env python3
"""Prueft, dass alle Versionsangaben zur einzigen Quelle ([hf] version in platformio.local.ini) passen."""
import re, sys
from pathlib import Path
sys.path.insert(0, str(Path(__file__).parent))
from version import version, ROOT

V = version()
err = []
for ini in ('platformio.local.ini', 'platformio.devices.ini'):
    for n, ln in enumerate((ROOT / ini).read_text(encoding='utf-8').splitlines(), 1):
        if 'FIRMWARE_VERSION' in ln and '-hf' in ln and '${hf.version}' not in ln:
            err.append(f'{ini}:{n}: feste Version statt ${{hf.version}}')
def need(path, text):
    if text not in (ROOT / path).read_text(encoding='utf-8'):
        err.append(f'{path}: "{text}" fehlt')
need('README.md', f'**Version {V}**')
need('README.md', f'v1.18.0-hf{V}')
need('CHANGELOG.md', f'## {V}')
need('web/index.html', f"firmware:'v1.18.0-hf{V}'")
if re.search(r'Neu in \d+\.\d+', (ROOT / 'tools/ANLEITUNG.template.md').read_text(encoding='utf-8')):
    err.append('tools/ANLEITUNG.template.md: feste Version statt @VERSION@')
for e in err:
    print('FEHLER', e)
print(f'Version {V}:', 'ok' if not err else f'{len(err)} Fehler')
sys.exit(1 if err else 0)
