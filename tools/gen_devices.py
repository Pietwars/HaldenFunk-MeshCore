#!/usr/bin/env python3
"""Erzeugt platformio.devices.ini und tools/devices.json fuer alle Geraete des MeshCore-Konfigurators
(Liste = MeshCore-v1.18.0-Releases unter flasher.meshcore.io, gespeichert in tools/devices.upstream.json)."""
import json,re,sys
from pathlib import Path
from platformio.project.config import ProjectConfig
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from version import version
VER=version()  # nur fuer die Ausgabe; die ini nutzt ${hf.version}
devs=json.loads((ROOT/'tools/devices.upstream.json').read_text())
# Bereits handgepflegte Portal-Profile
MANUAL={'LilyGo_T3S3_sx1262':'disaster_t3s3_sx1262_wifi','LilyGo_TLora_V2_1_1_6':'disaster_tlora_v2_1_sx1276_wifi','Heltec_v2':'disaster_heltec_v2_sx1276_wifi'}
(ROOT/'platformio.devices.ini').write_text('')
c=ProjectConfig(str(ROOT/'platformio.ini'))
def chip(e):
    p=c.get(f'env:{e}','platform',''); b=c.get(f'env:{e}','board','')
    if 'espressif32' in p:
        mcu=c.get(f'env:{e}','board_build.mcu','')
        if not mcu:
            for f in [ROOT/'boards'/f'{b}.json',*Path.home().glob(f'.platformio/platforms/*/boards/{b}.json')]:
                if f.exists(): mcu=json.loads(f.read_text())['build']['mcu']; break
        return mcu or 'esp32?'
    for k,v in (('nrf52','nrf52'),('raspberrypi','rp2040'),('ststm32','stm32')):
        if k in p: return v
    return '?'
NOPORTAL={'ThinkNode_M7':'eigene Display-Oberfläche ohne Hinweisfunktion (showAlert) – Portal baut nicht','M5Stack_Unit_C6L':'ESP32-C6: WLAN-Bibliothek (Network.h) im Portal-Build nicht auflösbar'}
NOROLES={'ThinkNode_M8':(['repeater','room_server'],'Repeater/Room-Server: Upstream-Linkfehler (EnvironmentSensorManager fehlt im Build)')}
ini=['; Automatisch erzeugt von tools/gen_devices.py – nicht von Hand bearbeiten','']
out=[]
for d in sorted(devs,key=str.lower):
    roles=devs[d]; first=f'{d}_{roles[0]}'; ch=chip(first)
    entry={'device':d,'chip':ch,'envs':{},'webui':False}
    if d in NOPORTAL: entry['note']=NOPORTAL[d]
    if d not in NOPORTAL and ch.startswith('esp32') and any(r.startswith('companion') for r in roles):
        entry['webui']=True
        if d in MANUAL: entry['envs']['wifi']=MANUAL[d]
        else:
            base=f'{d}_companion_radio_ble' if 'companion_radio_ble' in roles else f'{d}_companion_radio_usb'
            e=f'dar_{d}_wifi'; entry['envs']['wifi']=e
            flags=' '.join(c.get(f'env:{base}','build_flags',[]))
            unf=sorted(set(m.group(0).strip() for m in re.finditer(r'-D\s*(ENABLE_USB_INTERFACE|ENABLE_WIFI_INTERFACE|WIFI_SSID|WIFI_PWD|ENABLE_PRIVATE_KEY_EXPORT|ENABLE_PRIVATE_KEY_IMPORT|LORA_TX_POWER)(=\S+)?',flags) if m.group(0).strip().replace(' ','')!='-DLORA_TX_POWER=10'))
            ini+=[f'[env:{e}]',f'extends = env:{base}','test_framework = custom','lib_ldf_mode = deep+','build_unflags =',*['  '+u for u in unf],
                  'board_build.partitions = huge_app.csv',
                  'build_flags =',f'  ${{env:{base}.build_flags}}','  -D LORA_TX_POWER=10','  -D MAX_GROUP_CHANNELS=8','  -D DISASTER_PORTAL=1','  -D OFFLINE_QUEUE_SIZE=64',
                  '  -D DISASTER_WIFI_DELAY_MS=1500',"  -D FIRMWARE_VERSION='\"v1.18.0-hf${hf.version}-wifi\"'",
                  *(['  ${dar_classic_nimble.build_flags}'] if ch=='esp32' else []),
                  'lib_deps =',f'  ${{env:{base}.lib_deps}}','  densaugeo/base64 @ ~1.4.0',
                  *(['  ${dar_classic_nimble.lib_deps}','lib_ignore =','  ${hf_no_bluedroid.lib_ignore}',
                     'build_src_filter =',f'  ${{env:{base}.build_src_filter}}','  -<helpers/esp32/SerialBLEInterface.cpp>'] if ch=='esp32' else []),'']
    else:
        if d in NOROLES: entry['note']=NOROLES[d][1]
        for r in roles:
            if d in NOROLES and r in NOROLES[d][0]: continue
            if r=='companion_radio_usb' and 'companion_radio_ble' in roles: continue
            base=f'{d}_{r}'; key='companion' if r.startswith('companion') else r
            e=f'dar_{d}_{key}'; entry['envs'][key]=e
            ini+=[f'[env:{e}]',f'extends = env:{base}','build_flags =',f'  ${{env:{base}.build_flags}}',"  -D FIRMWARE_VERSION='\"v1.18.0-hf${hf.version}\"'",'']
    out.append(entry)
(ROOT/'platformio.devices.ini').write_text('\n'.join(ini)+'\n')
(ROOT/'tools/devices.json').write_text(json.dumps(out,indent=1)+'\n')
print(VER+':',len(out),'Geraete,',sum(e['webui'] for e in out),'mit Web-Oberflaeche,',sum(len(e['envs']) for e in out),'Envs')
