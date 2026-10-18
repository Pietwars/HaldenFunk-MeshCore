#!/usr/bin/env python3
"""Baut die Live-Demo (GitHub Pages): Weboberflaeche im Demo-Modus, Karten entpackt, Hinweisbanner."""
import gzip,shutil,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from version import version
VER=version()
SRC=ROOT/'web'; OUT=Path(sys.argv[1] if len(sys.argv)>1 else ROOT/'_site')
if OUT.exists(): shutil.rmtree(OUT)
shutil.copytree(SRC,OUT)
for gz in (OUT/'map').glob('*.json.gz'):
    (OUT/'map'/gz.name[:-3]).write_bytes(gzip.decompress(gz.read_bytes()))
html=(OUT/'index.html').read_text(encoding='utf-8')
force='<script>if(!/demo/.test(location.search))location.replace("?demo=1"+location.hash)</script>'
banner=('<div style="background:#f0d5a0;color:#3b2a1f;text-align:center;'
        'font:600 13px/16px system-ui,sans-serif;padding:6px 8px;height:28px;box-sizing:border-box;white-space:nowrap;overflow:hidden">Demo '+VER+' – simulierte Daten · '
        '<a style="color:#3b2a1f" href="https://github.com/Pietwars/HaldenFunk-MeshCore">Quellcode &amp; Firmware</a> · '
        '<a style="color:#3b2a1f" href="flasher/">Flasher</a> · '
        '<a style="color:#3b2a1f" href="flyer/HaldenFunk-MeshCore-Flyer-digital.pdf">Flyer</a></div>')
html=html.replace('<head>','<head>'+force,1)
i=html.index('<body'); j=html.index('>',i)+1
html=html[:j]+banner+html[j:]
(OUT/'index.html').write_text(html,encoding='utf-8')
if (ROOT/'docs/flyer').is_dir(): shutil.copytree(ROOT/'docs/flyer',OUT/'flyer')
if (ROOT/'docs/flasher').is_dir(): shutil.copytree(ROOT/'docs/flasher',OUT/'flasher')  # Firmware holt tools/flasher_fetch.py im Pages-Workflow
(OUT/'.nojekyll').write_text('')
print('Demo gebaut:',OUT)
