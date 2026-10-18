#!/usr/bin/env python3
"""Real-entrypoint sketch deletion, retained profile, solid and reopen gate."""
import argparse
import json
from pathlib import Path
from app_harness import application

parser = argparse.ArgumentParser()
parser.add_argument('--app', required=True)
parser.add_argument('--output', required=True)
args = parser.parse_args()
out = Path(args.output).resolve()

with application(args.app, out) as (call, transcript):
    assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
    call('sketch.begin', {'plane':'XY', 'name':'Delete profile repair'})
    call('sketch.addCircle', {'centerX':0, 'centerY':0, 'diameter':30, 'units':'mm'})
    call('sketch.line', {'x1':-25, 'y1':25, 'x2':25, 'y2':25, 'units':'mm'})
    before = call('app.state')['activeSession']['draftSketch']
    sketch_id = before['id']
    circle, line = before['parameters']['entities']
    call('sketch.selectionSet', {'entityIds':[line['id']]})
    deleted = call('sketch.deleteSelection')
    params = deleted['draftSketch']['parameters']
    assert [e['id'] for e in params['entities']] == [circle['id']]
    assert all(p['id'] != line['startPointId'] and p['id'] != line['endPointId'] for p in params['points'])
    assert params['points'][0]['id'] == circle['centerPointId']
    call('capture.screenshot', {'path':str(out/'remaining-circle.png')})
    call('sketch.finish')
    finished = call('document.inspect')
    profile = finished['evaluation']['sketches'][0]['profiles'][0]['profileId']
    call('solid.extrude', {'sketchId':sketch_id, 'profileId':profile, 'distance':8, 'units':'mm'})
    solid = call('document.inspect')
    body = solid['evaluation']['bodies'][0]
    assert body['valid'] and body['closed'] and body['volumeMm3'] > 5600
    path = out/'deleted-profile-solid.alloy'
    call('document.save', {'path':str(path)})
    call('document.open', {'path':str(path), 'discard':True})
    reopened = call('document.inspect')
    assert reopened['authored'] == solid['authored']
    assert reopened['evaluation']['bodies'][0]['bodyId'] == body['bodyId']
    call('capture.screenshot', {'path':str(out/'reopened-solid.png')})
    call('sketch.begin', {'plane':'XY', 'name':'Dependency refusal'})
    call('sketch.centerPointSlot', {'centerX':55, 'centerY':0, 'endX':75,
        'endY':0, 'width':10, 'units':'mm'})
    before_refusal = call('app.state')['activeSession']
    side_id = before_refusal['draftSketch']['parameters']['entities'][0]['id']
    call('sketch.selectionSet', {'entityIds':[side_id]})
    selected = call('app.state')['activeSession']
    call('sketch.deleteSelection', error='sketch_solve_failed')
    after_refusal = call('app.state')['activeSession']
    assert after_refusal['draftSketch'] == selected['draftSketch']
    assert after_refusal['draftHistory'] == selected['draftHistory']
    assert after_refusal['selection'] == selected['selection']
    call('sketch.cancel')
    (out/'receipt.json').write_text(json.dumps({
        'sketchId':sketch_id, 'removedEntityId':line['id'],
        'retainedEntityId':circle['id'], 'bodyId':body['bodyId'],
        'volumeMm3':body['volumeMm3'], 'dependencyRefusalTarget':side_id,
        'gpu':call('app.state')['viewport']['gpu'],
    }, indent=2)+'\n')
    (out/'transcript.json').write_text(json.dumps(transcript, indent=2)+'\n')
