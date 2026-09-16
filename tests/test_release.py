"""Exercise archive rejection before a release is handed to players."""
import importlib.util
import io
import stat
import json
import tempfile
from unittest.mock import patch
import unittest
from pathlib import Path
from zipfile import ZipFile, ZipInfo

spec = importlib.util.spec_from_file_location('release', Path(__file__).resolve().parents[1] / 'scripts/package-release.py')
release = importlib.util.module_from_spec(spec)
spec.loader.exec_module(release)

class ReleaseArchiveTests(unittest.TestCase):
    def check_names(self, names, valid=False):
        buffer = io.BytesIO()
        with ZipFile(buffer, 'w') as z:
            for name in names:
                z.writestr(name, b'content')
        buffer.seek(0)
        with ZipFile(buffer) as z:
            if valid:
                self.assertEqual(len(release.safe_files(z)), len(names))
            else:
                with self.assertRaises(SystemExit):
                    release.safe_files(z)

    def test_normal_package(self):
        self.check_names(['TeleportLogistics.uplugin', 'Content/Paks/Windows/TeleportLogistics.ucas'], valid=True)

    def test_traversal_and_windows_paths(self):
        for name in ('../outside', '/absolute', 'C:/outside', r'Content\..\outside'):
            with self.subTest(name=name):
                self.check_names([name])

    def test_case_collisions(self):
        self.check_names(['TeleportLogistics.uplugin', 'teleportlogistics.uplugin'])

    def test_manifest_identity_must_match_source(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            (root / 'TeleportLogistics').mkdir()
            expected = {'SemVersion': '0.5.9', 'Version': 29, 'VersionName': '0.5.9',
                        'FriendlyName': 'TeleportLogistics', 'GameVersion': '>=502094',
                        'Plugins': [{'Name': 'SML'}], 'Modules': [{'Name': 'TeleportLogistics'}]}
            (root / 'TeleportLogistics/TeleportLogistics.uplugin').write_text(json.dumps(expected))
            archive = root / 'release.zip'
            def write(manifest):
                with ZipFile(archive, 'w') as z:
                    z.writestr('TeleportLogistics.uplugin', json.dumps(manifest))
            with patch.object(release, 'ROOT', root):
                write(expected)
                release.verify_archive(archive, '0.5.9', ('TeleportLogistics.uplugin',))
                for key in ('SemVersion', 'Version', 'VersionName', 'FriendlyName', 'GameVersion',
                            'Plugins', 'Modules'):
                    with self.subTest(key=key):
                        write({**expected, key: 'stale'})
                        with self.assertRaises(SystemExit):
                            release.verify_archive(archive, '0.5.9', ('TeleportLogistics.uplugin',))
                write(expected)
                with self.assertRaises(SystemExit):
                    release.verify_archive(archive, '0.5.9', ('missing.dll',))

    def test_symlink(self):
        entry = ZipInfo('link')
        entry.create_system = 3
        entry.external_attr = (stat.S_IFLNK | 0o777) << 16
        self.check_names([entry])

if __name__ == '__main__':
    unittest.main()
