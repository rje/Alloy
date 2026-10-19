#!/usr/bin/env python3
"""Real-app mm/in sketch, feature-edit, history, storage and mesh-size check."""
import argparse
import json
import math
import struct
from pathlib import Path
from app_harness import application


def near(actual, expected):
    assert math.isclose(actual, expected, rel_tol=1e-8, abs_tol=1e-6), (actual, expected)


def mesh_bounds(path):
    data = path.read_bytes()
    count = struct.unpack_from('<I', data, 80)[0]
    assert len(data) == 84 + count * 50 and count > 0
    coords = [[], [], []]
    for index in range(count):
        for vertex in range(3):
            point = struct.unpack_from('<fff', data, 84 + index * 50 + 12 + vertex * 12)
            for axis in range(3):
                coords[axis].append(point[axis])
    return [[min(axis), max(axis)] for axis in coords]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    out = Path(args.output).resolve(); out.mkdir(parents=True, exist_ok=True)
    with application(args.app, out) as (call, transcript):
        session = call('sketch.begin', {'plane': 'XY', 'name': 'Unit plate'})
        call('sketch.rectangle', {'x1': 0, 'y1': 0, 'x2': 25.4, 'y2': 12.7, 'units': 'mm'})
        draft = call('command.preview', {'session': session['id']})['authoredDraft']
        first_line = draft['parameters']['entities'][0]
        line = first_line['id']
        call('sketch.constraint', {'type': 'fixed', 'pointIds': [first_line['startPointId']]})
        call('sketch.dimension', {'kind': 'horizontal', 'entityId': line, 'value': 25.4, 'units': 'mm'})
        call('sketch.finish')
        original = call('document.inspect')
        sketch = original['authored']['features'][0]
        profile = original['evaluation']['sketches'][0]['profiles'][0]['profileId']
        call('solid.extrude', {'sketchId': sketch['id'], 'profileId': profile, 'distance': 6.35, 'units': 'mm'})
        before = call('document.inspect')
        near(before['evaluation']['bodies'][0]['volumeMm3'], 25.4 * 12.7 * 6.35)
        extrude = before['authored']['features'][-1]
        call('document.units', {'units': 'cm'}, 'invalid_arguments')
        assert call('document.inspect')['authored'] == before['authored']
        unit_session = call('document.settings')
        call('command.update', {'session': unit_session['id'], 'inputs': {'units': 'in'}})
        preview = call('command.preview', {'session': unit_session['id']})
        assert preview['authoredDraft']['displayUnits'] == 'in'
        assert call('document.inspect')['authored'] == before['authored']
        call('command.cancel', {'session': unit_session['id']})
        assert call('document.inspect')['authored'] == before['authored']
        unit_session = call('document.settings')
        call('command.update', {'session': unit_session['id'], 'inputs': {'units': 'in'}})
        call('command.commit', {'session': unit_session['id']})
        inches = call('document.inspect')
        assert inches['units'] == 'in'
        assert inches['authored']['features'] == before['authored']['features']
        assert inches['evaluation']['bodies'] == before['evaluation']['bodies']
        call('view.fit')
        gpu = call('app.state')['viewport']['gpu']
        assert gpu['hardwareAccelerated']
        call('capture.screenshot', {'path': str(out / 'inches-browser.png')})
        edit = call('feature.editDialog', {'featureId': extrude['id']})
        assert edit['inputs']['units'] == 'in'
        near(edit['inputs']['distance'], 0.25)
        call('capture.screenshot', {'path': str(out / 'inches-extrude.png')})
        call('command.cancel', {'session': edit['id']})
        edit = call('sketch.edit', {'sketchId': sketch['id']})
        assert edit['inputs']['units'] == 'in'
        sketch_preview = call('command.preview', {'session': edit['id']})
        dimension = sketch_preview['authoredDraft']['parameters']['dimensions'][0]
        near(dimension['valueMm'], 25.4)
        call('capture.screenshot', {'path': str(out / 'inches-sketch.png')})
        call('sketch.cancel')
        assert call('document.inspect')['authored'] == inches['authored']
        mesh = out / 'unit-plate-mm.stl'
        call('document.exportStl', {'path': str(mesh), 'bodyId': inches['evaluation']['bodies'][0]['bodyId'],
             'units': 'mm', 'format': 'binary', 'surfaceDeviationMm': 0.05, 'normalDeviationDeg': 8})
        bounds = mesh_bounds(mesh)
        for actual, expected in zip(bounds, ([0, 25.4], [0, 12.7], [0, 6.35])):
            for a, b in zip(actual, expected): near(a, b)
        call('history.undo')
        assert call('document.inspect')['authored'] == before['authored']
        call('history.redo')
        assert call('document.inspect')['authored'] == inches['authored']
        edit = call('sketch.edit', {'sketchId': sketch['id']})
        call('sketch.dimensionUpdate', {'dimensionId': dimension['id'], 'value': 1.25, 'units': 'in'})
        call('sketch.finish')
        widened = call('document.inspect')
        near(widened['authored']['features'][0]['parameters']['dimensions'][0]['valueMm'], 31.75)
        near(widened['evaluation']['bodies'][0]['volumeMm3'], 31.75 * 12.7 * 6.35)
        edit = call('feature.editDialog', {'featureId': extrude['id']})
        call('command.update', {'session': edit['id'], 'inputs': {'distance': 0.5}})
        call('command.preview', {'session': edit['id']})
        call('command.commit', {'session': edit['id']})
        changed = call('document.inspect')
        near(changed['authored']['features'][-1]['parameters']['distanceMm'], 12.7)
        near(changed['evaluation']['bodies'][0]['volumeMm3'], 31.75 * 12.7 * 12.7)
        call('history.undo')
        assert call('document.inspect')['authored'] == widened['authored']
        call('history.undo')
        assert call('document.inspect')['authored'] == inches['authored']
        call('history.redo'); call('history.redo')
        assert call('document.inspect')['authored'] == changed['authored']
        saved = out / 'unit-plate.alloy'
        call('document.save', {'path': str(saved)})
        call('document.new', {'discard': False})
        call('document.open', {'path': str(saved)})
        reopened = call('document.inspect')
        assert reopened['authored'] == changed['authored']
        assert reopened['evaluation']['bodies'] == changed['evaluation']['bodies']
        (out / 'receipt.json').write_text(json.dumps({'result': 'passed', 'boundsMm': bounds, 'gpu': gpu,
            'calls': len(transcript), 'transcript': transcript}, indent=2) + '\n')
    print('PASS: display units preserve sketch/extrude geometry, edit values and STL size')


if __name__ == '__main__': main()
