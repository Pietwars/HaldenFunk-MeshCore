#!/usr/bin/env python3
"""Schreibt flasher.json und index.html des vorbereiteten Web-Flashers neu (SHA-256 aus den getesteten Paketen).

Aufruf: python3 tools/flasher-next/aktualisieren.py <Paket-T-Deck.zip> <Paket-T-Watch.zip>
Danach: python3 tools/flasher_fetch.py --cfg tools/flasher-next --check
"""
import hashlib, json, re, sys, zipfile
from pathlib import Path
HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
REPO = 'Pietwars/HaldenFunk-MeshCore'
GERAETE = [('tdeck', 'LILYGO T-Deck Plus', 'LilyGo_TDeck'), ('twatch', 'LILYGO T-Watch S3', 'LilyGo_TWatchS3')]

def main(zips):
    devs = []
    for (gid, label, dev), z in zip(GERAETE, zips):
        name = Path(z).stem
        m = re.fullmatch(r'HaldenFunk-MeshCore-(\d+\.\d+\.\d+)-' + dev, name)
        assert m, f'{z}: Paketname passt nicht zu {dev}'
        ver = m.group(1)
        zf = zipfile.ZipFile(z)
        sums = dict(reversed(l.split(None, 1)) for l in zf.read(f'{name}/SHA256SUMS').decode().splitlines() if l.strip())
        files = {}
        for art, suf, out in (('neu', 'factory', f'{gid}-neu-factory.bin'), ('update', 'firmware', f'{gid}-update-firmware.bin')):
            member = f'{name}/{name}-{suf}.bin'
            sha = hashlib.sha256(zf.read(member)).hexdigest()
            assert sums[f'./{name}-{suf}.bin'] == sha, f'{member}: SHA256SUMS im Paket passt nicht'
            files[art] = {'member': member, 'sha256': sha, 'out': out}
        def manifest(art, part, erase):
            return {'name': f'HaldenFunk-MeshCore {ver} · {label} · ' + ('Update' if art == 'update' else 'Neuinstallation'),
                    'version': ver, 'new_install_prompt_erase': erase, 'new_install_improv_wait_time': 0,
                    'builds': [{'chipFamily': 'ESP32-S3', 'parts': [part]}]}
        devs.append({'id': gid, 'label': label, 'asset': f'{name}.zip', 'files': files, 'modes': {
            'update': manifest('update', {'path': files['update']['out'], 'offset': 0x10000}, True),
            'neu': manifest('neu', {'path': files['neu']['out'], 'offset': 0}, False)}})
    cfg = {'repo': REPO, 'tag': f'v{ver}', 'devices': devs}
    (HERE / 'flasher.json').write_text(json.dumps(cfg, ensure_ascii=False, indent=1) + '\n', encoding='utf-8')
    (HERE / 'index.html').write_text(html(cfg, ver), encoding='utf-8')
    for d in devs:
        for art, f in d['files'].items(): print(d['id'], art, f['member'].split('/')[-1], f['sha256'])

def html(cfg, ver):
    hardware_hint = f'HaldenFunk-MeshCore {ver} ist auf echter Hardware noch nicht abgenommen (Funktionen aus DisAHRsterRadio-MeshCore 0.9.4 übernommen).'
    rel = f"https://github.com/{cfg['repo']}/releases/tag/{cfg['tag']}"
    karten = []
    for d in cfg['devices']:
        u, n = d['files']['update'], d['files']['neu']
        an = ('T-Watch S3 per USB-C-<b>Daten</b>kabel an den Computer (Seitentaste kurz drücken, falls sie aus ist)'
              if d['id'] == 'twatch' else 'T-Deck Plus per USB-C-<b>Daten</b>kabel an den Computer, einschalten')
        karten.append(f'''<div class="card">
<h2>{d['label']}</h2>
<p>Antenne anschrauben. {an}. Dann in <b>Chrome oder Edge am Computer</b> einen der beiden Knöpfe wählen, Port wählen (meist „USB JTAG/serial debug unit“ bzw. <code>usbmodem</code>/<code>ttyACM</code>), warten bis „Installation complete“, Gerät neu starten.</p>
<div class="mode upd">
<h3>Update – Einstellungen bleiben <span class="tag">empfohlen</span></h3>
<p>Für Geräte, auf denen schon HaldenFunk-MeshCore 0.9.x läuft (T-Watch S3: erst ab 0.9.2 – beim ersten Mal Neuinstallation). Schreibt nur die Firmware (<code>…-firmware.bin</code> an <b>0x10000</b>). <b>Knotenidentität, Kontakte, Kanäle, Chatverlauf, Name, Funkprofil und Web-Admin-Passwort bleiben erhalten.</b></p>
<p class="hint">Im Fenster „Erase device“ das Kästchen <b>NICHT</b> anhaken, dann „Next“ → „Install“.</p>
<esp-web-install-button manifest="manifest-{d['id']}-update.json">
 <button slot="activate" class="b-upd">Update auf {ver} – {d['label']} (Einstellungen bleiben)</button>
 <span slot="unsupported">Dieser Browser kann keine USB-Geräte ansprechen (Web Serial fehlt). Bitte Chrome oder Edge am Computer verwenden – oder manuell flashen (siehe unten).</span>
 <span slot="not-allowed">Der Flasher braucht eine sichere Verbindung (https).</span>
</esp-web-install-button>
<p class="sha">Datei <code>{u['member'].split('/')[-1]}</code>, SHA-256 <code>{u['sha256']}</code></p>
</div>
<div class="mode neu">
<h3>Neuinstallation – alles löschen</h3>
<p>Für neue Geräte, Geräte mit anderer Firmware oder wenn das Update nicht startet. <b>Achtung: löscht den ganzen Speicher</b> – Knotenidentität (neuer Schlüssel, neue Kennung), Kontakte, Kanäle, Chatverlauf und alle Einstellungen sind danach weg. Schreibt <code>…-factory.bin</code> an <b>0x0</b>.</p>
<esp-web-install-button manifest="manifest-{d['id']}-neu.json">
 <button slot="activate" class="b-neu">Neuinstallation – {d['label']} (alles löschen)</button>
 <span slot="unsupported">Dieser Browser kann keine USB-Geräte ansprechen (Web Serial fehlt). Bitte Chrome oder Edge am Computer verwenden.</span>
 <span slot="not-allowed">Der Flasher braucht eine sichere Verbindung (https).</span>
</esp-web-install-button>
<p class="sha">Datei <code>{n['member'].split('/')[-1]}</code>, SHA-256 <code>{n['sha256']}</code></p>
</div>
<p class="sha">Beide Dateien aus <code>{d['asset']}</code> im <a href="{rel}">Release {cfg['tag']}</a>; der Flasher prüft die SHA-256-Werte beim Bereitstellen.</p>
</div>''')
    return f'''<!doctype html>
<html lang="de"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>HaldenFunk-MeshCore · Web-Flasher {ver} (T-Deck Plus, T-Watch S3)</title>
<script type="module" src="https://unpkg.com/esp-web-tools@10.4.0/dist/web/install-button.js?module"></script>
<style>
body{{margin:0;background:#121012;color:#f6eee6;font:16px/1.5 system-ui,"Noto Sans",sans-serif}}
main{{max-width:760px;margin:0 auto;padding:24px 16px 48px}}
h1{{font-size:28px;margin:0 0 4px}}h1 span{{color:#e8955a}}h2{{font-size:20px;color:#e8955a;margin:4px 0 8px}}h3{{font-size:17px;margin:0 0 6px}}
.card{{background:#1e1a1b;border:1px solid #5a4034;border-radius:12px;padding:16px;margin:18px 0}}
.mode{{border-radius:10px;padding:12px 14px;margin:12px 0}}.upd{{background:#2e2019;border:2px solid #e8955a}}.neu{{background:#2a1a12;border:2px solid #e07a3d}}
.neu h3{{color:#ffb38a}}.tag{{background:#e8955a;color:#121012;border-radius:6px;padding:1px 7px;font-size:13px;vertical-align:2px}}
.hint{{background:#0005;border-radius:6px;padding:6px 10px}}
.warn{{border-left:5px solid #e07a3d;background:#2a1a12;border-radius:8px;padding:12px 14px;margin:16px 0}}.warn b{{color:#ffb38a}}
a{{color:#e8955a}}code{{background:#0006;padding:1px 5px;border-radius:4px;font-size:14px}}.sha{{font-size:13px;word-break:break-all;margin:6px 0 0}}
esp-web-install-button button[slot=activate]{{border:0;border-radius:10px;padding:14px 20px;font:700 17px system-ui,sans-serif;cursor:pointer}}
.b-upd{{background:#e8955a;color:#121012}}.b-neu{{background:#3a2216;color:#ffb38a;border:2px solid #e07a3d!important;font-size:15px!important}}
li{{margin:4px 0}}
</style></head><body><main>
<h1>Halden<span>Funk</span> MeshCore</h1>
<p><b>Web-Flasher · Version {ver} · LILYGO T-Deck Plus und LILYGO T-Watch S3</b> (ESP32-S3)</p>
<p>Pro Gerät gibt es zwei Wege: <b>Update</b> (Standard, Einstellungen bleiben) und <b>Neuinstallation</b> (löscht alles). Nur das passende Gerät anschließen und den Knopf für genau dieses Gerät verwenden.</p>
{chr(10).join(karten)}
<h2>Kein Port zu sehen?</h2>
<ul><li>Anderes USB-Kabel probieren (viele Kabel laden nur).</li>
<li>T-Deck Plus: <b>Trackball gedrückt halten</b>, einschalten, Trackball loslassen, dann erneut klicken.</li>
<li>T-Watch S3: USB-C neu einstecken und erneut versuchen.</li>
<li>Linux: Benutzer in die Gruppe <code>dialout</code> (bzw. <code>uucp</code>) aufnehmen und neu anmelden.</li></ul>
<h2>Update startet nicht richtig?</h2>
<p>Bleibt das Display nach dem Update dunkel oder startet das Gerät immer wieder neu, lief vorher eine andere Firmware (andere Speicheraufteilung). Dann die <b>Neuinstallation</b> verwenden – dabei gehen Identität und Kontakte verloren.</p>
<h2>Danach</h2>
<p>Das Display zeigt das HaldenFunk-Bild (Halden und Förderturm), die Akku-Zeile und den Knopf „Advert senden“. Am Handy das offene WLAN <code>HaldenFunk-XXXXXX</code> wählen (ohne Passwort) und <b>http://192.168.4.1</b> öffnen. Bluetooth für die MeshCore-App <b>ohne PIN</b>. Offenes, unverschlüsseltes WLAN: Jede Person in Reichweite kann mitlesen und senden.</p>
<p>Weboberfläche und MeshCore-App teilen die Nachrichten: Empfangene Kanal- und Direktnachrichten erscheinen in beiden (auch wenn die App erst später verbunden wird), in der App gesendete Nachrichten stehen im Web-Verlauf. Im Web gesendete Nachrichten zeigt die App als empfangene Nachricht mit Markierung („&lt;Knotenname&gt; (Web)“ im Kanal, „[von mir über Web]“ bei Direktnachrichten) – das MeshCore-Protokoll kennt keine Meldung „von einem anderen Gerät gesendet“.</p>
{'<p class="hint">'+hardware_hint+'</p>'+chr(10)}<p>Manuell flashen (esptool) und Einzelheiten: <a href="https://github.com/{cfg['repo']}/blob/main/docs/FLASHEN.md">Anleitung „Flashen“</a> · <a href="{rel}">Release {cfg['tag']}</a>.</p>
<div class="warn"><b>Prototyp, kein zertifiziertes Notrufsystem.</b> Im Notfall immer 112.</div>
<p style="font-size:13px"><a href="../">Live-Demo</a> · <a href="https://github.com/{cfg['repo']}">Quellcode</a> · Flasher: <a href="https://github.com/esphome/esp-web-tools">ESP Web Tools</a> 10.4.0 (Apache-2.0)</p>
</main></body></html>
'''

if __name__ == '__main__':
    if len(sys.argv) != 3: sys.exit(__doc__)
    main(sys.argv[1:])
