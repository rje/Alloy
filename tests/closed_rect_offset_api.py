#!/usr/bin/env python3
"""Actual adapted connector face Project and three retained outward rectangle clearances."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--design',required=True);p.add_argument('--output',required=True);a=p.parse_args()
out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
def ref(face):return {key:face[key] for key in ('producerFeatureId','bodyId','faceId')}
def yface(body,y):
    return next(face for face in body['supportFaces'] if abs(face['frame']['origin'][1]-y)<1e-6
        and face['frame']['normal'][1]>.99 and (body['name']=='Extrude' or len(face['edges'])==4))
with application(a.app,out,command_timeout=120) as (call,ledger):
    call('document.open',{'path':str(Path(a.design).resolve()),'discard':True})
    state=call('document.inspect');owner=next(c['id'] for c in state['authored']['components'] if c['name']=='Case')
    call('component.activate',{'definitionId':owner})
    for analysis in call('analysis.inspect')['analyses']:
        if analysis['visible']:call('analysis.visibility',{'analysisId':analysis['id'],'visible':False})
    bodies=call('document.inspect')['evaluation']['bodies']
    case=next(b for b in bodies if b['ownerId']==owner)
    outer=yface(case,46.5)
    connectors=[next(b for b in bodies if b['name']==name) for name in ('SIDE-ETH-001','SIDE-USB-A-001','SIDE-USB-B-001')]
    faces=[yface(b,45.5) for b in connectors]
    plane=call('construction.plane',{'source':{'support':ref(outer)},'offset':0,'units':'mm','name':'Three-port plane'})['planeId']
    call('sketch.begin',{'planeId':plane,'name':'Three linked port clearances'})
    call('sketch.projectBegin')
    for face in faces:
        selected=call('sketch.projectFace',ref(face))
        assert selected['added']==4
    call('sketch.projectCommit')
    projected_sketch=call('app.state')['activeSession']['draftSketch']['id']
    call('sketch.finish')
    call('document.save',{'path':str(out/'projected-port-rectangles.alloy')})
    call('sketch.edit',{'sketchId':projected_sketch})
    projected=call('app.state')['activeSession']['draftSketch']['parameters']['projections']
    assert len(projected)==12
    groups=[]
    for body in connectors:
        ids=[row['id'] for row in projected if row['source']['bodyId']==body['bodyId']]
        assert len(ids)==4
        call('sketch.tool',{'tool':'offset'})
        preview=call('sketch.offsetPreview',{'sourceEntityIds':ids,'distance':1,'units':'mm'})
        assert preview['pendingClosedRectOffset']['valid']
        accepted=call('sketch.offsetAccept')
        groups.append(accepted['draftSketch']['parameters']['closedRectOffsets'][-1])
    draft=call('app.state')['activeSession']['draftSketch']
    assert len(groups)==3 and len(draft['parameters']['projections'])==12
    assert all(not row['profile'] for row in draft['parameters']['projections'])
    assert len({g['id'] for g in groups})==3
    call('capture.screenshot',{'path':str(out/'three-clearances.png')})
    call('sketch.finish')
    state=call('document.inspect')
    sketch=next(s for s in state['evaluation']['sketches'] if s['featureId']==draft['id'])
    area=sum(profile['areaMm2'] for profile in sketch['profiles'])
    assert math.isclose(area,973.5,abs_tol=1e-5),(area,len(sketch['profiles']))
    print('profiles',len(sketch['profiles']),'union area',area)
    inner=next(face for face in case['supportFaces'] if abs(face['frame']['origin'][1]-43)<1e-6
        and abs(face['frame']['normal'][1])>.99)
    before=next(b for b in state['evaluation']['bodies'] if b['bodyId']==case['bodyId'])['volumeMm3']
    call('solid.extrude',{'sketchId':draft['id'],'profileIds':[profile['profileId'] for profile in sketch['profiles']],
        'extent':'toObject','endSupport':ref(inner),'operation':'cut','targetBodyIds':[case['bodyId']]})
    cut=call('document.inspect')
    after=next(b for b in cut['evaluation']['bodies'] if b['bodyId']==case['bodyId'])['volumeMm3']
    assert after<before and after>0,(before,after)
    call('capture.screenshot',{'path':str(out/'three-port-cut.png')})
    path=out/'three-clearances.alloy';call('document.save',{'path':str(path)})
    call('document.open',{'path':str(path),'discard':True})
    reopened=call('document.inspect')
    retained=next(f for f in reopened['authored']['features'] if f['id']==draft['id'])['parameters']['closedRectOffsets']
    assert [g['id'] for g in retained]==[g['id'] for g in groups]
    call('sketch.edit',{'sketchId':draft['id']})
    before_invalid=call('app.state')['activeSession']['draftSketch']
    call('sketch.closedRectOffsetUpdate',{'offsetId':groups[0]['id'],'distance':-12,'units':'mm'},error='sketch_solve_failed')
    assert call('app.state')['activeSession']['draftSketch']==before_invalid
    call('sketch.closedRectOffsetUpdate',{'offsetId':groups[0]['id'],'distance':1.1,'units':'mm'})
    call('sketch.finish',error='evaluation_failed')
    assert call('document.inspect')['authored']==reopened['authored']
    call('sketch.cancel')
    source={'support':ref(outer)}
    call('construction.plane',{'featureId':plane,'source':source,'offset':.25,'units':'mm'})
    adjusted=call('document.inspect')
    adjusted_group=next(f for f in adjusted['authored']['features'] if f['id']==draft['id'])['parameters']['closedRectOffsets'][0]
    assert adjusted_group['id']==groups[0]['id'] and abs(adjusted_group['distanceMm']-1)<1e-9
    assert next(b for b in adjusted['evaluation']['bodies'] if b['bodyId']==case['bodyId'])['volumeMm3']<before
    call('history.undo')
    assert next(f for f in call('document.inspect')['authored']['features'] if f['id']==plane)['parameters']['offsetMm']==0
    call('history.redo')
    assert abs(next(f for f in call('document.inspect')['authored']['features'] if f['id']==plane)['parameters']['offsetMm']-.25)<1e-9
    call('document.save',{'path':str(path)})
    call('document.open',{'path':str(path),'discard':True})
    assert next(f for f in call('document.inspect')['authored']['features'] if f['id']==draft['id'])['parameters']['closedRectOffsets'][0]['distanceMm']==1
    (out/'ledger.json').write_text(json.dumps(ledger,indent=2))
    (out/'receipt.json').write_text(json.dumps({'planeId':plane,'sketchId':draft['id'],'groupIds':[g['id'] for g in groups],
        'projectedSources':[p['source'] for p in projected],'profileCount':len(sketch['profiles']),
        'unionAreaMm2':area,'savedDesign':str(path)},indent=2))
