#!/usr/bin/env python3
"""REL-055: failed existing-file replacement through the real local API (Unix)."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import tempfile

from app_harness import application


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    if os.name != 'posix':
        parser.error('This bounded permission fixture requires Unix permissions.')
    output = Path(args.output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='alloy-failed-save-') as directory:
        parent = Path(directory)
        native = parent / 'existing.alloy'
        with application(args.app, output) as (call, transcript):
            call('solid.box', {'width': 20, 'depth': 30, 'height': 10, 'units': 'mm'})
            call('document.save', {'path': str(native)})
            original = call('document.inspect')
            original_bytes = native.read_bytes()
            previous = output / 'previous.alloy'
            previous.write_bytes(original_bytes)
            feature_id = original['authored']['features'][0]['id']
            call('feature.edit', {'featureId': feature_id, 'parameters': {'widthMm': 24}})
            call('document.units', {'units': 'in'})
            call('history.undo')
            dirty = call('document.inspect')
            assert dirty['dirty'] and dirty['undo']['canUndo'] and dirty['undo']['canRedo']
            permissions = parent.stat().st_mode & 0o777
            try:
                # Existing file stays writable so the fixture detects accidental
                # QSaveFile direct-write fallback as well as destructive saves.
                parent.chmod(0o500)
                probe = parent / 'creation-probe'
                try:
                    probe.write_bytes(b'')
                except PermissionError:
                    pass
                else:
                    raise AssertionError('Directory permissions are not enforced; do not run as root')
                call('document.save', {'path': str(native)}, 'save_failed')
                failure = transcript[-1]['response']
                assert failure['ok'] is False and failure['completion'] == 'failed' and 'result' not in failure
                assert failure['error']['field'] == 'path' and failure['error']['message']
                assert call('document.inspect') == dirty
                assert native.read_bytes() == original_bytes
            finally:
                parent.chmod(permissions)
            call('document.save', {'path': str(native)})
            saved = call('document.inspect')
            assert not saved['dirty'] and saved['authored'] == dirty['authored']
            assert saved['undo'] == dirty['undo'] and saved['revision'] == dirty['revision']
            assert json.loads(native.read_text()) == dirty['authored']
            (output / 'retry.alloy').write_bytes(native.read_bytes())
            call('history.undo')
            assert call('document.inspect')['authored'] == original['authored']
            call('history.redo')
            assert call('document.inspect')['authored'] == saved['authored']
            assert not call('document.inspect')['dirty']
            call('document.open', {'path': str(previous), 'discard': True})
            reopened = call('document.inspect')
            assert reopened['authored'] == original['authored']
            assert abs(reopened['evaluation']['bodies'][0]['volumeMm3'] - 6000) < 1e-7
            call('document.open', {'path': str(native), 'discard': True})
            assert call('document.inspect')['authored'] == saved['authored']
            result = {
                'result': 'passed', 'issue': 'REL-055',
                'failure': failure,
                'previousSha256': hashlib.sha256(original_bytes).hexdigest(),
                'retrySha256': hashlib.sha256(native.read_bytes()).hexdigest(),
                'transcript': transcript,
                'limits': ['Unix directory-permission failure only; no interrupted write, disk-full, migration or native chooser claim'],
            }
            (output / 'failed-save-api.json').write_text(json.dumps(result, indent=2) + '\n')
    print('PASS: failed replacement preserves prior bytes, dirty state, IDs and history; retry/reopen pass')


if __name__ == '__main__':
    main()
