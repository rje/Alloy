#!/usr/bin/env python3
"""PRT-016 bounded independent real-app destination-preservation fixture.

Run only after receiving the sole private GUI lease. Reuses independent STL and
Core 3MF readers from PRT-012; this deliberately does not repeat its mode matrix.
The local API does not expose widget edits or chooser Cancel: those belong to
the focused compiled native-control test, never inferred from this fixture.
"""
import argparse
import hashlib
import json
from pathlib import Path
import time

from app_harness import application
from batch_mesh_export_api import bounds_key, read_mesh


def run(app, out):
    report = {'exports': [], 'refusals': [], 'limits': [
        'Parent/name edits, unsafe leaf-name feedback, chooser launch/Cancel and '
        'exact displayed destination to native Save linkage require compiled-control evidence.',
        'The API opens and captures the native sheet but cannot edit or inspect its widgets.',
        'Physical input, matched reference pixels, 1x/2x and Windows/Linux runtime remain unqualified.']}
    with application(app, out, command_timeout=90) as (call, ledger):
        def checkpoint():
            (out/'partial-ledger.json').write_text(json.dumps(ledger, indent=2)+'\n')

        def unchanged(before, selection):
            after = call('document.inspect')
            for key in ('authored', 'undo', 'revision', 'evaluation'):
                assert after[key] == before[key], key
            assert call('app.state')['selection'] == selection
            checkpoint()

        def export(path, fmt='stl', error=None, selected=None):
            arguments = {'path': str(path), 'bodyIds': ids if selected is None else selected,
                         'structure': 'oneBodyPerFile', 'surfaceDeviationMm': .1,
                         'normalDeviationDeg': 10}
            if fmt == 'stl':
                arguments.update(units='mm', format='binary')
            result = call('document.exportStl' if fmt == 'stl' else 'document.export3mf',
                          arguments, error=error)
            checkpoint()
            return result

        call('window.resize', {'width': 1280, 'height': 900})
        for dimensions, origin, name in (
                ((20, 10, 5), (7, -4, 3), 'Destination coupon A'),
                ((12, 8, 9), (51, 16, 11), 'Destination coupon B')):
            call('solid.box', dict(zip(('width', 'depth', 'height'), dimensions),
                **dict(zip(('x', 'y', 'z'), origin)), units='mm', name=name))
        bodies = call('document.inspect')['evaluation']['bodies']
        assert len(bodies) == 2
        ids = [body['bodyId'] for body in bodies]
        expected = {body['bodyId']: bounds_key(body['boundsMm']) for body in bodies}
        call('selection.set', {'bodyId': ids[0]})
        call('view.home')
        call('document.save', {'path': str(out/'source.alloy')})
        saved_hash = hashlib.sha256((out/'source.alloy').read_bytes()).hexdigest()
        baseline = call('document.inspect')
        selection = call('app.state')['selection']

        # One new parent/name destination per format; exactly the returned files
        # must exist, and each independent mesh reader must find the right box.
        parent = out/'Chosen parent with spaces'
        parent.mkdir()
        for fmt in ('stl', '3mf'):
            directory = parent/f'New batch {fmt}'
            assert not directory.exists()
            result = export(directory, fmt)
            files = result['files']
            assert result['bodyCount'] == len(files) == 2
            assert {row['bodyId'] for row in files} == set(ids)
            assert {p.name for p in directory.iterdir()} == {row['filename'] for row in files}
            meshes = []
            for row in files:
                assert Path(row['path']).parent == directory
                meshes.append(read_mesh(Path(row['path']), fmt, [expected[row['bodyId']]]))
            report['exports'].append({'format': fmt, 'destination': str(directory),
                                      'result': result, 'meshes': meshes})
            unchanged(baseline, selection)

        # Existing file/directory/dangling symlink and missing parent must never
        # create a final folder, replace a sentinel, or alter the CAD document.
        file = parent/'existing-file'; file.write_bytes(b'preserve existing file\n')
        directory = parent/'existing-directory'; directory.mkdir()
        sentinel = directory/'sentinel'; sentinel.write_bytes(b'preserve existing directory\n')
        link = parent/'existing-symlink'; link.symlink_to(parent/'absent-link-target', target_is_directory=True)
        missing = parent/'missing-parent'/'new-batch'
        for path, code in ((file, 'destination_exists'), (directory, 'destination_exists'),
                           (link, 'destination_exists'), (missing, 'invalid_path')):
            export(path, error=code)
            report['refusals'].append({'path': str(path), 'error': code})
            unchanged(baseline, selection)
        assert file.read_bytes() == b'preserve existing file\n'
        assert list(directory.iterdir()) == [sentinel]
        assert sentinel.read_bytes() == b'preserve existing directory\n'
        assert link.is_symlink() and not link.exists()
        assert not missing.parent.exists()
        absent = parent/'stale-selection-must-not-create'
        export(absent, error='stale_body', selected=[ids[0], 'missing-body'])
        call('document.exportMeshDialog', {'bodyIds': [ids[0], 'missing-body']}, error='stale_body')
        assert not absent.exists()
        assert not list(parent.glob('.alloy-mesh-*'))
        unchanged(baseline, selection)

        # Export and its refusals do not consume the latest authored transaction.
        call('history.undo')
        assert len(call('document.inspect')['evaluation']['bodies']) == 1
        call('history.redo')
        restored = call('document.inspect')
        assert restored['authored'] == baseline['authored']
        # Undo/Redo intentionally advances documentRevision; compare all
        # geometry/history evaluation fields rather than the revision counter.
        def geometry(evaluation):
            return {key: value for key, value in evaluation.items() if key != 'documentRevision'}
        assert geometry(restored['evaluation']) == geometry(baseline['evaluation'])
        assert hashlib.sha256((out/'source.alloy').read_bytes()).hexdigest() == saved_hash
        report['historyUndoRedo'] = 'latest Box undone/restored; saved bytes unchanged'

        call('capture.screenshot', {'path': str(out/'source-solids.png')})
        report['environment'] = call('app.state')
        assert report['environment']['viewport']['gpu']['hardwareAccelerated']
        before_sheet = call('document.inspect')
        sheet_selection = call('app.state')['selection']
        call('document.exportMeshDialog', {'bodyIds': ids})
        time.sleep(.15)
        call('capture.dialog', {'path': str(out/'native-selected-set-sheet.png')})
        call('capture.screenshot', {'path': str(out/'native-selected-set-full-window.png')})
        unchanged(before_sheet, sheet_selection)
        report['nativeSheet'] = 'Opened through shared API with explicit two-body selection; default structure only.'
        report.update(passed=True, ledger=ledger)
        (out/'receipt.json').write_text(json.dumps(report, indent=2)+'\n')
    print('PASS two-file STL/3MF readback, destination/stale refusal, history and native-sheet capture')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    output = Path(args.output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    run(args.app, output)
