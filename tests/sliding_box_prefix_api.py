#!/usr/bin/env python3
"""Integrated named primitive Box/Shell prefix, not the complete lid tutorial."""
import argparse
import json
from pathlib import Path
from app_harness import application

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--app', required=True)
parser.add_argument('--output', required=True)
args = parser.parse_args()
output = Path(args.output).resolve()
with application(args.app, output) as (call, transcript):
    for name, value in [('Width', 50), ('Length', 80), ('Height', 40), ('Thickness', 4)]:
        call('document.parameterAdd', {'name': name, 'value': value, 'units': 'mm'})
    initial = call('document.inspect')
    ids = {p['name']: p['id'] for p in initial['userParameters']}
    call('solid.box', {'widthParameter': 'Width', 'depthParameter': 'Length',
                       'heightParameter': 'Height', 'units': 'mm'})
    box = call('document.inspect')['evaluation']['bodies'][0]
    top = next(f for f in box['supportFaces'] if f['role'] == 'box.z1')
    ref = {key: top[key] for key in ('producerFeatureId', 'bodyId', 'faceId')}
    call('solid.shell', {'targetBodyId': box['bodyId'], 'removedFace': ref,
                         'thicknessParameter': 'Thickness'})
    original = call('document.inspect')
    body = original['evaluation']['bodies'][0]
    assert body['valid'] and body['closed']
    assert abs(body['volumeMm3'] - (50*80*40 - 42*72*36)) < 1e-4
    back = next(f for f in body['supportFaces'] if f['role'] == 'shell-inner'
                and f['provenance']['boxFace'] == 'y1')
    assert abs(back['frame']['origin'][1] - 76) < 1e-6
    edge_ids = {e['edgeId'] for e in back['edges']}
    call('view.home')
    call('capture.screenshot', {'path': str(output / 'original-box-shell.png')})
    for name, value in [('Length', 40), ('Width', 74), ('Height', 100), ('Thickness', 3)]:
        call('document.parameterUpdate', {'parameterId': ids[name], 'value': value})
    edited = call('document.inspect')
    body = edited['evaluation']['bodies'][0]
    assert body['valid'] and body['closed']
    assert abs(body['volumeMm3'] - (74*40*100 - 68*34*97)) < 1e-4
    current_back = next(f for f in body['supportFaces'] if f['faceId'] == back['faceId'])
    assert abs(current_back['frame']['origin'][1] - 37) < 1e-6
    assert {e['edgeId'] for e in current_back['edges']} == edge_ids
    call('history.undo')
    call('history.redo')
    assert call('document.inspect')['authored'] == edited['authored']
    path = output / 'parameterized-box-shell.alloy'
    call('document.save', {'path': str(path)})
    call('document.open', {'path': str(path), 'discard': True})
    reopened = call('document.inspect')
    assert reopened['authored'] == edited['authored']
    assert abs(reopened['evaluation']['bodies'][0]['volumeMm3'] - body['volumeMm3']) < 1e-4
    call('view.home')
    gpu = call('app.state')['viewport']['gpu']
    assert gpu['hardwareAccelerated']
    call('capture.screenshot', {'path': str(output / 'edited-box-shell.png')})
(output / 'prefix.json').write_text(json.dumps({'original': original, 'edited': edited,
    'reopened': reopened, 'gpu': gpu, 'transcript': transcript}, indent=2) + '\n')
print('PASS: named primitive Box/Shell source prefix, resize, retained back edges, history and reopen')
