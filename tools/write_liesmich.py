#!/usr/bin/env python3
"""Schreibt ANLEITUNG.md fuer ein Geraete-Paket (Paketaufbau ab 0.9.2 wie DisAHRsterRadio-MeshCore)."""
import json,sys
from pathlib import Path
dev,d,ver=sys.argv[1],Path(sys.argv[2]),sys.argv[3]
e={x['device']:x for x in json.load(open(Path(__file__).parent/'devices.json'))}[dev]
off={}
if (d/'.offsets').exists():
    for l in (d/'.offsets').read_text().split():
        f=l.split('|')
        if len(f)<6: f+=['-'+f[0],'teile-'+f[0]]
        off[f[0]]=(f[1],f[2],f[3],f[4],f[5])
files=sorted(str(p.relative_to(d)) for p in d.rglob('*') if p.is_file() and not p.name.startswith('.'))
NAMES={'LilyGo_TDeck':'LILYGO T-Deck Plus','LilyGo_TWatchS3':'LILYGO T-Watch S3'}
FLASHER='https://pietwars.github.io/HaldenFunk-MeshCore/flasher/'
L=[f'# HaldenFunk-MeshCore {ver} für {NAMES.get(dev,dev)}','',
'> Offenes WLAN ohne Passwort, MeshCore-Bluetooth ohne Zahlencode – einfach verbinden. Alte Kopplungen bei Bedarf am Handy entfernen.','',
f'Chip: **{e["chip"]}** · Web-Oberfläche: **{"ja" if e["webui"] else "nein – ohne Web-Oberfläche"}** · https://github.com/Pietwars/HaldenFunk-MeshCore','',
'> ⚠️ Kein zertifiziertes Notrufsystem. In Lebensgefahr immer **112** wählen. '+(f'Version {ver} ist auf diesem Gerät **noch nicht auf echter Hardware getestet** (Funktionen übernommen aus DisAHRsterRadio-MeshCore 0.9.4).' if dev in NAMES else 'Dieses Gerät ist **nicht auf echter Hardware getestet**.'),'',
'**Antenne anschrauben, bevor das Gerät Strom bekommt.** Funkprofil: MeshCore-Standard EU/UK 869,618 MHz, 62,5 kHz, SF8, CR 4/8 – auf allen Geräten gleich einstellen.','']
if e['chip'].startswith('esp32'):
    for r,(ch,b,fs,suf,teile) in off.items():
        L+=[f'## {"WLAN-Portal mit Web-Oberfläche" if r=="wifi" else r}','']
        if dev in NAMES: L+=[f'Web-Flasher (Chrome/Edge am Computer): {FLASHER} – sobald er auf {ver} umgestellt ist, gibt es dort je Gerät „Update auf {ver} – {NAMES[dev]} (Einstellungen bleiben)“ (nur `firmware.bin` an 0x10000) und „Neuinstallation – {NAMES[dev]} (alles löschen)“ (`factory.bin` an 0x0). Ohne Web-Flasher:','']
        L+=[
            '1. Chrome/Edge, https://espressif.github.io/esptool-js/ öffnen, **Connect**, Port wählen ('+('Trackball gedrückt halten und einschalten' if dev=='LilyGo_TDeck' else 'USB-C neu einstecken' if dev=='LilyGo_TWatchS3' else 'ggf. BOOT gedrückt halten und RST drücken')+').',
            f'2. Neuinstallation: Flash Address **`0x0`**, Datei `{d.name}{suf}-factory.bin`, **Program**, danach neu starten (löscht Einstellungen, Identität, Kontakte, Verlauf).',
            f'3. Update ohne Datenverlust: nur `{d.name}{suf}-firmware.bin` an **`0x10000`**.','',
            f'esptool: `python -m esptool --chip {ch} write_flash 0x0 {d.name}{suf}-factory.bin` bzw. `… write_flash 0x10000 {d.name}{suf}-firmware.bin`.','',
            f'Einzelteile in `{teile}/`: bootloader.bin {b}, partitions.bin 0x8000, boot_app0.bin 0xe000'+(f', spiffs.bin {fs}' if fs!='-' else '')+'; Firmware 0x10000.','']
        if r=='wifi' and dev=='LilyGo_TWatchS3': L+=['**Neu in 0.9.2:** Beim ersten Aufspielen (andere Firmware vorher) die Neuinstallation (`factory.bin` an `0x0`) verwenden; spätere HaldenFunk-Updates nur mit `firmware.bin` an `0x10000`.','']
        elif r=='wifi': L+=['**Update von 0.9.x / 0.1.x-alpha:** gleiche Partitionstabelle – es reicht die `firmware.bin` an `0x10000`; Einstellungen, Identität und Kontakte bleiben erhalten.'+(' **Klassischer ESP32:** Bluetooth läuft mit NimBLE – bei Problemen die alte Kopplung am Handy entfernen und die MeshCore-App neu koppeln (ohne Zahlencode).' if ch=='esp32' else ''),'',
            'Danach mit dem offenen WLAN `HaldenFunk-XXXXXX` verbinden (nichts eingeben). Im Browser http://192.168.4.1 öffnen.','',
            'Parallel kann sich die offizielle MeshCore-App per **Bluetooth** verbinden (falls das Gerät BLE hat) – Kopplung ohne Zahlencode.','',
            '**Weboberfläche und MeshCore-App teilen die Nachrichten:** Empfangene Nachrichten bekommen beide, auch wenn die App erst später verbunden wird. In der App gesendete Nachrichten stehen auch im Web-Verlauf. Im Web gesendete Nachrichten erscheinen in der App als empfangene Nachricht mit Markierung (Kanal: Absender „<Knotenname> (Web)“, Direkt: Text beginnt mit „[von mir über Web]“) – das MeshCore-Protokoll kennt keine „von einem anderen Gerät gesendet“-Meldung.','']
elif e['chip'] in('nrf52','rp2040'):
    L+=['## Flashen per UF2','','1. Gerät per USB anschließen und **zweimal schnell Reset** drücken (RP2040: BOOTSEL gedrückt halten beim Einstecken). Es erscheint ein USB-Laufwerk.',
        '2. Die passende `.uf2`-Datei auf das Laufwerk kopieren. Das Gerät startet neu.','',
        'nRF52: Die `.zip` ist ein DFU-Paket für `adafruit-nrfutil` über USB (Bluetooth-DFU ist in dieser Firmware abgeschaltet).','',
        'Rollen: `companion` = MeshCore-Companion für die MeshCore-App (Bluetooth bzw. USB), `repeater` = Repeater, `room_server` = Room-Server (Schwarzes Brett).','',
        'Konfiguration über die offizielle MeshCore-App bzw. https://config.meshcore.dev / https://meshcore.co.uk/configurator/ – **keine** HaldenFunk-Web-Oberfläche (kein WLAN auf diesem Chip).','']
else:
    L+=['## STM32','','`.bin`/`.hex` mit STM32CubeProgrammer oder einem SWD-Programmer flashen. Ohne Web-Oberfläche.','']
BILD='Das Display zeigt nur das HaldenFunk-Bild (Halden und Förderturm bei Nacht mit Funkwellen, Schriftzug „⚒ HaldenFunk – Glück Auf · MeshCore“), darunter eine Akku-Zeile (Ladestand, Spannung, geschätzter Verbrauch und Restlaufzeit) und den kupferfarbenen Knopf **„Advert senden“** (Flood-Advert, höchstens einer je 10 s). Keine weiteren Seiten, keine Nachrichten am Gerät (Nachrichten und Einstellungen über MeshCore-App und Weboberfläche). Nach 30 s ohne Bedienung geht das Display aus.'
BRAND={
 'LilyGo_TDeck':['## Anzeige und Tasten (LILYGO T-Deck Plus)','',BILD,'',
  '| Bedienung | Wirkung |','|---|---|','| Trackball kurz klicken | Display an/aus |','| Trackball **2,5 s** halten | Flood-Advert senden |','| Trackball **10 s** halten | neues Web-Admin-Passwort (nur über USB-Seriell sichtbar; wie bisher) |','| Tastatur (auch Enter) | keine Funktion |','| Ein/Aus | Schiebeschalter (kein Abschalten per Trackball) |','',
  'Akku: Das T-Deck Plus misst nur die Spannung (Spannungsteiler an GPIO 4, eFuse-kalibrierter ADC). Ladestand „ca. … %“ ist aus der Spannung geschätzt; Laden und USB sind nicht messbar.',''],
 'LilyGo_TWatchS3':['## Anzeige und Tasten (LILYGO T-Watch S3)','',BILD,'',
  '| Bedienung | Wirkung |','|---|---|','| Seitentaste kurz | Display an/aus (bei ausgeschalteter Uhr: einschalten) |','| Seitentaste ca. **2 s** halten | Flood-Advert senden |','| Seitentaste ca. **6 s** halten | Uhr ausschalten (Power-Chip) |','| Knopf „Advert senden“ antippen | Flood-Advert senden |','| Antippen bei dunklem Display | nur aufwecken |','',
  'Akku: Ladestand, Spannung, USB und Laden kommen vom Power-Chip AXP2101. Verbrauch und Restlaufzeit sind Schätzungen aus dem Spannungsverlauf (kein Strommesser).',''],
}
if dev in BRAND:
    L+=BRAND[dev]
    if any(f.startswith('vorschau/') for f in files): L+=['Vorschaubilder der Anzeige (am PC nachgestellt, Beispielwerte): `vorschau/`.','']
if any(f.startswith('memory-report') for f in files): L+=['Flash-/Speicherprüfung (Header, Partitionen, factory.bin, RAM): `memory-report.md` / `memory-report.json`.','']
L+=['## Dateien','']+[f'- `{f}`' for f in files]+['','Prüfsummen: `SHA256SUMS`.']
(d/'ANLEITUNG.md').write_text('\n'.join(L)+'\n',encoding='utf-8')
