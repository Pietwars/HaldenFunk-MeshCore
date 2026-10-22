import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from prepare_standalone_candidate import build_factory, verify


class CandidatePackageTest(unittest.TestCase):
    def test_factory_preserves_components_and_erased_padding(self):
        files = {'bootloader.bin': b'BOOT', 'partitions.bin': b'PART',
                 'boot_app0.bin': b'OTA', 'firmware.bin': b'FIRMWARE'}
        table = [dict(type=1, subtype=0, offset=0xe000, size=8192)]
        factory = build_factory(files, table)
        self.assertEqual(len(factory), 0x10008)
        self.assertEqual(factory[:4], b'BOOT')
        self.assertEqual(factory[4:0x8000], b'\xff' * (0x8000-4))
        self.assertEqual(factory[0x8000:0x8004], b'PART')
        self.assertEqual(factory[0xe000:0xe003], b'OTA')
        self.assertEqual(factory[0x10000:], b'FIRMWARE')
        with self.assertRaises(ValueError):
            build_factory(files, [])
        files['bootloader.bin'] = b'x' * 0x8001
        with self.assertRaises(ValueError):
            build_factory(files, table)

    def test_package_detects_changed_unlisted_and_falsely_approved_files(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'candidate.zip'
            def package(approve=False, corrupt=False, extra=False):
                payload = b'TEST'
                manifest = dict(hardware_status='pending', publication_approved=approve,
                                sha256={'firmware.bin': hashlib.sha256(payload).hexdigest()})
                data = {'firmware.bin': payload, 'candidate.json': json.dumps(manifest).encode()}
                sums = ''.join(hashlib.sha256(content).hexdigest()+'  '+name+'\n'
                               for name, content in data.items()).encode()
                if corrupt:
                    data['firmware.bin'] = b'CHANGED'
                if extra:
                    data['unlisted.bin'] = b'EXTRA'
                data['SHA256SUMS'] = sums
                with zipfile.ZipFile(path, 'w') as archive:
                    for name, content in data.items():
                        archive.writestr('candidate/'+name, content)
            package()
            self.assertFalse(verify(path)['publication_approved'])
            package(approve=True)
            self.assertTrue(verify(path, release=True)['publication_approved'])
            for options in ({'approve': True}, {'corrupt': True}, {'extra': True}):
                package(**options)
                with self.assertRaises(ValueError):
                    verify(path)


if __name__ == '__main__':
    unittest.main()
