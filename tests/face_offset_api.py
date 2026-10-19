#!/usr/bin/env python3
"""Real-app retained planar Offset Face, downstream Hole and printable readback."""
import argparse
import json
import math
import struct
from pathlib import Path
from app_harness import application


def near(actual, expected, tolerance=1e-5):
    assert math.isclose(actual, expected, rel_tol=1e-8, abs_tol=tolerance), (actual, expected)


def support(face):
    return {key: face[key] for key in ('producerFeatureId', 'bodyId', 'faceId')}


def mesh_bounds(path):
    data = path.read_bytes()
    count = struct.unpack_from('<I', data, 80)[0]
    assert count > 0 and len(data) == 84 + 50 * count
    axes = [[], [], []]
    for index in range(count):
        for vertex in range(3):
            xyz = struct.unpack_from('<fff', data, 84 + 50 * index + 12 + 12 * vertex)
            for axis in range(3): axes[axis].append(xyz[axis])
    return [[min(axis), max(axis)] for axis in axes]


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--app', required=True)
parser.add_argument('--output', required=True)
args = parser.parse_args()
out = Path(args.output).resolve(); out.mkdir(parents=True, exist_ok=True)
with application(args.app, out) as (call, transcript):
    inspect = lambda: call('document.inspect')
    call('solid.box', {'width': 40, 'depth': 30, 'height': 20, 'units': 'mm', 'name': 'Offset stock'})
    original = inspect()
    body = original['evaluation']['bodies'][0]
    target = body['bodyId']
    top = next(face for face in body['supportFaces'] if face['role'] == 'box.z1')
    face_ref = support(top)
    call('solid.faceOffset', {'targetBodyId': target, 'face': face_ref, 'distanceMm': 5})
    offset = inspect()
    feature = offset['authored']['features'][-1]
    assert feature['type'] == 'faceOffset'
    moved = offset['evaluation']['bodies'][0]
    assert moved['bodyId'] == target and moved['valid'] and moved['closed']
    near(moved['volumeMm3'], 40 * 30 * 25)
    for actual, expected in zip(moved['boundsMm']['size'], (40, 30, 25)): near(actual, expected)
    cap = next(face for face in moved['supportFaces'] if face['producerFeatureId'] == feature['id'])
    cap_ref = support(cap)
    sketch_session = call('sketch.begin', {'support': cap_ref, 'name': 'Bore on moved cap'})
    frame = sketch_session['resolvedFrame']
    offset = [value - origin for value, origin in zip((20, 15, 25), frame['origin'])]
    u = sum(value * axis for value, axis in zip(offset, frame['uAxis']))
    v = sum(value * axis for value, axis in zip(offset, frame['vAxis']))
    call('sketch.addPoint', {'x': u, 'y': v, 'units': 'mm'})
    point_sketch = call('app.state')['activeSession']['draftSketch']
    point = point_sketch['parameters']['entities'][0]
    call('sketch.finish')
    hole = {'sourceSketchId': point_sketch['id'], 'sourceEntityId': point['id'],
        'sourcePointId': point['pointId'], 'targetBodyId': target, 'placement': 'fromSketch',
        'holeType': 'countersink', 'extent': 'all', 'boreDiameterMm': 4,
        'sinkDiameterMm': 8, 'includedAngleDeg': 90}
    call('solid.hole', hole)
    drilled = inspect()
    assert drilled['evaluation']['bodies'][0]['volumeMm3'] < moved['volumeMm3']
    call('view.orientation', {'name': 'isometric'})
    call('view.fit')
    gpu = call('app.state')['viewport']['gpu']; assert gpu['hardwareAccelerated']
    call('capture.screenshot', {'path': str(out / 'offset-hole.png')})
    call('solid.faceOffset', {'featureId': feature['id'], 'targetBodyId': target,
        'face': face_ref, 'distanceMm': 8})
    edited = inspect()
    assert next(row for row in edited['authored']['features'] if row['id'] == feature['id'])['type'] == 'faceOffset'
    assert support(next(face for face in edited['evaluation']['bodies'][0]['supportFaces']
        if face['producerFeatureId'] == feature['id'])) == cap_ref
    near(edited['evaluation']['bodies'][0]['boundsMm']['size'][2], 28)
    near(edited['evaluation']['bodies'][0]['entryMm'][2], 28)
    before_refusal = edited['authored']
    call('solid.faceOffset', {'featureId': feature['id'], 'targetBodyId': target,
        'face': face_ref, 'distanceMm': -30}, error='evaluation_failed')
    assert inspect()['authored'] == before_refusal
    call('history.undo')
    assert inspect()['authored'] == drilled['authored']
    call('history.redo')
    assert inspect()['authored'] == edited['authored']
    saved = out / 'offset-hole.alloy'
    call('document.save', {'path': str(saved)})
    call('document.new', {'discard': False})
    call('document.open', {'path': str(saved)})
    reopened = inspect()
    assert reopened['authored'] == edited['authored']
    near(reopened['evaluation']['bodies'][0]['entryMm'][2], 28)
    mesh = out / 'offset-hole-mm.stl'
    call('document.exportStl', {'path': str(mesh), 'bodyId': target, 'units': 'mm',
        'format': 'binary', 'surfaceDeviationMm': 0.05, 'normalDeviationDeg': 8})
    bounds = mesh_bounds(mesh)
    for actual, expected in zip(bounds, ([0, 40], [0, 30], [0, 28])):
        near(actual[0], expected[0], 0.05); near(actual[1], expected[1], 0.05)
    # Two offsets on one body must select each feature's own moved face.
    call('document.new', {'discard': True})
    call('solid.box', {'width': 20, 'depth': 20, 'height': 10, 'units': 'mm'})
    stock = inspect()['evaluation']['bodies'][0]
    top = next(face for face in stock['supportFaces'] if face['role'] == 'box.z1')
    side = next(face for face in stock['supportFaces'] if face['role'] == 'box.x1')
    call('solid.faceOffset', {'targetBodyId': stock['bodyId'], 'face': support(top), 'distanceMm': 2})
    first = inspect()['authored']['features'][-1]['id']
    call('solid.faceOffset', {'targetBodyId': stock['bodyId'], 'face': support(side), 'distanceMm': 3})
    second_state = inspect()
    second = second_state['authored']['features'][-1]['id']
    assert first != second
    chosen = call('app.state')['selection']['support']
    assert chosen['producerFeatureId'] == second
    expected = support(next(face for face in second_state['evaluation']['bodies'][0]['supportFaces']
        if face['producerFeatureId'] == second))
    assert support(chosen) == expected, (chosen, expected)
    dialog = call('solid.faceOffsetDialog', {'featureId': second})
    assert support(dialog['movedFace']) == expected and dialog['previewed'], dialog
    call('command.cancel', {'session': dialog['id']})
    (out / 'receipt.json').write_text(json.dumps({'result': 'passed', 'calls': len(transcript),
        'bodyId': target, 'offsetFeatureId': feature['id'], 'cap': cap_ref,
        'boundsMm': bounds, 'gpu': gpu, 'transcript': transcript}, indent=2) + '\n')
print('PASS: retained planar Offset Face preserves downstream Hole and printable bounds')
