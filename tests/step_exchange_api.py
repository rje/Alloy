#!/usr/bin/env python3
"""Independent STEP fixture, real-app import/export, and separate OCCT readback."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import subprocess
from app_harness import application


def independent(tool, *arguments):
    completed = subprocess.run([str(tool), *map(str, arguments)], check=True,
                               capture_output=True, text=True)
    return json.loads(completed.stdout.splitlines()[-1])


def check_body(body, fixture):
    assert body['valid'] and body['closed'], body
    expected = math.prod(fixture['boxSizeMm'])
    assert math.isclose(body['volumeMm3'], expected, rel_tol=1e-8, abs_tol=1e-4), body
    for actual, target in zip(body['boundsMm']['size'], fixture['boxSizeMm']):
        assert math.isclose(actual, target, rel_tol=1e-8, abs_tol=1e-5), body
    for actual, target in zip(body['boundsMm']['min'], (0, 0, 0)):
        assert math.isclose(actual, target, rel_tol=1e-8, abs_tol=1e-5), body
    for actual, target in zip(body['boundsMm']['max'], fixture['boxSizeMm']):
        assert math.isclose(actual, target, rel_tol=1e-8, abs_tol=1e-5), body


def check_independent(body, fixture):
    assert body['valid'] and body['solidCount'] == 1, body
    assert math.isclose(body['volumeMm3'], math.prod(fixture['boxSizeMm']), rel_tol=1e-8, abs_tol=1e-4), body
    for actual, target in zip(body['boundsMm'], [0, 0, 0, *fixture['boxSizeMm']]):
        assert math.isclose(actual, target, rel_tol=1e-8, abs_tol=1e-5), body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('app', 'output', 'fixture'):
        parser.add_argument('--' + name, required=True)
    parser.add_argument('--step-tool')
    args = parser.parse_args()
    output = Path(args.output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    fixture_path = Path(args.fixture)
    fixture = json.loads(fixture_path.read_text())
    app_path = Path(args.app).resolve()
    tool = Path(args.step_tool).resolve() if args.step_tool else (
        app_path.parents[3] / 'step_fixture' if '.app' in str(app_path) else app_path.parent / 'step_fixture')
    source = output / 'independent-source.step'
    independent(tool, 'write', source, fixture['sourceUnits'], 'single')
    independently_read = independent(tool, 'read', source)
    check_independent(independently_read, fixture)
    reader_results = {'source': independently_read}
    malformed = output / 'malformed.step'
    malformed.write_bytes(b'not a STEP model')
    multiple = output / 'two-solids.step'
    independent(tool, 'write', multiple, 'mm', 'multi')

    with application(args.app, output) as (call, transcript):
        initial = call('app.state')
        assert initial['viewport']['gpu']['hardwareAccelerated']
        original = call('document.inspect')
        for path in (malformed, multiple):
            call('document.importStep', {'path': str(path)}, 'step_import_failed')
            assert call('document.inspect') == original
        imported = call('document.importStep', {'path': str(source)})
        assert imported['bodies'] == 1 and imported['undo']['count'] == original['undo']['count'] + 1
        feature = imported['authored']['features'][-1]
        assert feature['type'] == 'stepImport'
        source_record = feature['parameters']['source']
        assert source_record['policy'] == 'embedded'
        assert source_record['sha256'] == hashlib.sha256(source.read_bytes()).hexdigest()
        assert source_record['byteCount'] == source.stat().st_size
        body = imported['evaluation']['bodies'][0]
        assert body['sourcePolicy'] == 'embedded'
        assert body['sourceLengthUnits']
        check_body(body, fixture)
        call('feature.editDialog', {'featureId': feature['id']}, 'import_source_readonly')
        assert call('document.inspect') == imported
        call('history.undo')
        assert call('document.inspect')['authored'] == original['authored']
        call('history.redo')
        assert call('document.inspect')['authored'] == imported['authored']
        restored = call('document.inspect')
        for unit in fixture['exportUnits']:
            path = output / f'export-{unit}.step'
            before = call('document.inspect')
            result = call('document.exportStep', {'path': str(path), 'bodyId': body['bodyId'], 'units': unit})
            assert result['authoredChanged'] is False and result['sha256'] == hashlib.sha256(path.read_bytes()).hexdigest()
            assert call('document.inspect') == before
            independent_body = independent(tool, 'read', path)
            check_independent(independent_body, fixture)
            reader_results[unit] = independent_body
            declared = {'in': ('INCH',), 'mm': ('MM', 'MILLIMETRE', 'MILLIMETER')}[unit]
            assert any(value.upper() in declared for value in independent_body['fileLengthUnits']), independent_body
        call('document.exportStep', {'path': str(output / 'bad.txt'), 'bodyId': body['bodyId'], 'units': 'mm'}, 'invalid_path')
        blocked = output / 'directory.step'
        blocked.mkdir()
        call('document.exportStep', {'path': str(blocked), 'bodyId': body['bodyId'], 'units': 'mm'}, 'export_failed')
        assert call('document.inspect') == restored
        saved = output / 'embedded-import.alloy'
        call('document.save', {'path': str(saved)})
        assert source.unlink() is None
        call('document.new', {'discard': False})
        call('document.open', {'path': str(saved)})
        reopened = call('document.inspect')
        assert reopened['authored'] == imported['authored']
        check_body(reopened['evaluation']['bodies'][0], fixture)
        assert reopened['evaluation']['bodies'][0]['sourcePolicy'] == 'embedded'
    (output / 'step-exchange-api.json').write_text(json.dumps({
        'result': 'passed', 'fixtureSha256': hashlib.sha256(fixture_path.read_bytes()).hexdigest(),
        'initialState': initial, 'transcript': transcript,
        'independentReadback': reader_results,
        'verifiedCapabilities': ['exchange.step_import', 'exchange.step_export', 'exchange.step_embedded_source',
                                 'exchange.step_independent_roundtrip'],
        'evidenceDimensions': {'api': {'status': 'passed'}, 'geometry': {'status': 'passed'},
                               'save_reopen': {'status': 'passed'}, 'export': {'status': 'passed'},
                               'physical_input': {'status': 'not_run', 'reason': 'Native File dialog remains for separate review.'}}
    }, indent=2) + '\n')
    print('PASS: independent STEP fixture, mm/in readback, atomic rejection, embedded reopen, history')


if __name__ == '__main__':
    main()
