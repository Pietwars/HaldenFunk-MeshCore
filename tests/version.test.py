#!/usr/bin/env python3
"""Prüft tools/version.py: Stand im Repo muss passen, eine abweichende VERSION muss erkannt werden."""
import shutil, subprocess, sys, tempfile
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
ok = subprocess.run([sys.executable, str(ROOT / 'tools/version.py'), '--check'])
assert ok.returncode == 0, 'Versionen im Repository stimmen nicht überein'
with tempfile.TemporaryDirectory() as d:
    tmp = Path(d) / 'repo'
    shutil.copytree(ROOT, tmp, ignore=shutil.ignore_patterns('.git', '.pio', 'node_modules', 'dist', '_site', 'test-results', '*.bin', 'docs'))
    shutil.copytree(ROOT / 'docs', tmp / 'docs', ignore=shutil.ignore_patterns('images', '*.pdf', '*.jpg', '*.png', 'demo'), dirs_exist_ok=True)
    v = (tmp / 'VERSION').read_text(encoding='utf-8')
    (tmp / 'VERSION').write_text(v.replace('VERSION=', 'VERSION=9.9.9-rc1\n#', 1), encoding='utf-8')
    bad = subprocess.run([sys.executable, str(tmp / 'tools/version.py'), '--check'], capture_output=True, text=True)
    assert bad.returncode != 0 and 'ABWEICHUNG' in bad.stdout, 'Abweichende Version wurde nicht erkannt'
    subprocess.run([sys.executable, str(tmp / 'tools/version.py')], check=True, capture_output=True)
    again = subprocess.run([sys.executable, str(tmp / 'tools/version.py'), '--check'], capture_output=True, text=True)
    assert again.returncode == 0, again.stdout
print('version.test.py: ok')
