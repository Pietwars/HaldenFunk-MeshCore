#!/usr/bin/env python3
"""Flyer-Generator (A5) für die Projektfamilie DisAHRsterRadio / HaldenFunk.

Erzeugt aus tools/flyer/flyer.json (+ Datei VERSION bzw. [hf] version in platformio.local.ini) zwei PDFs pro Repository:
  docs/flyer/<name>-Flyer.pdf          Druck: QR-Codes zu Projekt/Release und Demo
  docs/flyer/<name>-Flyer-digital.pdf  digital: alle Links und QR-Codes anklickbar
  docs/flyer/<name>-Flyer-vorschau.jpg Vorschau (über pdftoppm, falls vorhanden)

Aufruf (im Repository):  python3 tools/flyer/flyer.py [--json tools/flyer/flyer.json] [--out docs/flyer]
Benötigt: Python-Paket qrcode (pip install qrcode pillow) und Chrome/Chromium (headless),
          gefunden über $CHROME, google-chrome, chromium, chromium-browser oder Playwright-Browser.
Platzhalter in flyer.json: @VERSION@ (aus der Datei VERSION), @RELEASE@ (Release-URL des Tags v@VERSION@).
Optional "keep_html": Pfad, unter dem die Druck-HTML im Repository bleibt (z. B. docs/flyer/flyer.html).
Optional "theme" ({"#b9f48d": "#e8955a", …} ersetzt Farben des Standard-Looks) und "extra_css".
Optional "credit": Fußzeile links (Standard: © Pietwars · Bild © Pietwars · Kartendaten © OpenStreetMap).
"""
import argparse, glob, html, json, os, shutil, subprocess, sys, tempfile
from pathlib import Path
import qrcode

ROOT = Path.cwd()
CSS = r"""@page{size:A5;margin:0}
*{box-sizing:border-box}
html,body{margin:0;background:#07130f}
body{font-family:"Noto Sans","DejaVu Sans",system-ui,sans-serif;color:#e8f0e4;-webkit-print-color-adjust:exact;print-color-adjust:exact}
a{color:inherit}
.page{width:148mm;height:210mm;padding:8mm 10mm 7mm;display:flex;flex-direction:column;gap:2.4mm;background:radial-gradient(ellipse at 85% 0,#1d3a2a 0,transparent 55%),#07130f;overflow:hidden}
.hero{border-radius:4mm;overflow:hidden;border:0.3mm solid #2f5a3c;height:37mm;flex:none;position:relative;background:#07130f}
.hero .bg{position:absolute;inset:-4mm;background:var(--h) center/cover;filter:blur(3mm) brightness(.45)}
.hero img{position:relative;display:block;height:100%;margin:0 auto}
h1{font-size:22pt;line-height:1;margin:1mm 0 0;letter-spacing:-.4pt;color:#f3f8ee}
h1 span{color:#b9f48d}
.sub{font-size:10.5pt;color:#b9f48d;margin:0;font-weight:600}
p{margin:0;font-size:8.7pt;line-height:1.38}
h2{font-size:10.5pt;margin:0 0 1.6mm;color:#b9f48d;text-transform:uppercase;letter-spacing:.6pt}
.feat{display:grid;grid-template-columns:1fr 1fr;gap:1mm 4mm;margin:0;padding-left:4mm;font-size:8.4pt;line-height:1.35}
.steps{display:grid;grid-template-columns:repeat(3,1fr);gap:2.6mm}
.step{background:#10261b;border:0.3mm solid #2a4d36;border-radius:3mm;padding:2mm}
.step>b{display:block;font-size:15pt;color:#b9f48d;line-height:1}
.step p{font-size:7.9pt;line-height:1.35;margin-top:1mm}
.join{display:grid;grid-template-columns:1fr 24mm 24mm;gap:4mm;align-items:center;background:#10261b;border:0.3mm solid #2a4d36;border-radius:3mm;padding:2.4mm 3mm}
.qr{background:#fff;border-radius:2mm;padding:1.2mm;text-align:center}
.qr img{width:100%;display:block}
.qr small{display:block;color:#10261c;font-size:6.2pt;font-weight:700;margin-top:.6mm}
.url{font-size:8.6pt;font-weight:700;color:#f3f8ee;margin-top:1.6mm}
.url2{font-size:7.6pt;color:#cfe3d2;margin-top:.6mm}
.links{font-size:7.6pt;color:#cfe3d2;margin-top:1.2mm}.links a{color:#b9f48d;font-weight:700}
.warn{border-left:1mm solid #e07a3d;background:#2a1a12;border-radius:2mm;padding:2.2mm 3mm;font-size:8.2pt;line-height:1.4}
.warn b{color:#ffb38a}
.top{display:grid;grid-template-columns:1fr 25mm;gap:4mm;align-items:start}.top>div{display:flex;flex-direction:column;gap:2.4mm}
.phone{background:#0b0f0d;border:0.9mm solid #3a5a45;border-radius:4.5mm;padding:1.2mm;box-shadow:0 0 0 0.3mm #1c3526}
.phone img{display:block;width:100%;height:46mm;object-fit:cover;object-position:top;border-radius:3mm}
.top .feat{gap:.6mm 3mm;font-size:8pt}.top p{font-size:8.4pt}.phone small{display:block;text-align:center;font-size:6.2pt;color:#9fb8a5;margin-top:.8mm}
footer{margin-top:auto;display:flex;justify-content:space-between;font-size:7pt;color:#9fb8a5}"""

def chrome():
    c = [os.environ.get('CHROME')] + [shutil.which(n) for n in ('google-chrome', 'google-chrome-stable', 'chromium', 'chromium-browser')]
    pw = os.environ.get('PLAYWRIGHT_BROWSERS_PATH', str(Path.home() / '.cache/ms-playwright'))
    c += sorted(glob.glob(pw + '/chromium_headless_shell-*/chrome-*/chrome-headless-shell')) + sorted(glob.glob(pw + '/chromium-*/chrome-linux*/chrome'))
    for x in c:
        if x and os.path.exists(x): return x
    sys.exit('Kein Chrome/Chromium gefunden (CHROME=… setzen)')

def qr(data, path, level):
    q = qrcode.QRCode(error_correction=level, border=2, box_size=12); q.add_data(data); q.make(fit=True)
    q.make_image(fill_color='black', back_color='white').save(path)

def css(f):  # optionales Farbschema: "theme" {alte Farbe: neue Farbe}, "extra_css" für Zusatzregeln
    c = CSS
    for k, v in f.get('theme', {}).items(): c = c.replace(k, v)
    return c + f.get('extra_css', '')

def short(u): return u.replace('https://', '').rstrip('/')

def page(f, digital):
    a = (lambda u, t: f'<a href="{u}">{t}</a>') if digital else (lambda u, t: t)
    feats = ''.join(f'<li>{x}</li>' for x in f['features'])
    steps = ''.join(f'<div class="step"><b>{i}</b><p>{s}</p></div>' for i, s in enumerate(f['steps'], 1))
    urls = f'<div class="url">{a(f["repo"], short(f["repo"]))}</div>' + ''.join(
        f'<div class="url2"><b>{k}:</b> {a(u, short(u))}</div>' for k, u in f['extra_urls'])
    if digital:
        urls += '<div class="links">Links: ' + ' · '.join(f'<a href="{u}">{k}</a>' for k, u in f['links']) + '</div>'
    top = f'<div><h1>{f["title_html"]}</h1><p class="sub">{f["subtitle"]}</p></div><p>{f["intro"]}</p><div><h2>Funktionen</h2><ul class="feat">{feats}</ul></div>'
    if f.get('gui'):
        cap = f.get('gui_caption', 'Weboberfläche')
        img = '<img src="gui.jpg" alt="' + html.escape(cap) + '">'
        top = f'<div class="top"><div>{top}</div><div class="phone">{a(f["demo"], img)}<small>{cap}</small></div></div>'
    q1 = f'<img src="qr-projekt.png" alt="QR {html.escape(f["qr_label"])}">'
    q2 = '<img src="qr-demo.png" alt="QR Demo">'
    return f"""<!doctype html><html lang="de"><meta charset="utf-8"><title>{f['name']} · Flyer</title><style>{css(f)}</style><div class="page">
<div class="hero"><div class="bg" style="--h:url(hero.jpg)"></div><img src="hero.jpg" alt="{f['name']}"></div>
{top}
<div><h2>So startest du</h2><div class="steps">{steps}</div></div>
<div class="join"><div><h2>Projekt &amp; Download</h2><p style="font-size:8.6pt">Code, Anleitung, Fehler melden und mitmachen:</p>{urls}</div><div class="qr">{a(f['qr_url'], q1)}<small>{f['qr_label']}</small></div><div class="qr">{a(f['demo'], q2)}<small>Demo</small></div></div>
<div class="warn"><b>Prototyp, kein zertifiziertes Notrufsystem.</b> Gemeinschaftsprojekt im Aufbau, ohne Gewähr. <b>Im Notfall immer 112.</b></div>
<footer><span>{f.get('credit', '© Pietwars · Bild © Pietwars · Kartendaten © OpenStreetMap (ODbL)')}</span><span>Stand {f['version']}</span></footer></div></html>"""

def main():
    ap = argparse.ArgumentParser(); ap.add_argument('--json', default='tools/flyer/flyer.json'); ap.add_argument('--out', default='docs/flyer')
    o = ap.parse_args()
    if (ROOT / 'VERSION').exists():
        version = dict(l.split('=', 1) for l in (ROOT / 'VERSION').read_text().split() if '=' in l)['VERSION']
    else:  # HaldenFunk-MeshCore: Version aus [hf] version in platformio.local.ini
        import configparser; c = configparser.ConfigParser(); c.read(ROOT / 'platformio.local.ini'); version = c['hf']['version'].strip()
    raw = Path(o.json).read_text(encoding='utf-8').replace('@VERSION@', version)
    f = json.loads(raw); f['version'] = version
    f = json.loads(json.dumps(f).replace('@RELEASE@', f'{f["repo"]}/releases/tag/v{version}'))
    out = Path(o.out); out.mkdir(parents=True, exist_ok=True)
    tmp = Path(tempfile.mkdtemp(prefix='flyer-'))
    shutil.copy(ROOT / f['hero'], tmp / 'hero.jpg')
    if f.get('gui'): shutil.copy(ROOT / f['gui'], tmp / 'gui.jpg')
    qr(f['qr_url'], tmp / 'qr-projekt.png', qrcode.constants.ERROR_CORRECT_H)
    qr(f['demo'], tmp / 'qr-demo.png', qrcode.constants.ERROR_CORRECT_M)
    exe = chrome()
    for digital in (False, True):
        name = f['name'] + '-Flyer' + ('-digital' if digital else '')
        (tmp / (name + '.html')).write_text(page(f, digital), encoding='utf-8')
        pdf = out / (name + '.pdf')
        subprocess.run([exe, '--headless', '--no-sandbox', '--disable-gpu', '--no-pdf-header-footer', '--print-to-pdf-no-header',
                        f'--print-to-pdf={pdf.resolve()}', (tmp / (name + '.html')).as_uri()], check=True, capture_output=True)
        print('geschrieben:', pdf)
    for p in (out / f'{f["name"]}-Flyer-qr.png', out / f'{f["name"]}-Flyer-demo-qr.png'):
        shutil.copy(tmp / ('qr-projekt.png' if 'demo' not in p.name else 'qr-demo.png'), p)
    if f.get('keep_html'):  # Druck-HTML im Repo behalten (Bildpfade relativ zu docs/flyer)
        h = page(f, False).replace('hero.jpg', Path(f['hero']).name).replace('gui.jpg', '../images/' + Path(f.get('gui', 'gui.jpg')).name).replace('qr-projekt.png', f'{f["name"]}-Flyer-qr.png').replace('qr-demo.png', f'{f["name"]}-Flyer-demo-qr.png')
        dst = ROOT / f['keep_html']; dst.write_text(h, encoding='utf-8')
        if not (dst.parent / Path(f['hero']).name).exists(): shutil.copy(ROOT / f['hero'], dst.parent / Path(f['hero']).name)
        print('geschrieben:', dst)
    if shutil.which('pdftoppm'):
        subprocess.run(['pdftoppm', '-jpeg', '-r', '110', '-singlefile', str(out / f'{f["name"]}-Flyer.pdf'), str(out / f'{f["name"]}-Flyer-vorschau')], check=True)
        print('geschrieben:', out / f'{f["name"]}-Flyer-vorschau.jpg')
    shutil.rmtree(tmp)

if __name__ == '__main__':
    main()
