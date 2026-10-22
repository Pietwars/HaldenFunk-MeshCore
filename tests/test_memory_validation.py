"""Host regression tests: synthetic ESP images, real binary partition format."""
import hashlib
import json
import os
import struct
import subprocess
import sys
import tempfile
import unittest
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import validate_memory as v


def image(payload=b'firmware', size_code=4, chip=9, digest=True):
    header = bytearray(24)
    header[:4] = bytes((0xE9, 1, 2, size_code << 4 | 15))
    struct.pack_into('<H', header, 12, chip)
    header[23] = int(digest)
    data = header + struct.pack('<II', 0x3C000020, len(payload)) + payload
    checksum = 0xEF
    for byte in payload:
        checksum ^= byte
    data += bytes(15 - len(data) % 16) + bytes((checksum,))
    if digest:
        data += hashlib.sha256(data).digest()
    return bytes(data)


def partition(label, kind, subtype, offset, size):
    return struct.pack('<HBBII16sI', 0x50AA, kind, subtype, offset, size, label.encode(), 0)


def table(rows=None, md5=True):
    rows = rows if rows is not None else [
        partition('nvs', 1, 2, 0x9000, 0x5000),
        partition('otadata', 1, 0, 0xE000, 0x2000),
        partition('app0', 0, 0x10, 0x10000, 0x300000),
        partition('spiffs', 1, 0x82, 0x310000, 0xE0000),
        partition('coredump', 1, 3, 0x3F0000, 0x10000)]
    data = b''.join(rows)
    if md5:
        data += b'\xeb\xeb' + b'\xff' * 14 + hashlib.md5(data).digest()
    return data + b'\xff' * (0xC00 - len(data))


class MemoryValidation(unittest.TestCase):
    def setUp(self):
        self.board = json.loads(v.BOARDS.read_text())['LilyGo_TDeck']
        self.firmware, self.boot, self.table, self.fs = image(), image(b'boot'), table(), b'fs'
        self.log = ('RAM: [==] 48.9% (used 160252 bytes from 327680 bytes)\n'
                    'Flash: [==] 65.4% (used 2058269 bytes from 3145728 bytes)\n')

    def validate(self, **overrides):
        args = dict(board=self.board, firmware=self.firmware, bootloader=self.boot,
                    table=self.table, filesystem=self.fs, log=self.log)
        args.update(overrides)
        return v.validate_artifacts(**args)

    def test_supported_boards(self):
        for board in json.loads(v.BOARDS.read_text()).values():
            with self.subTest(board=board['env']):
                report = self.validate(board=board)
                self.assertEqual(report['flash_unused_bytes'], 12 * 1024 * 1024)
                self.assertEqual(report['memory']['ram']['free'], 167428)

    def test_bad_headers(self):
        for data in [b'', image(size_code=2), image(chip=0), self.firmware[:24],
                     self.firmware[:-1], self.firmware + b'x']:
            with self.subTest(data=data[:4]), self.assertRaises(ValueError):
                self.validate(firmware=data)
        for index, value in [(0, 0), (1, 0), (1, 17), (2, 6), (3, 0x42), (3, 0x8F), (23, 2)]:
            bad = bytearray(self.firmware)
            bad[index] = value
            with self.subTest(index=index, value=value), self.assertRaises(ValueError):
                self.validate(firmware=bad)

    def test_bootloader_header_also_checked(self):
        with self.assertRaisesRegex(ValueError, 'header flash size'):
            self.validate(bootloader=image(size_code=2))

    def test_corrupt_payload_checksum_and_digest(self):
        for index in [32, 47, -1]:
            bad = bytearray(self.firmware)
            bad[index] ^= 1
            with self.subTest(index=index), self.assertRaises(ValueError):
                self.validate(firmware=bad)

    def test_no_digest_supported(self):
        self.validate(firmware=image(digest=False))

    def test_partition_integrity(self):
        for bad in [b'', self.table[:31], self.table[:-1], b'\xff' * 0xC00,
                    self.table[:160] + b'\x00' * 32 + self.table[192:]]:
            with self.subTest(length=len(bad)), self.assertRaises(ValueError):
                self.validate(table=bad)
        bad = bytearray(self.table)
        bad[12] ^= 1
        with self.assertRaisesRegex(ValueError, 'MD5'):
            self.validate(table=bad)

    def test_partition_bounds_alignment_and_duplicates(self):
        cases = [
            [partition('app0', 0, 0x10, 0x10000, 0x1100000)],
            [partition('app0', 0, 0x10, 0x11000, 0x300000)],
            [partition('app0', 0, 0x10, 0x10000, 1)],
            [partition('nvs', 1, 2, 0x8000, 0x1000)],
            [partition('same', 1, 2, 0x9000, 0x1000), partition('same', 1, 2, 0xA000, 0x1000)],
            [partition('a', 1, 2, 0x9000, 0x5000), partition('b', 1, 2, 0xA000, 0x1000)],
        ]
        for rows in cases:
            with self.subTest(rows=rows), self.assertRaises(ValueError):
                self.validate(table=table(rows))

    def test_missing_app_or_filesystem(self):
        for rows in [[partition('nvs', 1, 2, 0x9000, 0x1000)],
                     [partition('app0', 0, 0, 0x10000, 0x300000)]]:
            with self.assertRaises(ValueError):
                self.validate(table=table(rows))

    def test_oversize_firmware_and_filesystem(self):
        with self.assertRaisesRegex(ValueError, 'application partition'):
            self.validate(firmware=image(b'x' * 0x300000))
        with self.assertRaisesRegex(ValueError, 'spiffs.bin'):
            self.validate(filesystem=b'x' * 0xE0001)
        with self.assertRaises(ValueError):
            self.validate(filesystem=b'')

    def test_exact_partition_boundary(self):
        firmware = image(b'x' * (0x300000 - 80))
        self.assertEqual(len(firmware), 0x300000)
        self.assertEqual(self.validate(firmware=firmware)['app_free_bytes'], 0)

    def test_invalid_size_reports(self):
        for log in ['', self.log.replace('327680', '100000'),
                    self.log.replace('160252', '400000'), self.log.replace('3145728', '16777216')]:
            with self.subTest(log=log), self.assertRaises(ValueError):
                self.validate(log=log)

    def test_factory_components_and_padding(self):
        boot_app = b'ota'
        factory = bytearray(b'\xff' * (0x310000 + len(self.fs)))
        for off, data in [(0, self.boot), (0x8000, self.table), (0xE000, boot_app),
                          (0x10000, self.firmware), (0x310000, self.fs)]:
            factory[off:off + len(data)] = data
        self.assertTrue(self.validate(factory=factory, boot_app0=boot_app)['factory_checked'])
        for index in [0, 0x4000, 0x8000, 0xE000, 0x10000, 0x310000]:
            bad = bytearray(factory)
            bad[index] ^= 1
            with self.subTest(index=index), self.assertRaises(ValueError):
                self.validate(factory=bad, boot_app0=boot_app)
        with self.assertRaises(ValueError):
            self.validate(factory=factory[:-1], boot_app0=boot_app)

    def test_cli_writes_reports_and_fails_closed(self):
        with tempfile.TemporaryDirectory() as directory:
            d = Path(directory)
            for name, data in [('firmware.bin', self.firmware), ('bootloader.bin', self.boot),
                               ('partitions.bin', self.table), ('spiffs.bin', self.fs)]:
                (d / name).write_bytes(data)
            (d / 'build.log').write_text(self.log)
            command = [sys.executable, str(ROOT / 'tools/validate_memory.py'), '--env', self.board['env'],
                       '--build-dir', str(d), '--build-log', str(d / 'build.log'), '--output', str(d / 'memory')]
            subprocess.run(command, check=True, capture_output=True)
            self.assertEqual(json.loads((d / 'memory.json').read_text())['device'], 'LilyGo_TDeck')
            self.assertIn('Statisches RAM', (d / 'memory.md').read_text())
            (d / 'firmware.bin').write_bytes(image(size_code=2))
            self.assertNotEqual(subprocess.run(command, capture_output=True).returncode, 0)

    def test_release_zip_revalidated_after_checksums(self):
        with tempfile.TemporaryDirectory() as directory:
            d = Path(directory)
            for device, board in json.loads(v.BOARDS.read_text()).items():
                name = f'DisAHRsterRadio-MeshCore-0.9.3-{device}'
                boot_app = b'ota'
                factory = bytearray(b'\xff' * (0x310000 + len(self.fs)))
                for off, data in [(0, self.boot), (0x8000, self.table), (0xE000, boot_app),
                                  (0x10000, self.firmware), (0x310000, self.fs)]:
                    factory[off:off + len(data)] = data
                report = dict(device=device, board=board,
                              result=self.validate(board=board, factory=factory, boot_app0=boot_app),
                              sha256={key: hashlib.sha256(data).hexdigest() for key, data in
                                      [('firmware.bin', self.firmware), ('bootloader.bin', self.boot),
                                       ('partitions.bin', self.table), ('spiffs.bin', self.fs)]})
                files = {f'{name}-firmware.bin': self.firmware, f'{name}-factory.bin': factory,
                         'teile/bootloader.bin': self.boot, 'teile/partitions.bin': self.table,
                         'teile/boot_app0.bin': boot_app, 'teile/spiffs.bin': self.fs,
                         'ANLEITUNG.md': b'Version 0.9.3', 'memory-report.md': b'memory',
                         'memory-report.json': json.dumps(report).encode()}

                def write_zip():
                    sums = ''.join(hashlib.sha256(data).hexdigest() + '  ' + key + '\n'
                                   for key, data in files.items())
                    with zipfile.ZipFile(d / (name + '.zip'), 'w', zipfile.ZIP_DEFLATED) as archive:
                        for key, data in {**files, 'SHA256SUMS': sums.encode()}.items():
                            archive.writestr(name + '/' + key, data)

                command = [sys.executable, str(ROOT / 'tools/verify_devices.py'), str(d), '0.9.3', device]
                write_zip()
                subprocess.run(command, check=True, capture_output=True)
                # All ZIP checksums still match: the binary validation must reject this.
                files['teile/bootloader.bin'] = image(size_code=2)
                write_zip()
                bad = subprocess.run(command, capture_output=True, text=True)
                self.assertNotEqual(bad.returncode, 0)
                self.assertIn('header flash size', bad.stdout)
                files['teile/bootloader.bin'] = self.boot
                del files['memory-report.json']
                write_zip()
                self.assertNotEqual(subprocess.run(command, capture_output=True).returncode, 0)

    def test_failed_build_removes_stale_release_zip(self):
        with tempfile.TemporaryDirectory() as directory:
            d = Path(directory)
            fake = d / 'pio'
            fake.write_text('#!/bin/sh\nexit 1\n')
            fake.chmod(0o755)
            archive = d / 'DisAHRsterRadio-MeshCore-0.9.3-LilyGo_TDeck.zip'
            archive.write_bytes(b'stale release')
            result = subprocess.run(['bash', str(ROOT / 'tools/build_device.sh'), 'LilyGo_TDeck', str(d)],
                                    env={**os.environ, 'PATH': str(d) + os.pathsep + os.environ['PATH'],
                                         'VERSION': '0.9.3'}, capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertFalse(archive.exists())


if __name__ == '__main__':
    unittest.main()
