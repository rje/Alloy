#!/usr/bin/env python3
"""Real-app four linked imported hole circles, clearance offsets and joined pegs."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application

p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--fixture',required=True);p.add_argument('--output',required=True)
a=p.parse_args();out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
def ref(face):return {key:face[key] for key in ('producerFeatureId','bodyId','faceId')}
def near(x,y):return math.isclose(x,y,abs_tol=1e-5)
with application(a.app,out,command_timeout=90) as (call,ledger):
    assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
    imported=call('reference.insert',{'path':str(Path(a.fixture).resolve()),'name':'Board Reference'})
    feature_id=imported['featureId']
    placement={'axis':'Z','angleDeg':90,'translationMm':[28,-42.5,6]}
    call('reference.move',{'featureId':feature_id,'placement':placement});call('reference.capture')
    owner=call('component.create',{'name':'Plastic Case','activate':True})['definitionId']
    call('solid.box',{'width':64,'depth':93,'height':3.5,'x':-32,'y':-46.5,'z':0,'units':'mm','name':'Case Floor'})
    state=call('document.inspect');pcb=next(b for b in state['evaluation']['bodies'] if b['name']=='PCB-001')
    floor=next(b for b in state['evaluation']['bodies'] if b['ownerId']==owner)
    top=next(f for f in floor['supportFaces'] if f['role']=='box.z1')
    edges=[e for e in pcb['solidEdges'] if e['provenance'].get('sourceEdgeIndex') in (12,13,14,15)]
    assert len(edges)==4 and len({e['edgeId'] for e in edges})==4
    call('view.orientation',{'name':'top'});call('view.home')
    call('sketch.begin',{'support':ref(top),'name':'Mounting Circles'})
    original=call('app.state')['activeSession']['draftSketch']
    call('sketch.projectBegin')
    call('sketch.projectSelect',{'bodyId':pcb['bodyId'],'edgeId':edges[0]['edgeId']})
    preview=call('sketch.projectPreview')
    assert len(preview['previewDraft']['parameters']['circleProjections'])==1
    assert preview['previewDraft']['parameters']['circleProjections'][0]['source']['occurrenceId']==pcb['occurrenceId']
    call('capture.screenshot',{'path':str(out/'circular-project-preview.png')})
    call('sketch.projectCancel');assert call('app.state')['activeSession']['draftSketch']==original
    call('sketch.projectBegin')
    for edge in edges:call('sketch.projectSelect',{'bodyId':pcb['bodyId'],'edgeId':edge['edgeId']})
    call('sketch.projectCommit')
    draft=call('app.state')['activeSession']['draftSketch']
    circles=draft['parameters']['circleProjections'];assert len(circles)==4
    for circle in circles:call('sketch.circleOffset',{'sourceEntityId':circle['id'],'distance':-0.3,'units':'mm'})
    call('capture.screenshot',{'path':str(out/'circular-offsets.png')})
    call('sketch.finish')
    state=call('document.inspect');sketch=next(s for s in state['evaluation']['sketches'] if s['name']=='Mounting Circles')
    assert len(sketch['projectedCircles'])==4
    assert all(near(c['radiusMm'],1.35) for c in sketch['projectedCircles'])
    peg_profiles=[profile['profileId'] for profile in sketch['profiles'] if near(profile['areaMm2'],math.pi*1.05**2)]
    assert len(peg_profiles)==4,sketch['profiles']
    call('solid.extrude',{'sketchId':sketch['sketchId'],'profileIds':peg_profiles,'distance':5,'units':'mm',
        'operation':'join','targetBodyIds':[floor['bodyId']]})
    pegged=call('document.inspect')
    assert pegged['bodies']==13 and next(b for b in pegged['evaluation']['bodies'] if b['bodyId']==floor['bodyId'])['valid']
    call('capture.screenshot',{'path':str(out/'four-pegs-join.png')})
    shifted={'axis':'Z','angleDeg':90,'translationMm':[29,-42.5,6]}
    call('reference.move',{'featureId':feature_id,'placement':shifted});call('reference.capture')
    moved=call('document.inspect');moved_sketch=next(s for s in moved['evaluation']['sketches'] if s['sketchId']==sketch['sketchId'])
    for before,after in zip(sketch['projectedCircles'],moved_sketch['projectedCircles']):
        assert before['id']==after['id'] and near(after['radiusMm'],before['radiusMm'])
        assert near(abs(after['uMm']-before['uMm'])+abs(after['vMm']-before['vMm']),1)
    call('history.undo');assert call('document.inspect')['authored']==pegged['authored']
    saved=out/'linked-circular-pegs.alloy';call('document.save',{'path':str(saved)})
    call('document.new',{'discard':True});call('document.open',{'path':str(saved),'discard':True})
    assert call('document.inspect')['authored']==pegged['authored']
    (out/'linked-circular-project-api.json').write_text(json.dumps({'ledger':ledger,'sourceBody':pcb['bodyId'],
        'sourceOccurrence':pcb['occurrenceId'],'projectedCircles':sketch['projectedCircles'],
        'pegProfiles':peg_profiles,'joinedFeature':pegged['authored']['features'][-1]},indent=2)+'\n')
print('PASS: four true imported circles, linked Offset−0.3, Join5, capture update, undo and reopen')
