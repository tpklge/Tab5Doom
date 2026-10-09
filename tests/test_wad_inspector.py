#!/usr/bin/env python3
"""Check real WAD signatures, directory bounds and extended-map detection."""
import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest

spec = importlib.util.spec_from_file_location(
    'inspector', Path(__file__).resolve().parents[1] / 'tools/inspect_chex_wad.py')
inspector = importlib.util.module_from_spec(spec)
spec.loader.exec_module(inspector)


class InspectorTests(unittest.TestCase):
    def check_wad(self, data):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / 'game.wad'
            path.write_bytes(data)
            return inspector.inspect(path)

    def wad(self, magic, names):
        directory = b''.join(struct.pack('<ii8s', 12, 0, name.encode())
                             for name in names)
        return struct.pack('<4sii', magic, len(names), 12) + directory

    def test_pwad_is_valid(self):
        report = self.check_wad(self.wad(b'PWAD', ['E1M1', 'THINGS']))
        self.assertEqual(report['signature'], 'PWAD')
        self.assertFalse(report['requires_extended_engine'])

    def test_hexen_map_and_scripts(self):
        report = self.check_wad(self.wad(b'IWAD', ['E1M1', 'BEHAVIOR',
                                                   'E1M2', 'THINGS', 'DECORATE']))
        self.assertTrue(report['requires_extended_engine'])
        self.assertEqual([m['behavior'] for m in report['map_markers']], [True, False])

    def test_truncated_header(self):
        with self.assertRaises(ValueError):
            self.check_wad(b'PWAD')

    def test_truncated_directory(self):
        with self.assertRaises(ValueError):
            self.check_wad(struct.pack('<4sii', b'PWAD', 2, 12))

    def test_lump_outside_file(self):
        with self.assertRaises(ValueError):
            self.check_wad(struct.pack('<4sii', b'PWAD', 1, 12)
                           + struct.pack('<ii8s', 1000, 5, b'THINGS'))


if __name__ == '__main__':
    unittest.main()
