#!/usr/bin/env python3
"""Prüft die Web-Portal-Grenzen (0.9.1): kein Web-Admin mehr; Funkprofil, Name, Kanäle, Verlauf löschen ohne Anmeldung."""
import re
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
src = (ROOT / 'examples/companion_radio/DisasterPortal.h').read_text(encoding='utf-8')
html = (ROOT / 'web/index.html').read_text(encoding='utf-8')
def handler(route):
    m = re.search(r'http\.on\("' + re.escape(route) + r'",HTTP_POST,\[this\]\(\)\{(.*?)\n    \}\);', src, re.S)
    assert m, f'{route} fehlt'
    return m.group(1)
# Web-Admin komplett entfernt (0.9.1): kein Login, kein Passwort, keine AdminSession-Nutzung
for gone in ('/api/admin/login', '/api/admin/logout', '/api/admin/password'):
    assert f'"{gone}"' not in src, f'{gone} darf es nicht mehr geben'
for gone in ('adminAllowed', 'adminRoom', 'AdminSession admin', 'X-Disaster-Admin', 'ADMIN-PASSWORT', 'resetAdminPassword'):
    assert gone not in src, gone
# Ohne Anmeldung: Funkprofil (nur dePreset), Knotenname (1-24 Bytes), Kanäle, Verlauf löschen
radio = handler('/api/radio')
assert 'if(!room())return;' in radio and 'disaster::dePreset(' in radio
name = handler('/api/name')
assert 'if(!room())return;' in name and 'name.length()<1||name.length()>24||hasNull(name)' in name
assert 'if(!room())return;' in handler('/api/channel')
assert 'if(!room())return;' in handler('/api/history-wipe')
# Weboberfläche: kein Admin-Bereich, alle Bedienelemente im Reiter Gerät
for gone in ('id="admin"', 'id="apass"', 'id="alogin"', 'id="admbtn"', 'adminToken', 'X-Disaster-Admin', 'admin/'):
    assert gone not in html, gone
for need in ('id="nname"', 'id="setname"', "act('name'", 'id="rpreset"', "act('radio'", 'id="cslot"', 'id="setch"', "act('channel'", 'id="wipe"', "act('history-wipe')", 'confirm('):
    assert need in html, need
assert set(re.findall(r'<option value="([a-z-]+)">[^<]*MHz', html)) == {'de-narrow', 'ahr-legacy', 'eu-legacy'}
print('portal_routes.test.py: ok')
