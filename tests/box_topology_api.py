#!/usr/bin/env python3
"""Real-entrypoint Box → Shell topology receipt; run with the sole editor lease."""
import argparse
import json
from pathlib import Path
from app_harness import application

parser = argparse.ArgumentParser()
parser.add_argument('--app', required=True)
parser.add_argument('--output', required=True)
args = parser.parse_args()
output = Path(args.output).resolve()

def support(face):
    return {key: face[key] for key in ('producerFeatureId', 'bodyId', 'faceId')}

with application(args.app, output) as (call, transcript):
    assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
    call('solid.box', {'width': 50, 'depth': 80, 'height': 40,
                       'x': 0, 'y': 0, 'z': 0, 'units': 'mm', 'name': 'Box'})
    initial = call('document.inspect')
    box = initial['evaluation']['bodies'][0]
    assert len(box['supportFaces']) == 6 and len(box['solidEdges']) == 12
    top = next(face for face in box['supportFaces'] if face['role'] == 'box.z1')
    assert len(top['edges']) == 4
    call('view.orientation', {'name': 'top'})
    call('view.home')
    call('capture.screenshot', {'path': str(output / 'box.png')})
    session = call('solid.shellDialog')
    pixel = call('view.projectFacePoint', {'support': support(top), 'uMm': 25, 'vMm': 40})
    picked = call('selection.pick', {'x': pixel['x'], 'y': pixel['y']})
    assert picked == support(top)
    call('command.update', {'session': session['id'], 'inputs': {'thicknessMm': 4}})
    assert call('command.preview', {'session': session['id']})['geometryEvaluated']
    call('capture.dialog', {'path': str(output / 'shell-panel.png')})
    call('command.cancel', {'session': session['id']})
    assert call('document.inspect')['authored'] == initial['authored']
    call('solid.shell', {'targetBodyId': box['bodyId'], 'removedFace': support(top),
                         'thicknessMm': 4})
    shelled = call('document.inspect')
    body = shelled['evaluation']['bodies'][0]
    assert body['valid'] and body['closed'] and abs(body['volumeMm3'] - (50*80*40-42*72*36)) < 1e-3
    inner = next(face for face in body['supportFaces'] if face['role'] == 'shell-inner'
                 and face['provenance']['boxFace'] == 'y1')
    assert abs(inner['frame']['origin'][1] - 76) < 1e-5 and inner['frame']['normal'] == [0, -1, 0]
    assert len(inner['edges']) == 4 and all(edge['supported'] for edge in inner['edges'])
    inner_face_id = inner['faceId']
    inner_edge_ids = {edge['edgeId'] for edge in inner['edges']}
    call('view.home')
    call('capture.screenshot', {'path': str(output / 'box-shell.png')})
    feature = initial['authored']['features'][0]
    call('feature.edit', {'featureId': feature['id'], 'parameters': {'widthMm': 54, 'depthMm': 90}})
    edited = call('document.inspect')
    changed = edited['evaluation']['bodies'][0]
    assert changed['bodyId'] == box['bodyId'] and abs(changed['volumeMm3'] - (54*90*40-46*82*36)) < 1e-3
    changed_inner = next(face for face in changed['supportFaces'] if face['faceId'] == inner_face_id)
    assert abs(changed_inner['frame']['origin'][1] - 86) < 1e-5
    assert {edge['edgeId'] for edge in changed_inner['edges']} == inner_edge_ids
    call('history.undo')
    call('history.redo')
    assert call('document.inspect')['authored'] == edited['authored']
    path = output / 'box-shell.alloy'
    call('document.save', {'path': str(path)})
    call('document.new', {'discard': True})
    call('document.open', {'path': str(path), 'discard': True})
    reopened = call('document.inspect')
    assert reopened['authored'] == edited['authored']
    assert any(face['faceId'] == inner_face_id for face in reopened['evaluation']['bodies'][0]['supportFaces'])
(output / 'box-topology-api.json').write_text(json.dumps({'initial': initial, 'shell': shelled,
    'edited': edited, 'reopened': reopened, 'transcript': transcript}, indent=2))
print('PASS: real-entrypoint Box, top-face Shell, inner back references, edit/history/reopen')
