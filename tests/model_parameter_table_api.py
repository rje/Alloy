#!/usr/bin/env python3
"""Real-app after-authoring model expression table on the sliding-box prefix."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application

p=argparse.ArgumentParser()
p.add_argument('--app',required=True)
p.add_argument('--output',required=True)
a=p.parse_args()
out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)

def ref(face): return {key:face[key] for key in ('producerFeatureId','bodyId','faceId')}
def near(a,b): assert math.isclose(a,b,abs_tol=1e-6), (a,b)

with application(a.app,out) as (call,transcript):
    def inspect(): return call('document.inspect')
    def model(name,kind):
        matches=[row for row in inspect()['modelParameters'] if row['name']==name and row['featureType']==kind]
        assert len(matches)==1,(name,kind,matches)
        return matches[0]
    def bind(name,kind,expression):
        target=model(name,kind)['target']
        call('document.modelParameterSet',{'target':target,'expression':expression})
        return model(name,kind)
    assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
    box_owner=call('component.create',{'name':'Box'})['definitionId']
    call('component.activate',{'definitionId':box_owner})
    call('solid.box',{'width':50,'depth':80,'height':40,'units':'mm','name':'Box'})
    state=inspect();box=state['authored']['features'][-1]
    body=state['evaluation']['bodies'][0]
    top=next(f for f in body['supportFaces'] if f['role']=='box.z1')
    call('solid.shell',{'targetBodyId':body['bodyId'],'removedFace':ref(top),'thicknessMm':4})
    body=inspect()['evaluation']['bodies'][0]
    back=next(f for f in body['supportFaces'] if f['role']=='shell-inner' and abs(f['frame']['origin'][1]-76)<1e-6)
    slant=call('sketch.begin',{'plane':'XY','name':'Slanted length'})['draftSketch']['id']
    call('sketch.line',{'x1':100,'y1':0,'x2':102,'y2':3.464101615,'units':'mm'})
    line=call('app.state')['activeSession']['draftSketch']['parameters']['entities'][0]
    call('sketch.dimension',{'kind':'linear','entityId':line['id'],'value':4,'units':'mm'})
    call('sketch.finish')
    cutter=call('sketch.begin',{'plane':'XZ','name':'Front cutter'})['draftSketch']['id']
    call('sketch.rectangle',{'x1':4,'y1':36,'x2':46,'y2':42,'units':'mm'})
    call('sketch.finish')
    profile=inspect()['evaluation']['sketches'][-1]['profiles'][0]['profileId']
    call('solid.extrude',{'sketchId':cutter,'profileId':profile,'extent':'toObject',
                          'endSupport':ref(back),'endOffset':0.25,'operation':'cut',
                          'targetBodyIds':[body['bodyId']]})
    # A separately authored literal chain checks the table's nested Offset row.
    call('sketch.begin',{'plane':'XY','name':'Lid clearance'})
    chain=((100,10,104,10),(104,10,106,0),(106,0,118,0),
           (118,0,120,10),(120,10,124,10),(104,10,120,10))
    for x1,y1,x2,y2 in chain:
        call('sketch.line',{'x1':x1,'y1':y1,'x2':x2,'y2':y2,'units':'mm'})
    line_ids=[e['id'] for e in call('app.state')['activeSession']['draftSketch']['parameters']['entities']]
    call('sketch.tool',{'tool':'offset'})
    call('sketch.offsetPreview',{'sourceEntityIds':line_ids[:5],
        'topEntityId':line_ids[5],'distance':-0.25,'units':'mm'})
    call('sketch.offsetAccept')
    call('sketch.finish')
    source=inspect()
    assert source['authored']['features'][0]['id']==box['id']
    assert not any('ParameterId' in key for feature in source['authored']['features']
                   for key in feature['parameters'])
    extrude=next(f for f in source['authored']['features'] if f['type']=='extrude')
    assert 'distanceMm' not in extrude['parameters']
    assert len(source['modelParameters'])==7 # Box 3, Shell, slanted length, end and chain clearance
    for name,value in [('Length',80),('Width',50),('Height',40),('Thickness',4),('Clearance',0.25)]:
        call('document.parameterAdd',{'name':name,'value':value,'units':'mm'})
    ids={row['name']:row['id'] for row in inspect()['userParameters']}
    for label,kind,expression in [('Width','box','Width'),('Depth','box','Length'),
                                  ('Height','box','Height'),('Thickness','shell','Thickness'),
                                  ('linear Dimension','sketch','Thickness'),('End Offset','extrude','Clearance'),
                                  ('Chain Offset','sketch','Clearance')]:
        row=bind(label,kind,expression)
        assert row['parameterId']==ids[expression] and row['sourceName']==expression
    bound=inspect()
    assert bound['authored']['features'][0]['id']==box['id']
    near(model('End Offset','extrude')['valueMm'],0.25)
    near(model('Chain Offset','sketch')['valueMm'],-0.25)
    call('document.parametersDialog')
    call('capture.dialog',{'path':str(out/'bound-model-table.png')})
    call('document.parametersDialog',{'visible':False})
    assert not model('Width','box')['target']==model('Depth','box')['target']
    baseline=inspect()['authored']
    call('document.modelParameterSet',{'target':model('Width','box')['target'],'expression':'Missing'},error='invalid_model_expression')
    assert inspect()['authored']==baseline
    call('document.modelParameterSet',{'target':model('Width','box')['target'],'expression':'50 mm'})
    assert 'parameterId' not in model('Width','box')
    bind('Width','box','Width')
    for name,value in [('Length',40),('Width',74),('Height',100),('Thickness',3.5),('Clearance',0.5)]:
        call('document.parameterUpdate',{'parameterId':ids[name],'value':value})
    edited=inspect()
    near(model('Depth','box')['valueMm'],40)
    near(model('Width','box')['valueMm'],74)
    near(model('Thickness','shell')['valueMm'],3.5)
    near(model('linear Dimension','sketch')['valueMm'],3.5)
    near(model('End Offset','extrude')['valueMm'],0.5)
    near(model('Chain Offset','sketch')['valueMm'],-0.5)
    near(edited['evaluation']['bodies'][0]['boundsMm']['size'][0],74)
    assert next(f for f in edited['authored']['features'] if f['type']=='extrude')['parameters']['endSupport']==ref(back)
    call('history.undo');call('history.redo')
    assert inspect()['authored']==edited['authored']
    path=out/'model-table.alloy'
    call('document.save',{'path':str(path)})
    call('document.open',{'path':str(path),'discard':True})
    assert inspect()['authored']==edited['authored']
    call('view.home')
    call('capture.screenshot',{'path':str(out/'rebound-box.png')})
    (out/'result.json').write_text(json.dumps({'source':source,'bound':bound,'edited':edited,'transcript':transcript},indent=2))
print('PASS: numeric Box/Shell/slanted length/To Object end offset rebound after authoring; edits, refusal and reopen')
