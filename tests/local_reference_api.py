#!/usr/bin/env python3
"""Real-app linked board placement, picked topology, sibling Project and source status."""
import argparse
import hashlib
import json
import math
import shutil
from pathlib import Path
from app_harness import application

p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--fixture',required=True);p.add_argument('--output',required=True)
a=p.parse_args();out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
source=out/'board-reference.step';shutil.copyfile(a.fixture,source)
def ref(face):return {key:face[key] for key in ('producerFeatureId','bodyId','faceId')}
def close(x,y):assert math.isclose(x,y,abs_tol=1e-4),(x,y)
def dot(a,b):return sum(x*y for x,y in zip(a,b))

with application(a.app,out,command_timeout=90) as (call,ledger):
    assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
    original=call('document.inspect')
    inserted=call('reference.insert',{'path':str(source),'name':'Electronics Reference'})
    assert len(inserted['assembly']['occurrences'])>12
    feature_id=inserted['featureId'];outer_id=inserted['outerOccurrenceId'];board_owner=inserted['definitionId']
    rest=call('document.inspect');assert rest['bodies']==12 and rest['undo']['count']==original['undo']['count']+1
    pcb_rest=next(b for b in rest['evaluation']['bodies'] if b['name']=='PCB-001')
    assert pcb_rest['outerOccurrenceId']==outer_id
    case=call('component.create',{'name':'Case','activate':True})
    call('solid.box',{'width':110,'depth':80,'height':10,'x':-20,'y':-20,'z':-20,'units':'mm','name':'Case stock'})
    call('component.opacitySet',{'definitionId':case['definitionId'],'opacity':0.6})
    case_body=next(b for b in call('document.inspect')['evaluation']['bodies'] if b['ownerId']==case['definitionId'])
    case_top=next(f for f in case_body['supportFaces'] if f['role']=='box.z1')
    placement={'axis':'Z','angleDeg':90,'translationMm':[10,20,15]}
    call('reference.move',{'featureId':feature_id,'placement':placement})
    pending=call('app.state')['pendingReferencePosition'];assert pending['featureId']==feature_id
    call('solid.box',{'width':1,'depth':1,'height':1,'units':'mm'},error='pending_reference_position')
    call('document.save',{'path':str(out/'refused-pending.alloy')},error='pending_reference_position')
    assert call('document.inspect')['authored']==call('document.inspect')['authored']
    call('reference.revert');assert not call('app.state')['pendingReferencePosition']
    call('reference.move',{'featureId':feature_id,'placement':placement})
    call('reference.capture')
    moved=call('document.inspect');pcb=next(b for b in moved['evaluation']['bodies'] if b['bodyId']==pcb_rest['bodyId'])
    assert pcb['occurrenceId']==pcb_rest['occurrenceId'] and pcb['outerOccurrenceId']==outer_id
    assert pcb['boundsMm']!=pcb_rest['boundsMm']
    assert next(b for b in moved['evaluation']['bodies'] if b['bodyId']==case_body['bodyId'])['opacity']==0.6
    call('view.orientation',{'name':'top'});call('view.home')
    # Choose a real horizontal board face near its central solid region.
    faces=[f for f in pcb['supportFaces'] if abs(f['frame']['normal'][2])>0.99]
    face=max(faces,key=lambda f:f['frame']['origin'][2])
    frame=face['frame'];bounds=pcb['boundsMm'];world=[(lo+hi)/2 for lo,hi in zip(bounds['min'],bounds['max'])]
    delta=[x-y for x,y in zip(world,frame['origin'])]
    pixel=call('view.projectFacePoint',{'support':ref(face),'uMm':dot(delta,frame['uAxis']),'vMm':dot(delta,frame['vAxis'])})
    picked=call('selection.facePick',{'x':pixel['x'],'y':pixel['y']})
    assert picked['outerOccurrenceId']==outer_id,picked
    picked_body=next(b for b in moved['evaluation']['bodies'] if b['bodyId']==picked['support']['bodyId'])
    assert picked['occurrenceId']==picked_body['occurrenceId'] and picked['support'] in [ref(f) for f in picked_body['supportFaces']]
    # A top perimeter edge projects to the viewport, then a real screen pick recovers its occurrence-qualified identity.
    candidates=[e for e in pcb['solidEdges'] if len(e['provenance'].get('startMm',[]))==3
        and abs(e['provenance']['startMm'][2]-e['provenance']['endMm'][2])<1e-5]
    edge=max(candidates,key=lambda e: math.dist(e['provenance']['startMm'],e['provenance']['endMm']))
    edge_pixel=call('view.projectEdge',{'bodyId':pcb['bodyId'],'edgeId':edge['edgeId']})
    picked_edge=call('selection.edgePick',{'x':edge_pixel['x'],'y':edge_pixel['y']})
    assert picked_edge['bodyId']==pcb['bodyId'] and picked_edge['outerOccurrenceId']==outer_id,picked_edge
    assert picked_edge['occurrenceId']==pcb['occurrenceId']
    call('capture.screenshot',{'path':str(out/'board-moved-case-opacity.png')})
    # Project a retained straight imported edge into the sibling Case sketch.
    call('sketch.begin',{'support':ref(case_top),'name':'Reference driven case'})
    call('sketch.projectBegin')
    selected=call('sketch.projectSelect',{'bodyId':pcb['bodyId'],'edgeId':edge['edgeId']})
    assert selected['selectedCount']==1
    call('sketch.projectCommit');call('sketch.finish')
    before_project=call('document.inspect')
    sketch=next(s for s in before_project['evaluation']['sketches'] if s['name']=='Reference driven case')
    shifted={'axis':'Z','angleDeg':90,'translationMm':[20,20,15]}
    call('reference.move',{'featureId':feature_id,'placement':shifted});call('reference.capture')
    after_project=call('document.inspect')
    after_sketch=next(s for s in after_project['evaluation']['sketches'] if s['sketchId']==sketch['sketchId'])
    before_line=next(line for line in sketch['projectedLines'] if line['source']=={'bodyId':pcb['bodyId'],'edgeId':edge['edgeId']})
    after_line=next(line for line in after_sketch['projectedLines'] if line['id']==before_line['id'])
    for key in ('u1Mm','u2Mm'):close(after_line[key]-before_line[key],10)
    for key in ('v1Mm','v2Mm'):close(after_line[key],before_line[key])
    path=out/'linked-reference.alloy';call('document.save',{'path':str(path)})
    call('document.new',{'discard':True});call('document.open',{'path':str(path),'discard':True})
    reopened=call('document.inspect');assert reopened['authored']==after_project['authored']
    assert call('reference.inspect')['references'][0]['sourceStatus']=='current'
    source.rename(out/'board-reference-missing.step')
    assert call('reference.inspect')['references'][0]['sourceStatus']=='missing'
    call('reference.update',{'featureId':feature_id},error='source_missing')
    assert call('document.inspect')['authored']==reopened['authored']
    source.write_bytes((out/'board-reference-missing.step').read_bytes()+b'\n')
    assert call('reference.inspect')['references'][0]['sourceStatus']=='changed'
    call('reference.update',{'featureId':feature_id},error='source_changed')
    assert call('document.inspect')['authored']==reopened['authored']
    source.write_bytes((out/'board-reference-missing.step').read_bytes())
    assert call('reference.update',{'featureId':feature_id})['status']=='current'
    (out/'local-reference-api.json').write_text(json.dumps({'fixtureSha256':hashlib.sha256(Path(a.fixture).read_bytes()).hexdigest(),
        'featureId':feature_id,'outerOccurrenceId':outer_id,'pickedFace':picked,'pickedEdge':picked_edge,
        'projectedBefore':sketch,'projectedAfter':after_sketch,'ledger':ledger},indent=2)+'\n')
print('PASS: linked board placement, world picks, sibling Project, opacity, source status and reopen')
