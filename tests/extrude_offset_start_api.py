#!/usr/bin/env python3
"""Real-app front-sketch offset cutter and persistent edit workflow."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--app', required=True)
parser.add_argument('--output', required=True)
args = parser.parse_args()
out = Path(args.output).resolve()

with application(args.app, out) as (call, transcript):
    assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
    call('solid.box', {'width': 50, 'depth': 80, 'height': 40, 'units': 'mm'})
    box = call('document.inspect')['evaluation']['bodies'][0]
    target = box['bodyId']
    session = call('sketch.begin', {'plane': 'XZ', 'name': 'Front cutter profile'})
    sketch_id = session['draftSketch']['id']
    call('sketch.centerPointEllipse', {'centerX': 25, 'centerY': 20,
         'firstX': 35, 'firstY': 20, 'contourX': 25, 'contourY': 28, 'units': 'mm'})
    profile = call('command.preview', {'session': session['id']})['sketchGeometry']['profiles'][0]
    assert profile['frame']['normal'] == [0, -1, 0], profile['frame']
    call('sketch.finish')
    source = call('document.inspect')
    profile_id = source['evaluation']['sketches'][0]['profiles'][0]['profileId']
    assert abs(source['evaluation']['sketches'][0]['boundsMm']['min'][1]) < 1e-6
    session = call('solid.extrudeDialog', {'sketchId': sketch_id, 'profileId': profile_id})
    call('command.update', {'session': session['id'], 'inputs': {
        'start': 'offset', 'startOffset': -10, 'distance': 5,
        'operation': 'cut', 'targetBodyIds': [target]}})
    preview = call('command.preview', {'session': session['id']})
    expected = 160000 - 400 * math.pi
    assert math.isclose(preview['geometry']['volumeMm3'], expected, abs_tol=1e-4), preview['geometry']
    assert call('document.inspect') == source
    call('view.home')
    call('capture.screenshot', {'path': str(out / 'front-cut-preview.png')})
    call('capture.dialog', {'path': str(out / 'front-cut-dialog.png')})
    call('command.cancel', {'session': session['id']})
    assert call('document.inspect') == source
    call('solid.extrude', {'sketchId': sketch_id, 'profileId': profile_id, 'distance': 5,
         'start': 'offset', 'startOffset': -10, 'operation': 'cut',
         'targetBodyIds': [target], 'units': 'mm'})
    cut = call('document.inspect')
    feature = cut['authored']['features'][-1]
    assert feature['parameters']['startOffsetMm'] == -10
    assert feature['parameters']['distanceMm'] == 5
    assert math.isclose(cut['evaluation']['bodies'][0]['volumeMm3'], expected, abs_tol=1e-4)
    assert abs(cut['evaluation']['sketches'][0]['boundsMm']['min'][1]) < 1e-6
    call('history.undo')
    assert call('document.inspect')['authored'] == source['authored']
    call('history.redo')
    assert call('document.inspect')['authored'] == cut['authored']
    edit = call('feature.editDialog', {'featureId': feature['id']})
    assert edit['inputs']['start'] == 'offset' and edit['inputs']['startOffset'] == -10
    call('command.update', {'session': edit['id'], 'inputs': {'startOffset': -8}})
    call('command.preview', {'session': edit['id']})
    call('command.commit', {'session': edit['id']})
    edited = call('document.inspect')
    assert edited['authored']['features'][-1]['parameters']['startOffsetMm'] == -8
    path = out / 'offset-front-cut.alloy'
    call('document.save', {'path': str(path)})
    call('document.open', {'path': str(path), 'discard': True})
    assert call('document.inspect')['authored'] == edited['authored']
    call('view.home')
    call('capture.screenshot', {'path': str(out / 'front-cut-reopened.png')})

(out / 'receipt.json').write_text(json.dumps({'featureId': feature['id'], 'targetBodyId': target,
    'expectedVolumeMm3': expected, 'transcript': transcript}, indent=2) + '\n')
print('PASS: real front ellipse offset cutter, preview/cancel, cut, edit, undo and reopen')
