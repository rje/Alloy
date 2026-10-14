#!/usr/bin/env python3
"""Bounded real-entrypoint retained Midplane evidence; use the private editor lease."""
import argparse
import json
from pathlib import Path
from app_harness import application

parser = argparse.ArgumentParser()
parser.add_argument('--app', required=True)
parser.add_argument('--output', required=True)
args = parser.parse_args()
output = Path(args.output).resolve()

def ref(face):
    return {key: face[key] for key in ('producerFeatureId', 'bodyId', 'faceId')}

def height(inspected):
    return inspected['evaluation']['constructionPlanes'][0]['frame']['origin'][2]

with application(args.app, output) as (call, transcript):
    gpu = call('app.state')['viewport']['gpu']
    assert gpu['hardwareAccelerated'], gpu
    call('solid.box', {'width': 50, 'depth': 80, 'height': 40, 'units': 'mm', 'name': 'Enclosure'})
    initial = call('document.inspect')
    faces = initial['evaluation']['bodies'][0]['supportFaces']
    top = ref(next(face for face in faces if face['role'] == 'box.z1'))
    bottom = ref(next(face for face in faces if face['role'] == 'box.z0'))
    side = ref(next(face for face in faces if face['role'] == 'box.x0'))

    session = call('command.begin', {'command': 'construction.midplane', 'inputs': {}})
    assert session['inputs']['faces'] == [] and not session['previewed']
    first = call('selection.faceSet', {'support': top})
    assert len(first['inputs']['faces']) == 1 and not first['previewed']
    call('selection.faceSet', {'support': side}, error='invalid_faces')
    assert call('app.state')['activeSession']['inputs']['faces'] == [top]
    second = call('selection.faceSet', {'support': bottom})
    assert second['previewed'] and len(second['inputs']['faces']) == 2
    call('capture.screenshot', {'path': str(output / 'midplane-preview.png')})
    call('capture.dialog', {'path': str(output / 'midplane-panel.png')})
    call('command.cancel', {'session': session['id']})
    assert call('document.inspect')['authored'] == initial['authored']

    created = call('construction.midplane', {'faces': [top, bottom], 'name': 'Case Midplane'})
    plane_id = created['planeId']
    baseline = call('document.inspect')
    assert abs(height(baseline) - 20) < 1e-7
    plane = baseline['authored']['features'][-1]
    assert plane['id'] == plane_id and plane['parameters'] == {'source': {'faces': [top, bottom]}}
    call('sketch.begin', {'planeId': plane_id, 'name': 'Split profile'})
    call('sketch.rectangle', {'x1': 0, 'y1': 0, 'x2': 10, 'y2': 10, 'units': 'mm'})
    call('sketch.finish')
    with_sketch = call('document.inspect')
    assert abs(with_sketch['evaluation']['sketches'][0]['frame']['origin'][2] - 20) < 1e-7

    box_id = initial['authored']['features'][0]['id']
    call('feature.edit', {'featureId': box_id, 'parameters': {'heightMm': 60}})
    resized = call('document.inspect')
    assert abs(height(resized) - 30) < 1e-7
    assert abs(resized['evaluation']['sketches'][0]['frame']['origin'][2] - 30) < 1e-7
    call('construction.midplane', {'featureId': plane_id, 'faces': [top, side]}, error='evaluation_failed')
    assert call('document.inspect')['authored'] == resized['authored']
    call('history.undo')
    assert call('document.inspect')['authored'] == with_sketch['authored']
    call('history.redo')
    assert call('document.inspect')['authored'] == resized['authored']
    path = output / 'retained-midplane.alloy'
    call('document.save', {'path': str(path)})
    call('document.new', {'discard': True})
    call('document.open', {'path': str(path), 'discard': True})
    reopened = call('document.inspect')
    assert reopened['authored'] == resized['authored'] and abs(height(reopened)-30) < 1e-7
    call('view.home')
    call('capture.screenshot', {'path': str(output / 'midplane-reopened.png')})

(output / 'midplane-api-ledger.json').write_text(json.dumps({'gpu': gpu, 'baseline': baseline,
    'resized': resized, 'reopened': reopened, 'transcript': transcript}, indent=2))
print('PASS: retained Midplane session, geometry, source edit, sketch, history and reopen')
