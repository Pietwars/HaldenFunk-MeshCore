#!/usr/bin/env bash
# Flashpakete je Board: factory.bin (ab 0x0) + Teile + SHA256SUMS + ANLEITUNG.md
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd); OUT=${1:-$ROOT/dist}
PREFIX=${PREFIX:-HaldenFunk-MeshCore}; VERSION=${VERSION:-0.1.0-alpha}; REPO=${REPO:-https://github.com/Pietwars/HaldenFunk-MeshCore}
TEMPLATE=${TEMPLATE:-$ROOT/tools/ANLEITUNG.template.md}; PIODIR=${PIODIR:-$ROOT}; cd "$PIODIR"
ESPTOOL=${ESPTOOL:-"python3 -m esptool"}
COMMIT=$(git rev-parse --short HEAD 2>/dev/null || echo unbekannt)
mkdir -p "$OUT"
declare -A CHIP=([disaster_t3s3_sx1262_wifi]=esp32s3 [disaster_tlora_v2_1_sx1276_wifi]=esp32 [disaster_heltec_v2_sx1276_wifi]=esp32)
declare -A BOOT=([disaster_t3s3_sx1262_wifi]=0x0 [disaster_tlora_v2_1_sx1276_wifi]=0x1000 [disaster_heltec_v2_sx1276_wifi]=0x1000)
declare -A NAME=([disaster_t3s3_sx1262_wifi]="LILYGO T3-S3 (SX1262)" [disaster_tlora_v2_1_sx1276_wifi]="LILYGO T3 V1.6.1 (SX1276)" [disaster_heltec_v2_sx1276_wifi]="Heltec WiFi LoRa 32 V2 (SX1276, WLAN-Fix 8,5 dBm)")
declare -A ZIP=([disaster_t3s3_sx1262_wifi]=t3s3 [disaster_tlora_v2_1_sx1276_wifi]=t3-v161 [disaster_heltec_v2_sx1276_wifi]=heltec-v2)
for env in disaster_t3s3_sx1262_wifi disaster_tlora_v2_1_sx1276_wifi disaster_heltec_v2_sx1276_wifi; do
  pio run -e $env >/dev/null && pio run -e $env -t buildfs >/dev/null
  B=.pio/build/$env; Z=$VERSION-${ZIP[$env]}; D=$OUT/$PREFIX-$Z; rm -rf "$D"; mkdir -p "$D/teile"
  BOOTAPP0=$(find ~/.platformio/packages/ -path '*framework-arduinoespressif32*' -name boot_app0.bin | head -1)
  cp $B/bootloader.bin $B/partitions.bin $B/firmware.bin $B/spiffs.bin "$D/teile/"; cp "$BOOTAPP0" "$D/teile/boot_app0.bin"
  FSOFF=$(python3 -c "import struct,sys;d=open(sys.argv[1],'rb').read()
for i in range(0,len(d),32):
  m,t,st,o,sz=struct.unpack('<HBBII',d[i:i+12])
  if m==0x50aa and t==1 and st in(0x82,0x83):print(hex(o));break" $B/partitions.bin)
  $ESPTOOL --chip ${CHIP[$env]} merge-bin -o "$D/$PREFIX-$Z-factory.bin" --flash-mode keep --flash-freq keep --flash-size keep \
    ${BOOT[$env]} $B/bootloader.bin 0x8000 $B/partitions.bin 0xe000 "$BOOTAPP0" 0x10000 $B/firmware.bin $FSOFF $B/spiffs.bin >/dev/null
  sed -e "s/@BOARD@/${NAME[$env]}/g" -e "s/@ZIP@/$Z/g" -e "s/@BOOT@/${BOOT[$env]}/g" -e "s/@COMMIT@/$COMMIT/g" -e "s/@FSOFF@/$FSOFF/g" -e "s/@PREFIX@/$PREFIX/g" -e "s|@REPO@|$REPO|g" -e "s/@VERSION@/$VERSION/g" "$TEMPLATE" > "$D/ANLEITUNG.md"
  (cd "$D" && sha256sum *.bin teile/*.bin > SHA256SUMS)
  rm -f "$OUT/$PREFIX-$Z.zip"; (cd "$OUT" && python3 -m zipfile -c "$PREFIX-$Z.zip" "$PREFIX-$Z")
  echo "$env -> $OUT/$PREFIX-$Z.zip"
done
