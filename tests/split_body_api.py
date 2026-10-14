#!/usr/bin/env python3
"""Bounded real-app Split Body by retained construction plane receipt."""
import argparse
import json
from pathlib import Path
from app_harness import application

parser = argparse.ArgumentParser()
parser.add_argument('--app', required=True)
parser.add_argument('--output', required=True)
args = parser.parse_args()
out = Path(args.output).resolve()

def support(face):
    return {key: face[key] for key in ('producerFeatureId', 'bodyId', 'faceId')}

def halves(state):
    bodies = state['evaluation']['bodies']
    assert len(bodies) == 2, bodies
    assert all(b['closed'] and b['valid'] and b['topology']['solids'] == 1 for b in bodies)
    return bodies

with application(args.app, out) as (call, transcript):
    gpu = call('app.state')['viewport']['gpu']
    assert gpu['hardwareAccelerated'], gpu
    call('solid.box', {'width': 50, 'depth': 80, 'height': 40, 'units': 'mm', 'name': 'Enclosure'})
    initial = call('document.inspect')
    stock = initial['evaluation']['bodies'][0]
    faces = stock['supportFaces']
    top = support(next(f for f in faces if f['role'] == 'box.z1'))
    bottom = support(next(f for f in faces if f['role'] == 'box.z0'))
    plane = call('construction.midplane', {'faces': [top, bottom], 'name': 'Case Midplane'})['planeId']
    before = call('document.inspect')
    body_id = stock['bodyId']
    session = call('command.begin', {'command': 'solid.splitBody', 'inputs': {}})
    assert not session['previewed']
    call('selection.set', {'bodyId': body_id})
    ready = call('selection.planeSet', {'planeId': plane})
    assert ready['previewed'] and ready['geometryEvaluated'] and len(ready['geometry']) == 2
    call('capture.screenshot', {'path': str(out / 'split-preview.png')})
    call('capture.dialog', {'path': str(out / 'split-panel.png')})
    call('command.cancel', {'session': session['id']})
    assert call('document.inspect')['authored'] == before['authored']
    session = call('solid.splitBodyDialog', {'targetBodyId': body_id, 'planeId': plane})
    assert session['previewed']
    call('command.commit', {'session': session['id']})
    split = call('document.inspect')
    left, right = halves(split)
    assert left['bodyId'] == body_id and abs(left['volumeMm3'] - 80000) < 1e-5
    assert right['bodyId'] != body_id and abs(right['volumeMm3'] - 80000) < 1e-5
    assert left['splitSide'] == 'negative' and right['splitSide'] == 'positive'
    assert all(any(f['role'].startswith('split.') for f in body['supportFaces']) for body in (left, right))
    call('capture.screenshot', {'path': str(out / 'split-committed.png')})
    split_feature = split['authored']['features'][-1]
    assert split_feature['bodyId'] == right['bodyId']
    call('history.undo')
    assert call('document.inspect')['authored'] == before['authored']
    call('history.redo')
    assert call('document.inspect')['authored'] == split['authored']
    box_id = initial['authored']['features'][0]['id']
    call('feature.edit', {'featureId': box_id, 'parameters': {'heightMm': 50}})
    resized = call('document.inspect')
    assert [round(x['volumeMm3']) for x in halves(resized)] == [100000, 100000]
    assert resized['evaluation']['bodies'][1]['bodyId'] == right['bodyId']
    call('feature.edit', {'featureId': plane, 'parameters': {'source': {'plane': 'XY'}, 'offsetMm': 18}})
    moved = call('document.inspect')
    assert [round(x['volumeMm3']) for x in halves(moved)] == [72000, 128000]
    call('feature.edit', {'featureId': plane, 'parameters': {'source': {'plane': 'XY'}, 'offsetMm': 60}}, error='evaluation_failed')
    assert call('document.inspect')['authored'] == moved['authored']
    path = out / 'split-body.alloy'
    call('document.save', {'path': str(path)})
    call('document.new', {'discard': True})
    call('document.open', {'path': str(path), 'discard': True})
    reopened = call('document.inspect')
    assert reopened['authored'] == moved['authored']
    assert [round(x['volumeMm3']) for x in halves(reopened)] == [72000, 128000]
    assert reopened['evaluation']['bodies'][1]['bodyId'] == right['bodyId']
    call('view.home')
    call('capture.screenshot', {'path': str(out / 'split-reopened.png')})
(out / 'split-body-api-ledger.json').write_text(json.dumps({'gpu': gpu, 'split': split, 'moved': moved,
    'reopened': reopened, 'transcript': transcript}, indent=2))
print('PASS: real Split Body preview/cancel, two closed volumes, history, edits and reopen')
