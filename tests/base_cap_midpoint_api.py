#!/usr/bin/env python3
"""Actual front base-cap Project and retained ellipse midpoint after source edit."""
import argparse
import json
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,ledger):
    def inspect():return call('document.inspect')
    def draft():return call('app.state')['activeSession']['draftSketch']
    call('sketch.begin',{'plane':'XZ'})
    call('sketch.rectangle',{'x1':0,'y1':36,'x2':50,'y2':40,'units':'mm'})
    edges=draft()['parameters']['entities']
    call('sketch.constraint',{'type':'fixed','pointIds':[edges[0]['startPointId']]})
    call('sketch.dimension',{'kind':'horizontal','entityId':edges[0]['id'],'value':50,'units':'mm'})
    width_dimension=draft()['parameters']['dimensions'][-1]['id']
    call('sketch.dimension',{'kind':'vertical','entityId':edges[1]['id'],'value':4,'units':'mm'})
    source=draft();source_id=source['id'];call('sketch.finish')
    profile=inspect()['evaluation']['sketches'][0]['profiles'][0]['profileId']
    call('solid.extrude',{'sketchId':source_id,'profileId':profile,'distance':-76,'units':'mm'})
    body=inspect()['evaluation']['bodies'][0]
    base=next(f for f in body['supportFaces'] if f['role']=='extrude.base');assert len(base['edges'])==4
    support={k:base[k] for k in ('producerFeatureId','bodyId','faceId')}
    call('sketch.begin',{'support':support,'name':'Thumb midpoint'})
    call('sketch.projectBegin');call('sketch.projectFace',support);project=call('sketch.projectCommit')
    top=next(l for l in project['sketchGeometry']['projectedLines'] if abs(l['v1Mm']-40)<1e-6 and abs(l['v2Mm']-40)<1e-6)
    call('sketch.centerPointEllipse',{'centerX':25,'centerY':40,'firstX':32.5,'firstY':40,'contourX':25,'contourY':41,'units':'mm',
        'centerSnapTarget':{'kind':'midpoint','entityId':top['id']}})
    ellipse=next(e for e in draft()['parameters']['entities'] if e['type']=='ellipse')
    call('sketch.constraint',{'type':'horizontal','entityIds':[ellipse['firstAxisId']]})
    for axis,value in [(ellipse['firstAxisId'],15),(ellipse['secondAxisId'],2)]:
        call('sketch.dimension',{'kind':'linear','entityId':axis,'value':value,'units':'mm'})
    assert call('app.state')['activeSession']['solverDiagnostics']['dof']==0
    thumb=draft();call('sketch.finish')
    baseline=inspect();call('sketch.edit',{'sketchId':source_id})
    call('sketch.dimensionUpdate',{'dimensionId':width_dimension,'value':74,'units':'mm'})
    call('sketch.finish')
    edited=inspect();geometry=edited['evaluation']['sketches'][-1]
    assert geometry['profiles'], geometry['profileDiagnostics']
    actual=next(e for e in geometry['entities'] if e['id']==ellipse['id'])
    assert abs(actual['centerMm'][0]-37)<1e-6 and abs(actual['centerMm'][2]-40)<1e-6
    assert {e['edgeId'] for e in next(f for f in edited['evaluation']['bodies'][0]['supportFaces'] if f['role']=='extrude.base')['edges']}=={e['edgeId'] for e in base['edges']}
    saved=out/'base-midpoint.alloy';call('document.save',{'path':str(saved)});call('document.open',{'path':str(saved),'discard':True})
    reopened=inspect();assert reopened['authored']==edited['authored']
    assert {e['edgeId'] for e in next(f for f in reopened['evaluation']['bodies'][0]['supportFaces'] if f['role']=='extrude.base')['edges']}=={e['edgeId'] for e in base['edges']}
    call('sketch.edit',{'sketchId':thumb['id']});call('sketch.lookAt');call('capture.screenshot',{'path':str(out/'retained-base-midpoint.png')});call('sketch.finish')
    conic_regions=[p for p in geometry['profiles'] if ellipse['id'] in p['sourceEntityIds'] and p['areaMm2']<30]
    assert conic_regions
    # Both ellipse halves may be selectable when the projected top divides it.
    call('solid.extrude',{'sketchId':thumb['id'],'profileIds':[p['profileId'] for p in conic_regions],
        'start':'offset','startOffset':-10,'distance':5,'units':'mm','operation':'cut','targetBodyIds':[body['bodyId']]})
    cut=inspect()['evaluation']['bodies'][0]
    assert cut['valid'] and cut['closed'] and cut['volumeMm3']<edited['evaluation']['bodies'][0]['volumeMm3']
    call('view.home');call('capture.screenshot',{'path':str(out/'base-thumb-cut.png')})
    (out/'receipt.json').write_text(json.dumps({'baseline':baseline,'edited':edited,'transcript':ledger},indent=2)+'\n')
print('PASS: front base Project and retained top-midpoint ellipse follows source width')
