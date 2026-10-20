#!/usr/bin/env python3
"""PRT-012 independent real-app selected-set/batch transaction verification.

Run only with the sole private GUI lease. STL facets and Core 3MF ZIP/XML
readers are independent of application output metadata. The fixture deliberately
uses unequal, separated solids and a distant excluded third solid.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import time

from app_harness import application
from alloyctl import request
from stl_export_api import inspect_stl
from three_mf_export_api import inspect_3mf


def bounds_key(bounds):
    return tuple(bounds['min']) + tuple(bounds['max'])


def compare_bounds(actual, expected):
    assert len(actual) == len(expected), (actual, expected)
    for a, b in zip(sorted(actual), sorted(expected)):
        assert all(abs(x-y) < 1e-4 for x, y in zip(a, b)), (a, b)


def read_mesh(path, fmt, expected):
    if fmt == 'stl':
        mesh = inspect_stl(path, True)
        actual = [tuple(c['bounds'][0])+tuple(c['bounds'][1]) for c in mesh['components']]
    else:
        mesh = inspect_3mf(path)
        assert mesh['objects'] == len(expected)
        actual = [bounds_key(i['boundsMm']) for i in mesh['buildItems']]
    compare_bounds(actual, expected)
    expected_volume = sum((b[3]-b[0])*(b[4]-b[1])*(b[5]-b[2]) for b in expected)
    triangles = mesh['triangles'] if fmt == 'stl' else sum(i['triangles'] for i in mesh['buildItems'])
    volume = mesh['signedVolume'] if fmt == 'stl' else sum(i['volumeMm3'] for i in mesh['buildItems'])
    assert triangles == 12*len(expected), triangles
    assert abs(volume-expected_volume) < 1e-3, (volume, expected_volume)
    return {'reader': mesh, 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}


def run(app, out):
    reports, refusals = [], []
    with application(app, out, command_timeout=90) as (call, ledger):
        def raw(command, parameters=None):
            response = request(call.endpoint, command, parameters or {}, timeout=90,
                               transport=call.transport)
            ledger.append({'command': command, 'args': parameters or {}, 'response': response})
            (out/'partial-ledger.json').write_text(json.dumps(ledger, indent=2)+'\n')
            return response

        def export(parameters, fmt='stl', refused=None):
            selection = call('app.state')['selection']
            command = 'document.exportStl' if fmt == 'stl' else 'document.export3mf'
            if fmt == 'stl':
                parameters = dict(parameters, units='mm', format='binary')
            response = raw(command, dict(parameters, surfaceDeviationMm=.1, normalDeviationDeg=10))
            deadline = time.monotonic()+90
            initial = response.get('result', {})
            while response.get('completion') == 'pending':
                assert time.monotonic() < deadline, response
                time.sleep(.025)
                response = raw('command.status', {'session': initial['id'],
                    'generation': initial['previewGeneration']})
            if refused:
                assert not response['ok'] and response['error']['code'] == refused, response
                refusals.append(response)
            else:
                assert response['ok'], response
            assert call('app.state')['selection'] == selection
            return {**initial, **response.get('result', {})}

        def unchanged(before):
            after = call('document.inspect')
            for key in ('authored', 'undo', 'revision', 'evaluation'):
                assert after[key] == before[key], key
            (out/'partial-readback.json').write_text(json.dumps(reports, indent=2)+'\n')

        call('window.resize', {'width':1280, 'height':900})
        for dimensions, origin, name in (
                ((20,10,5), (7,-4,3), 'Coupon/?'),
                ((12,8,9), (51,16,11), 'Coupon/?'),
                ((4,6,8), (170,80,30), 'Excluded')):
            call('solid.box', dict(zip(('width','depth','height'), dimensions),
                **dict(zip(('x','y','z'), origin)), units='mm', name=name))
        source = call('document.inspect')
        bodies = source['evaluation']['bodies']
        ids = [b['bodyId'] for b in bodies[:2]]
        expected = {b['bodyId']: bounds_key(b['boundsMm']) for b in bodies[:2]}
        call('selection.set', {'bodyId':ids[0]})
        call('view.home')
        call('document.save', {'path':str(out/'source.alloy')})
        baseline = call('document.inspect')
        for fmt in ('stl', '3mf'):
            combined = out/('combined.'+fmt)
            result = export({'path':str(combined), 'bodyIds':ids, 'structure':'oneFile'}, fmt)
            mesh = read_mesh(combined, fmt, list(expected.values()))
            assert result['bodyCount'] == 2 and result['triangles'] == (mesh['reader']['triangles']
                if fmt == 'stl' else sum(i['triangles'] for i in mesh['reader']['buildItems']))
            reports.append({'mode':'oneFile', 'format':fmt, 'result':result, 'mesh':mesh})
            directory = out/f'{fmt}-a'/'Batch'
            directory.parent.mkdir()
            result = export({'path':str(directory), 'bodyIds':ids, 'structure':'oneBodyPerFile'}, fmt)
            files = result['files']
            assert result['bodyCount'] == len(files) == 2
            assert {f['bodyId'] for f in files} == set(ids)
            assert {p.name for p in directory.iterdir()} == {f['filename'] for f in files}
            assert len({f['filename'].casefold() for f in files}) == 2
            artifacts = []
            for row in files:
                assert re.fullmatch(r'[A-Za-z0-9_]+\.'+fmt, row['filename']), row
                artifacts.append(read_mesh(Path(row['path']), fmt, [expected[row['bodyId']]]))
            reports.append({'mode':'oneBodyPerFile', 'format':fmt, 'result':result, 'meshes':artifacts})
            # Same stem, reversed explicit selection: filename-to-instance identity
            # must be deterministic, independent of caller selection order.
            repeat = out/f'{fmt}-b'/'Batch'
            repeat.parent.mkdir()
            again = export({'path':str(repeat), 'bodyIds':list(reversed(ids)), 'structure':'oneBodyPerFile'}, fmt)
            assert [(r['bodyId'],r['filename']) for r in files] == [(r['bodyId'],r['filename']) for r in again['files']]
            for row in again['files']:
                read_mesh(Path(row['path']), fmt, [expected[row['bodyId']]])
            unchanged(baseline)
        # Case-only/unsafe-name normalization must not merge distinct members.
        call('body.rename', {'bodyId':ids[1], 'name':'coupon:?'})
        renamed = call('document.inspect')
        result = export({'path':str(out/'CaseNames'), 'bodyIds':ids, 'structure':'oneBodyPerFile'})
        assert len({r['filename'].casefold() for r in result['files']}) == 2
        for row in result['files']:
            read_mesh(Path(row['path']), 'stl', [expected[row['bodyId']]])
        unchanged(renamed)
        # Existing file, directory and dangling symlink are all preserved.
        blockers = [out/'existing-file', out/'existing-dir', out/'existing-link']
        blockers[0].write_bytes(b'file sentinel')
        blockers[1].mkdir(); (blockers[1]/'sentinel').write_bytes(b'directory sentinel')
        blockers[2].symlink_to(out/'absent-link-target', target_is_directory=True)
        for path in blockers:
            export({'path':str(path), 'bodyIds':ids, 'structure':'oneBodyPerFile'}, refused='destination_exists')
            unchanged(renamed)
        assert blockers[0].read_bytes() == b'file sentinel'
        assert list(blockers[1].iterdir()) == [blockers[1]/'sentinel']
        assert (blockers[1]/'sentinel').read_bytes() == b'directory sentinel'
        assert blockers[2].is_symlink() and not blockers[2].exists()
        absent = out/'must-stay-absent'
        for selected, code in (([], 'invalid_body_selection'), ([ids[0],ids[0]], 'invalid_body_selection'),
                               ([ids[0],'missing'], 'stale_body')):
            export({'path':str(absent), 'bodyIds':selected, 'structure':'oneBodyPerFile'}, refused=code)
        assert not absent.exists()
        unchanged(renamed)
        # Immediate raw request observes pending state; harness auto-wait would
        # otherwise conceal the cancellation window before publication approval.
        selection_before_cancel = call('app.state')['selection']
        started = raw('document.exportStl', {'path':str(absent), 'bodyIds':ids,
            'structure':'oneBodyPerFile', 'format':'binary', 'units':'mm',
            'surfaceDeviationMm':.1, 'normalDeviationDeg':10})
        assert started['ok'] and started['completion'] == 'pending', started
        operation = started['result']
        cancelled = raw('command.cancel', {'session':operation['id']})
        late_cancel = not cancelled['ok']
        if late_cancel:
            assert cancelled['error']['code'] == 'export_publication_started', cancelled
        deadline = time.monotonic()+20
        while call('app.state')['geometryWork']['active']:
            assert time.monotonic() < deadline
            time.sleep(.025)
        terminal = raw('command.status', {'session':operation['id'], 'generation':operation['previewGeneration']})
        if late_cancel:
            assert terminal['ok'] and terminal['completion'] == 'completed', terminal
            files = terminal['result']['files']
            assert len(files) == 2 and {p.name for p in absent.iterdir()} == {r['filename'] for r in files}
            for row in files:
                read_mesh(Path(row['path']), 'stl', [expected[row['bodyId']]])
        else:
            assert not terminal['ok'] and terminal['error']['code'] == 'export_cancelled', terminal
            assert not absent.exists()
        assert call('app.state')['selection'] == selection_before_cancel
        assert not list(out.glob('.alloy-mesh-*'))
        unchanged(renamed)
        # Export did not consume the latest authored undo transaction.
        call('history.undo'); assert call('document.inspect')['authored'] == baseline['authored']
        call('history.redo'); assert call('document.inspect')['authored'] == renamed['authored']
        # A bounded additional per-body 3MF checks a genuinely nonidentity
        # occurrence transform, where double-applying placement is material.
        root = renamed['authored']['documentId']
        component = call('component.create', {'name':'Placed coupon'})
        call('component.reparent', {'bodyId':ids[0], 'destinationId':component['definitionId']})
        copied = call('component.copyOccurrence', {'occurrenceId':component['occurrenceId'],
            'placement':{'axis':'Z', 'angleDeg':90, 'translationMm':[90,20,7]}})
        occurrence = copied['rootOccurrenceId']
        placed = next(r for r in call('component.inspect')['localInstances'] if r['occurrenceId'] == occurrence)
        placement_baseline = call('document.inspect')
        transformed = export({'path':str(out/'PlacedBatch'), 'occurrenceId':occurrence,
            'structure':'oneBodyPerFile'}, '3mf')
        assert transformed['bodyCount'] == len(transformed['files']) == 1
        transformed_mesh = read_mesh(Path(transformed['files'][0]['path']), '3mf',
            [bounds_key(placed['bodies'][0]['boundsMm'])])
        assert any(abs(x) > 1 for x in transformed_mesh['reader']['buildItems'][0]['transform'][9:])
        unchanged(placement_baseline)
        reports.append({'mode':'placedOccurrenceOneBodyPerFile', 'result':transformed, 'mesh':transformed_mesh})
        call('component.activate', {'definitionId':root})
        for _ in range(3):
            call('history.undo')
        assert call('document.inspect')['authored'] == renamed['authored']
        call('capture.screenshot', {'path':str(out/'source-solids.png')})
        state = call('app.state')
        assert state['viewport']['gpu']['hardwareAccelerated'], state['viewport']['gpu']
        call('document.exportMeshDialog', {'bodyIds':ids})
        time.sleep(.15)
        call('capture.dialog', {'path':str(out/'native-selected-set-sheet.png')})
        call('capture.screenshot', {'path':str(out/'native-selected-set-full-window.png')})
        (out/'receipt.json').write_text(json.dumps({'passed':True, 'exports':reports,
            'nameCaseResult':result, 'refusals':refusals, 'cancelled':terminal,
            'cancelRequest':cancelled, 'lateCancelBoundary':late_cancel, 'environment':state,
            'limits':['Physical chooser/input, matched public pixels, 1x/2x and other-platform runtime unqualified.',
                      'Late staged-body failure injection is a focused core-test gate, not exercised by this API fixture.',
                      'Tiny-box pending Cancel may reach publication approval before the next request; explicit late refusal requires complete independent output readback. Preapproval Cancel is separately qualified by the focused core/native test.',
                      'Four selected two-body combinations use unequal world-coordinate solids; an extra per-body 3MF checks a rotated/translated occurrence.'],
            'ledger':ledger}, indent=2)+'\n')
    print('PASS selected-set one-file/per-body STL/Core3MF readback, names, refusal, cancel and history')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    output = Path(args.output).resolve(); output.mkdir(parents=True, exist_ok=True)
    run(args.app, output)
