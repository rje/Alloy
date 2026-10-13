#!/usr/bin/env python3
"""Shared additive feature-pattern session from a clean design; no native-input claim."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application

p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,transcript):
    call('sketch.begin',{'plane':'XY','name':'Base'});call('sketch.rectangle',{'x1':0,'y1':0,'x2':31.8,'y2':15.8,'units':'mm'});call('sketch.finish')
    sketch=call('document.inspect')['evaluation']['sketches'][0]
    call('solid.extrude',{'sketchId':sketch['sketchId'],'profileId':sketch['profiles'][0]['profileId'],'distance':9.6,'units':'mm'})
    body=call('document.inspect')['evaluation']['bodies'][0];target=body['bodyId'];cap=body['supportFaces'][0];support={key:cap[key] for key in ('producerFeatureId','bodyId','faceId')}
    call('sketch.begin',{'support':support,'name':'Stud'});call('sketch.addCircle',{'centerX':3.9,'centerY':3.9,'diameter':5,'units':'mm'});call('sketch.finish');stud=call('document.inspect')['evaluation']['sketches'][-1]
    extrusion={'sketchId':stud['sketchId'],'profileId':stud['profiles'][0]['profileId'],'distance':1.7,'operation':'join','targetBodyIds':[target],'units':'mm'}
    call('solid.extrude',extrusion);before=call('document.inspect');seed=before['authored']['features'][-1];body=before['evaluation']['bodies'][0]
    def axis(coordinate,quantity,extent):
        for edge in body['solidEdges']:
            if not edge['available']:continue
            vector=[edge['endMm'][i]-edge['startMm'][i] for i in range(3)];length=math.sqrt(sum(v*v for v in vector))
            if abs(vector[coordinate])>1 and math.isclose(abs(vector[coordinate]),length,abs_tol=1e-6):return {'edgeId':edge['edgeId'],'quantity':quantity,'extentMm':math.copysign(extent,vector[coordinate])}
        raise AssertionError('Missing retained straight axis')
    axes=[axis(0,4,24),axis(1,2,8)];inputs={'sourceFeatureId':seed['id'],'targetBodyId':target,'axes':axes,'name':'Stud pattern'}
    call('view.home');session=call('solid.rectangularPatternDialog');call('selection.featureSet',{'featureId':seed['id']})
    for i,source_axis in enumerate(axes):
        call('selection.patternAxisSet',{'edgeId':source_axis['edgeId'],'axis':i})
        call('command.update',{'session':session['id'],'inputs':{'axes':axes[:i+1]}})
        preview=call('command.preview',{'session':session['id']})
    assert preview['geometry']['instanceCount']==8 and preview['bodyId']==target
    assert call('app.state')['selection']['kind']=='featurePattern'
    assert call('command.preview',{'session':session['id']})['featureId']==preview['featureId']
    call('capture.screenshot',{'path':str(out/'eight-stud-preview.png')});call('capture.dialog',{'path':str(out/'pattern-panel.png')})
    call('command.cancel',{'session':session['id']});assert call('document.inspect')['authored']==before['authored']
    session=call('command.begin',{'command':'solid.rectangularPattern','inputs':inputs});preview=call('command.preview',{'session':session['id']});call('command.commit',{'session':session['id']})
    patterned=call('document.inspect');assert patterned['bodies']==1 and patterned['evaluation']['bodies'][0]['bodyId']==target
    geometry=patterned['evaluation']['bodies'][0];assert geometry['instanceCount']==8 and geometry['valid'] and geometry['closed']
    centers=sorted(tuple(round(v,6) for v in instance['centerMm'][:2]) for instance in geometry['instances'])
    assert centers==sorted((round(3.9+8*i,6),round(3.9+8*j,6)) for j in range(2) for i in range(4))
    rims=[e['edgeId'] for e in geometry['solidEdges'] if e['available'] and e['provenance']['sourceSketchId']==stud['sketchId'] and e['provenance']['role']=='endRim'];assert len(set(rims))==8
    feature=patterned['authored']['features'][-1];assert feature['id']==preview['featureId']
    edit=call('feature.editDialog',{'featureId':feature['id']});assert call('command.preview',{'session':edit['id']})['bodyId']==target;call('command.cancel',{'session':edit['id']});assert call('document.inspect')['authored']==patterned['authored']
    call('solid.extrude',dict(extrusion,featureId=seed['id'],distance=2));edited=call('document.inspect');assert edited['evaluation']['bodies'][0]['instanceCount']==8
    expected=31.8*15.8*9.6+8*math.pi*2.5**2*2;assert math.isclose(edited['evaluation']['bodies'][0]['volumeMm3'],expected,abs_tol=1e-5)
    call('history.undo');assert call('document.inspect')['authored']==patterned['authored'];call('history.redo');assert call('document.inspect')['authored']==edited['authored']
    call('view.home');call('capture.screenshot',{'path':str(out/'eight-studs.png')})
    path=out/'stud-pattern.alloy';call('document.save',{'path':str(path)});call('document.new',{'discard':True});call('document.open',{'path':str(path),'discard':True});assert call('document.inspect')['authored']==edited['authored']
(out/'pattern-api-ledger.json').write_text(json.dumps({'before':before,'patterned':patterned,'edited':edited,'transcript':transcript},indent=2))
print('PASS: feature/axis selection,8studpreview/cancel/commit,seededit,stableIDs/history/storage')
