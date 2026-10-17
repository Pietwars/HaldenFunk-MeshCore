#!/usr/bin/env bash
# Baut alle Envs eines Geraets aus tools/devices.json und packt ein Release-Asset.
# Aufruf: tools/build_device.sh <Geraet> [Ausgabeordner]
set -uo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd); DEV=$1; OUT=${2:-$ROOT/dist/devices}; VERSION=${VERSION:-0.1.0-alpha}
ESPTOOL=${ESPTOOL:-"python3 -m esptool"}; cd "$ROOT"; mkdir -p "$OUT"
read -r CHIP WEBUI ENVS < <(python3 -c "
import json,sys;d={e['device']:e for e in json.load(open('tools/devices.json'))}['$DEV']
print(d['chip'],int(d['webui']),' '.join(k+'='+v for k,v in d['envs'].items()))")
NAME=HaldenFunk-MeshCore-$VERSION-$DEV; D=$OUT/$NAME; rm -rf "$D"; mkdir -p "$D"; FAIL=0
for kv in $ENVS; do role=${kv%%=*}; env=${kv#*=}
  if ! pio run -e "$env" > "$OUT/$DEV-$role.log" 2>&1; then echo "FEHLER $DEV $role ($env)"; FAIL=1; continue; fi
  B=.pio/build/$env
  case $CHIP in
   esp32*)
    BOOT=0x0; [ "$CHIP" = esp32 ] && BOOT=0x1000
    BOOTAPP0=$(find ~/.platformio/packages/ -path '*framework-arduinoespressif32*' -name boot_app0.bin | head -1)
    ARGS="$BOOT $B/bootloader.bin 0x8000 $B/partitions.bin 0xe000 $BOOTAPP0 0x10000 $B/firmware.bin"
    mkdir -p "$D/teile-$role"; cp $B/bootloader.bin $B/partitions.bin $B/firmware.bin "$BOOTAPP0" "$D/teile-$role/"
    FSOFF=-
    if [ "$role" = wifi ]; then
      pio run -e "$env" -t buildfs >> "$OUT/$DEV-$role.log" 2>&1 || { echo "FEHLER buildfs $DEV"; FAIL=1; continue; }
      FSOFF=$(python3 -c "import struct,sys;d=open(sys.argv[1],'rb').read()
for i in range(0,len(d),32):
  m,t,st,o,sz=struct.unpack('<HBBII',d[i:i+12])
  if m==0x50aa and t==1 and st in(0x82,0x83):print(hex(o));break" $B/partitions.bin)
      cp $B/spiffs.bin "$D/teile-$role/"; ARGS="$ARGS $FSOFF $B/spiffs.bin"
    fi
    $ESPTOOL --chip $CHIP merge-bin -o "$D/$NAME-$role-factory.bin" --flash-mode keep --flash-freq keep --flash-size keep $ARGS >/dev/null || { echo "FEHLER merge $DEV $role"; FAIL=1; continue; }
    echo "$role|$CHIP|$BOOT|$FSOFF" >> "$D/.offsets";;
   nrf52|rp2040)
    [ $CHIP = nrf52 ] && [ ! -f $B/firmware.uf2 ] && python3 bin/uf2conv/uf2conv.py $B/firmware.hex -c -o $B/firmware.uf2 -f 0xADA52840 >/dev/null
    for f in $B/firmware.uf2 $B/firmware.zip; do [ -f $f ] && cp $f "$D/$NAME-$role.${f##*.}"; done
    ls "$D"/*-$role.uf2 >/dev/null 2>&1 || { echo "FEHLER keine UF2 $DEV $role"; FAIL=1; };;
   stm32)
    for f in $B/firmware.bin $B/firmware.hex; do [ -f $f ] && cp $f "$D/$NAME-$role.${f##*.}"; done;;
  esac
done
python3 tools/write_liesmich.py "$DEV" "$D" "$VERSION"
(cd "$D" && rm -f .offsets && sha256sum $(find . -type f ! -name SHA256SUMS | sort) > SHA256SUMS)
rm -f "$OUT/$NAME.zip"; (cd "$OUT" && python3 -m zipfile -c "$NAME.zip" "$NAME")
echo "$DEV fertig (Fehler=$FAIL) -> $OUT/$NAME.zip"; exit $FAIL
