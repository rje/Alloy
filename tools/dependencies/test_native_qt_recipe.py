#!/usr/bin/env python3
"""Small cache-integrity tests; these do not substitute for native SDK builds."""
# SPDX-License-Identifier: MIT
import tempfile
from pathlib import Path
import unittest

from build_native_qt import inventory, verify
from private_qt_recipe import sha, tree_sha


class CacheAdmission(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.work = Path(self.temporary.name)
        for name in ('source/input.cpp', 'install/plugins/platforms/native.plugin',
                     'build/CMakeCache.txt', 'cache-test-evidence/result.xml'):
            path = self.work / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(name)
        self.manifest = {'recipe_key': 'accepted', 'status': 'complete',
                         'source_tree': tree_sha(self.work / 'source'),
                         'installed_files_sha256': inventory(self.work / 'install'),
                         'cmake_cache_sha256': sha(self.work / 'build/CMakeCache.txt'),
                         'regression_evidence_sha256': inventory(self.work / 'cache-test-evidence')}

    def test_complete_unchanged_cache_is_admitted(self):
        verify(self.work, self.manifest, 'accepted')

    def test_changed_recipe_is_rejected(self):
        with self.assertRaisesRegex(RuntimeError, 'recipe mismatch'):
            verify(self.work, self.manifest, 'different')

    def test_incomplete_stamp_is_rejected(self):
        self.manifest['status'] = 'building'
        with self.assertRaisesRegex(RuntimeError, 'recipe mismatch'):
            verify(self.work, self.manifest, 'accepted')

    def test_changed_source_plugin_configuration_or_regression_is_rejected(self):
        for name in ('source/input.cpp', 'install/plugins/platforms/native.plugin',
                     'build/CMakeCache.txt', 'cache-test-evidence/result.xml'):
            with self.subTest(name=name):
                path = self.work / name
                original = path.read_text()
                path.write_text('tampered')
                with self.assertRaisesRegex(RuntimeError, 'changed'):
                    verify(self.work, self.manifest, 'accepted')
                path.write_text(original)


if __name__ == '__main__':
    unittest.main()
