#!/usr/bin/env python3
"""Validate release ESP32 artifacts without hardware or third-party Python packages.

The board contract is independent of PlatformIO's inherited defaults. Never patch
an image here: reject mismatches before packaging instead. Sizes are bytes.
"""
import argparse
import hashlib
import json
import os
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BOARDS = ROOT / 'tools/release_boards.json'


def require(condition, message):
    if not condition:
        raise ValueError(message)


def image_header(data, board, name):
    require(len(data) >= 24, f'{name}: truncated ESP32 header')
    magic, segments, mode, size_freq = struct.unpack_from('<BBBB', data)
    require(magic == 0xE9, f'{name}: invalid image magic')
    require(1 <= segments <= 16, f'{name}: invalid segment count')
    require(mode <= 5, f'{name}: invalid flash mode')
    require(size_freq & 15 in (0, 1, 2, 15), f'{name}: invalid flash frequency')
    size_code = size_freq >> 4
    require(size_code <= 7, f'{name}: unknown flash size code {size_code}')
    flash = (1 << size_code) * 1024 * 1024
    require(flash == board['flash_bytes'],
            f'{name}: header flash size {flash} != board {board["flash_bytes"]}')
    require(struct.unpack_from('<H', data, 12)[0] == board['chip_id'],
            f'{name}: wrong ESP32 chip ID')
    require(data[23] in (0, 1), f'{name}: invalid hash flag')
    # Walk every segment so a valid-looking header cannot hide a truncated image.
    cursor, checksum = 24, 0xEF
    for _ in range(segments):
        require(cursor + 8 <= len(data), f'{name}: truncated segment header')
        length = struct.unpack_from('<I', data, cursor + 4)[0]
        cursor += 8
        require(cursor + length <= len(data), f'{name}: truncated segment data')
        for byte in data[cursor:cursor + length]:
            checksum ^= byte
        cursor += length
    checksum_at = cursor + (15 - cursor % 16)
    require(checksum_at < len(data), f'{name}: missing image checksum')
    require(data[checksum_at] == checksum, f'{name}: image checksum mismatch')
    end = checksum_at + 1
    if data[23]:
        require(len(data) >= end + 32, f'{name}: truncated SHA256 digest')
        require(data[end:end + 32] == hashlib.sha256(data[:end]).digest(),
                f'{name}: image SHA256 mismatch')
        end += 32
    # Signed/encrypted release images need an explicit verification implementation.
    require(end == len(data), f'{name}: unsupported trailing image data')
    return {'flash_bytes': flash, 'mode': mode, 'frequency_code': size_freq & 15}


def partitions(data, board):
    require(0 < len(data) <= 4096 and len(data) % 32 == 0,
            'partition table: invalid length')
    entries, labels, md5_seen, terminated = [], set(), False, False
    for pos in range(0, len(data), 32):
        row = data[pos:pos + 32]
        if row == b'\xff' * 32:
            require(data[pos:] == b'\xff' * (len(data) - pos),
                    'partition table: data after terminator')
            terminated = True
            break
        magic = struct.unpack_from('<H', row)[0]
        if magic == 0xEBEB:
            require(not md5_seen and row[2:16] == b'\xff' * 14,
                    'partition table: invalid MD5 record')
            require(row[16:] == hashlib.md5(data[:pos]).digest(),
                    'partition table: MD5 mismatch')
            md5_seen = True
            continue
        require(not md5_seen, 'partition table: entry after MD5')
        require(magic == 0x50AA, 'partition table: invalid entry magic')
        _, kind, subtype, offset, size, raw_label, flags = struct.unpack('<HBBII16sI', row)
        label = raw_label.split(b'\0', 1)[0].decode('ascii')
        require(label and label not in labels, 'partition table: empty/duplicate label')
        labels.add(label)
        require(kind in (0, 1), f'{label}: unsupported partition type')
        require(flags == 0, f'{label}: encrypted/read-only partitions unsupported')
        require(size > 0 and size % 4096 == 0, f'{label}: invalid size/alignment')
        require(offset % (65536 if kind == 0 else 4096) == 0,
                f'{label}: invalid offset alignment')
        require(offset >= board['partition_offset'] + 4096,
                f'{label}: overlaps partition table/bootloader')
        require(offset + size <= board['flash_bytes'], f'{label}: exceeds physical flash')
        entries.append(dict(label=label, type=kind, subtype=subtype, offset=offset, size=size))
    require(terminated and entries, 'partition table: missing entries/terminator')
    ordered = sorted(entries, key=lambda p: p['offset'])
    for left, right in zip(ordered, ordered[1:]):
        require(left['offset'] + left['size'] <= right['offset'],
                f'partition table: overlap {left["label"]}/{right["label"]}')
    return entries


def memory_usage(log, board, app_size):
    # PlatformIO's linker size report includes .data/.bss for static RAM. This is
    # deliberately separate from firmware.bin length and runtime free heap.
    clean = re.sub(r'\x1b\[[0-9;]*m', '', log)
    result = {}
    for name in ('RAM', 'Flash'):
        matches = re.findall(name + r':[^\n]*\(used (\d+) bytes from (\d+) bytes\)', clean)
        require(matches, f'build log: missing {name} size report')
        used, total = map(int, matches[-1])
        require(0 < used <= total, f'build log: invalid/overflowing {name} usage')
        expected = board['ram_bytes'] if name == 'RAM' else app_size
        require(total == expected, f'build log: {name} capacity {total} != {expected}')
        result[name.lower()] = dict(used=used, total=total, free=total - used)
    return result


def validate_artifacts(board, firmware, bootloader, table, filesystem, log,
                       factory=None, boot_app0=None):
    header = image_header(firmware, board, 'firmware.bin')
    image_header(bootloader, board, 'bootloader.bin')
    require(board['boot_offset'] + len(bootloader) <= board['partition_offset'],
            'bootloader overlaps partition table')
    parts = partitions(table, board)
    apps = [p for p in parts if p['type'] == 0 and p['offset'] == board['app_offset']]
    require(len(apps) == 1, 'missing application partition at expected flash offset')
    app = apps[0]
    require(len(firmware) <= app['size'], 'firmware.bin exceeds application partition')
    fs = [p for p in parts if p['type'] == 1 and p['subtype'] == 0x82]
    require(len(fs) == 1, 'expected exactly one SPIFFS partition')
    require(0 < len(filesystem) <= fs[0]['size'], 'spiffs.bin empty or exceeds partition')
    images = [(board['boot_offset'], bootloader), (board['partition_offset'], table),
              (app['offset'], firmware), (fs[0]['offset'], filesystem)]
    if boot_app0 is not None:
        require(0 < len(boot_app0) <= 8192, 'invalid boot_app0.bin size')
        require(any(p['type'] == 1 and p['subtype'] == 0 and
                    p['offset'] <= 0xE000 and 0xE000 + len(boot_app0) <= p['offset'] + p['size']
                    for p in parts), 'boot_app0.bin does not fit OTA data partition')
        images.append((0xE000, boot_app0))
    images.sort()
    for (off, data), (next_off, _) in zip(images, images[1:]):
        require(off + len(data) <= next_off, 'flash image components overlap')
    extent = max(off + len(data) for off, data in images)
    require(extent <= board['flash_bytes'], 'flash images exceed physical flash')
    if factory is not None:
        require(len(factory) == extent, 'factory.bin has wrong length/flash extent')
        cursor = 0
        for off, data in images:
            require(factory[cursor:off] == b'\xff' * (off - cursor),
                    'factory.bin: unexpected data in padding')
            require(factory[off:off + len(data)] == data,
                    f'factory.bin: component mismatch at {off:#x}')
            cursor = off + len(data)
    usage = memory_usage(log, board, app['size'])
    return dict(header=header, partitions=parts, memory=usage,
                firmware_bytes=len(firmware), app_free_bytes=app['size'] - len(firmware),
                filesystem_bytes=len(filesystem), flash_extent_bytes=extent,
                flash_unused_bytes=board['flash_bytes'] - max(p['offset'] + p['size'] for p in parts),
                factory_checked=factory is not None)


def markdown(report):
    b, r = report['board'], report['result']
    ram, flash = r['memory']['ram'], r['memory']['flash']
    lines = [f'# Speicherbericht: {report["device"]}', '',
             f'Environment: `{b["env"]}` · Commit: `{report["commit"]}`', '',
             '| Messwert | Bytes |', '|---|---:|',
             f'| Physischer Flash laut Boardvertrag | {b["flash_bytes"]} |',
             f'| App-Partition | {flash["total"]} |',
             f'| Flash laut Linker | {flash["used"]} |',
             f'| Firmware-Datei einschließlich Header/Digest | {r["firmware_bytes"]} |',
             f'| Frei in App-Partition nach Firmware-Datei | {r["app_free_bytes"]} |',
             f'| SPIFFS-Abbild | {r["filesystem_bytes"]} |',
             f'| Flash-Endadresse der Abbilder (exklusiv) | {r["flash_extent_bytes"]} |',
             f'| Nicht partitionierter Flash | {r["flash_unused_bytes"]} |',
             f'| Statisches RAM (.data/.bss) | {ram["used"]} |',
             f'| Linker-RAM-Budget | {ram["total"]} |',
             f'| RAM nach statischer Belegung (kein Laufzeit-Heapwert) | {ram["free"]} |', '',
             'Laufzeit-Heap, minimaler freier Heap, Stack und PSRAM sind hier nicht gemessen.',
             'WLAN/BLE-Koexistenz und Flash-Chiperkennung benötigen echte Hardware.', '',
             f'Factory-Abbild geprüft: {r["factory_checked"]}.', '',
             '| Partition | Typ/Subtyp | Offset | Größe (Bytes) |', '|---|---|---|---:|']
    lines += [f'| {p["label"]} | {p["type"]:#x}/{p["subtype"]:#x} | {p["offset"]:#x} | {p["size"]} |'
              for p in r['partitions']]
    return '\n'.join(lines) + '\n'


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--env', required=True)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--build-log', type=Path, required=True)
    parser.add_argument('--factory', type=Path)
    parser.add_argument('--boot-app0', type=Path)
    parser.add_argument('--output', type=Path, required=True, help='report path without extension')
    args = parser.parse_args(argv)
    try:
        boards = json.loads(BOARDS.read_text())
        matches = [(device, b) for device, b in boards.items() if b['env'] == args.env]
        require(len(matches) == 1, f'no release board contract for {args.env}')
        device, board = matches[0]
        read = lambda name: (args.build_dir / name).read_bytes()
        result = validate_artifacts(board, read('firmware.bin'), read('bootloader.bin'),
                                    read('partitions.bin'), read('spiffs.bin'),
                                    args.build_log.read_text(errors='replace'),
                                    args.factory.read_bytes() if args.factory else None,
                                    args.boot_app0.read_bytes() if args.boot_app0 else None)
        report = dict(schema_version=1, device=device, board=board, result=result,
                      commit=os.environ.get('GITHUB_SHA', 'local'),
                      sha256={name: hashlib.sha256(read(name)).hexdigest()
                              for name in ('firmware.bin', 'bootloader.bin', 'partitions.bin', 'spiffs.bin')})
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.with_suffix('.json').write_text(json.dumps(report, indent=2) + '\n')
        args.output.with_suffix('.md').write_text(markdown(report))
        print(f'{device}: flash/header/partitions/firmware/RAM valid; report {args.output}.md')
        return 0
    except (ValueError, OSError, KeyError, struct.error) as exc:
        print(f'FEHLER: {exc}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    sys.exit(main())
