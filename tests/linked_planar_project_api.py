#!/usr/bin/env python3
"""Real-entrypoint linked Project on primitive Box/Shell and sibling sketch."""
import argparse
import json
from pathlib import Path
from app_harness import application

p = argparse.ArgumentParser()
p.add_argument('--app', required=True)
p.add_argument('--output', required=True)
a = p.parse_args()
out = Path(a.output).resolve()
out.mkdir(parents=True, exist_ok=True)
def ref(face):
    return {key: face[key] for key in ('producerFeatureId', 'bodyId', 'faceId')}
def along_y(edge):
    source = edge.get('provenance', edge)
    x, y, z = source['startMm']
    xx, yy, zz = source['endMm']
    return abs(x-xx)<1e-6 and abs(z-zz)<1e-6 and abs(y-yy)>1

with application(a.app, out) as (call, transcript):
    root = call('document.inspect')['authored']['documentId']
    box_owner = call('component.create', {'name': 'Box'})['definitionId']
    call('component.activate', {'definitionId': box_owner})
    call('solid.box', {'width': 50, 'depth': 80, 'height': 40, 'x': 0, 'y': 0, 'z': 0, 'units': 'mm', 'name': 'Box'})
    state = call('document.inspect')
    top = next(f for f in state['evaluation']['bodies'][0]['supportFaces'] if f['role']=='box.z1')
    call('solid.shell', {'targetBodyId': state['evaluation']['bodies'][0]['bodyId'], 'removedFace': ref(top), 'thicknessMm': 4})
    shelled = call('document.inspect')
    body = shelled['evaluation']['bodies'][0]
    front = next(f for f in body['supportFaces'] if f['role']=='box.y0')
    inner = next(e for f in body['supportFaces'] if f['role']=='shell-inner' for e in f['edges'] if along_y(e))
    call('view.home')
    call('sketch.begin', {'support': ref(front), 'name':'Front'})
    before = call('app.state')['activeSession']['draftSketch']
    call('sketch.projectBegin')
    face_result = call('sketch.projectFace', ref(front))
    assert face_result['added']==4 and face_result['selectedCount']==4
    point_result = call('sketch.projectSelect', {'bodyId':body['bodyId'], 'edgeId':inner['edgeId']})
    assert point_result['selectedCount']==5
    preview = call('sketch.projectPreview')
    assert preview['previewDraft']['parameters']['pointProjections']
    assert call('app.state')['activeSession']['draftSketch']==before
    call('capture.screenshot', {'path':str(out/'project-preview.png')})
    call('capture.dialog', {'path':str(out/'project-panel.png')})
    call('sketch.projectCancel')
    assert call('app.state')['activeSession']['draftSketch']==before
    call('sketch.projectBegin')
    call('sketch.projectFace', ref(front))
    call('sketch.projectSelect', {'bodyId':body['bodyId'], 'edgeId':inner['edgeId']})
    call('sketch.projectCommit')
    front_draft = call('app.state')['activeSession']['draftSketch']
    assert len(front_draft['parameters']['pointProjections'])==1
    assert all(item['profile'] for item in front_draft['parameters']['projections'])
    call('sketch.finish')
    front_state = call('document.inspect')
    assert len(front_state['evaluation']['sketches'][0]['profiles'])==1
    lid_owner = call('component.create', {'name':'Lid'})['definitionId']
    call('component.activate', {'definitionId': lid_owner})
    call('sketch.begin', {'support': ref(front), 'name':'Lid front'})
    lid_draft = call('app.state')['activeSession']['draftSketch']
    assert lid_draft['ownerId']==lid_owner
    call('sketch.projectBegin')
    call('sketch.projectSelect', {'bodyId':body['bodyId'], 'edgeId':inner['edgeId']})
    call('sketch.projectCommit')
    call('sketch.finish')
    box_feature = front_state['authored']['features'][0]
    call('feature.edit', {'featureId':box_feature['id'], 'parameters':{'widthMm':74,'depthMm':100,'heightMm':40}})
    resized = call('document.inspect')
    assert resized['evaluation']['bodies'][0]['boundsMm']['size']==[74,100,40]
    path = out/'linked-project.alloy'
    call('document.save', {'path':str(path)})
    call('document.new', {'discard':True})
    call('document.open', {'path':str(path), 'discard':True})
    reopened = call('document.inspect')
    assert reopened['authored']==resized['authored']
(out/'linked-project-api.json').write_text(json.dumps({'source':shelled,'front':front_state,'resized':resized,'reopened':reopened,'transcript':transcript},indent=2))
print('PASS: real-entrypoint linked Project preview/cancel/commit, profile, point, sibling, edit/reopen')
