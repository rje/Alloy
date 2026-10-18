#!/usr/bin/env python3
"""Focused contract failures; no build, editor, or real archive mutation."""
import json
from pathlib import Path
import tempfile
import unittest
import zipfile

from runtime_inventory import SCHEMA, digest, file_record, reference, sha, validate


class InventoryContractTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.base = Path(self.temp.name)
        self.root = self.base / 'Package'
        self.root.mkdir()
        (self.root / 'app').write_bytes(b'compiled fixture')
        (self.root / 'LICENSE').write_text('fixture license')
        self.archive = self.base / 'package.zip'
        with zipfile.ZipFile(self.archive, 'w') as zipped:
            for path in self.root.iterdir():
                zipped.write(path, 'Package/' + path.name)
        self.evidence_path = self.base / 'evidence.json'
        self.evidence = {'source': {'version': 'fixture-1', 'source_url': 'https://example.org/source'},
                         'loaded': [str(self.root / 'app')], 'outside': []}
        self.evidence_path.write_text(json.dumps(self.evidence))
        self.manifest = {
            'schema': SCHEMA, 'target': 'linux-x64', 'archive_sha256': sha(self.archive),
            'archive_package_root': 'Package', 'observed_package_root': str(self.root),
            'evidence': {'fixture': {'path': str(self.evidence_path), 'sha256': sha(self.evidence_path)}},
            'load_observation': {'paths': reference('fixture', '/loaded'), 'outside': reference('fixture', '/outside')},
            'components': [{'id': 'fixture', 'role': 'executable', 'source': reference('fixture', '/source'),
                            'notices': ['LICENSE'], 'built_input': {'status': 'unavailable', 'reason': 'fixture'},
                            'files': [file_record(self.root, name) for name in ('app', 'LICENSE')],
                            'runtime': {'status': 'observed', 'paths': self.evidence['loaded']}}],
            'limits': ['Synthetic contract fixture; no platform qualification.']}

    def check(self, pattern):
        with self.assertRaisesRegex((ValueError, KeyError), pattern):
            validate(self.manifest, self.root, self.archive)

    def test_valid_join(self):
        self.assertEqual(validate(self.manifest, self.root, self.archive)['observed_images'], 1)

    def test_missing_packaged_entry(self):
        self.manifest['components'][0]['files'].pop()
        self.check('coverage mismatch')

    def test_missing_required_component(self):
        self.manifest['target'] = 'macos-arm64'
        self.check('Missing required components')

    def test_changed_packaged_bytes(self):
        (self.root / 'app').write_bytes(b'changed')
        self.check('Packaged hash/kind mismatch: app')

    def test_changed_archive(self):
        self.archive.write_bytes(b'changed')
        self.check('Archive SHA-256 mismatch')

    def test_archive_extraction_join(self):
        # Even a manifest updated to bless the altered extraction fails the ZIP join.
        (self.root / 'app').write_bytes(b'changed')
        self.manifest['components'][0]['files'][0] = file_record(self.root, 'app')
        self.check('Archive/extracted hash mismatch: app')

    def test_changed_evidence(self):
        self.evidence_path.write_text('{}')
        self.check('Evidence SHA-256 mismatch: fixture')

    def test_unresolved_source(self):
        self.manifest['components'][0]['source']['evidence'] = 'missing'
        self.check('Unresolved evidence link: missing')

    def test_unresolved_notice(self):
        self.manifest['components'][0]['notices'] = ['absent']
        self.check('Unresolved notice: fixture: absent')

    def test_unresolved_build_input(self):
        self.manifest['components'][0]['built_input'] = {'status': 'unknown'}
        self.check('Unresolved built-input provenance')

    def test_external_loaded_path(self):
        self.manifest['components'][0]['runtime']['paths'] = ['/opt/developer/lib.so']
        self.check('not in the subpath|is not in the subpath')

    def test_external_package_symlink(self):
        (self.root / 'escape').symlink_to(self.evidence_path)
        self.manifest['components'][0]['files'].append(
            {'path': 'escape', 'kind': 'symlink', 'sha256': digest(str(self.evidence_path).encode())})
        self.check('External package symlink: escape')

    def test_incomplete_load_summary(self):
        self.manifest['components'][0]['runtime'] = {'status': 'not-observed', 'reason': 'not seen'}
        self.check('Loaded runtime coverage mismatch')


if __name__ == '__main__':
    unittest.main()
