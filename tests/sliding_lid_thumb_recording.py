#!/usr/bin/env python3
"""Focused visible thumb-relief replay from the completed actual tutorial model."""
import argparse
from contextlib import ExitStack
import json
import math
from pathlib import Path
import time
from app_harness import application

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--app',required=True)
p.add_argument('--source',required=True)
p.add_argument('--output',required=True)
a=p.parse_args();out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
assert not out.is_relative_to(Path(__file__).resolve().parents[1])
with application(a.app,out) as (raw,transcript), ExitStack() as cleanup:
    raw('window.resize',{'width':1400,'height':1000})
    raw('document.open',{'path':str(Path(a.source).resolve()),'discard':True})
    source=raw('document.inspect')
    thumb=next(f for f in source['authored']['features'] if f['name']=='Thumb relief')
    cut=source['authored']['features'][-1]
    assert cut['type']=='extrude' and cut['parameters']['sketchId']==thumb['id']
    # Restore the actual pre-thumb modeling prefix through real document commands.
    raw('feature.delete',{'featureId':cut['id']})
    raw('feature.delete',{'featureId':thumb['id']})
    prefix=raw('document.inspect')
    assert prefix['authored']['features']==source['authored']['features'][:-2]
    owner=thumb['ownerId']
    body=next(b for b in prefix['evaluation']['bodies'] if b['ownerId']==owner)
    base=next(f for f in body['supportFaces'] if f['role']=='extrude.base')
    support={k:base[k] for k in ('producerFeatureId','bodyId','faceId')}
    active=False;recording={}
    def stop():
        global active
        if active:
            active=False;recording.update(raw('recording.stop'))
    cleanup.callback(stop)
    raw('recording.start',{'path':str(out/'thumb-relief-focused-replay.mp4'),'fps':5});active=True
    def call(c,args=None):
        r=raw(c,args)
        if c not in ('app.state','document.inspect'):time.sleep(.45)
        return r
    def stage(t):
        raw('recording.step',{'text':'API | Focused replay: '+t});time.sleep(.8)
    def draft():return raw('app.state')['activeSession']['draftSketch']
    stage('thumb relief on the completed, resized Box and Lid')
    call('component.activate',{'definitionId':owner})
    call('sketch.begin',{'support':support,'name':'Thumb relief'})
    call('sketch.projectBegin');call('sketch.projectFace',support)
    projected=call('sketch.projectCommit')
    # The retained front lies at world Z100. Fit its projected geometry before drawing.
    call('sketch.lookAt')
    top=next(l for l in projected['sketchGeometry']['projectedLines']
             if abs(l['v1Mm']-100)<1e-5 and abs(l['v2Mm']-100)<1e-5)
    center=(top['u1Mm']+top['u2Mm'])/2
    call('view.zoom',{'factor':2.5})
    point=raw('view.projectSketchPoint',{'sketchId':draft()['id'],'uMm':center,'vMm':100})
    viewport=raw('app.state')['viewport']
    call('view.pan',{'x':viewport['width']/2-point['x'],'y':viewport['height']/2-point['y']})
    point=raw('view.projectSketchPoint',{'sketchId':draft()['id'],'uMm':center,'vMm':100})
    assert 100<point['x']<viewport['width']-100 and 100<point['y']<viewport['height']-100
    stage('midpoint ellipse; horizontal axis; dimensions 15 x 2 mm')
    call('sketch.centerPointEllipse',{'centerX':center,'centerY':100,'firstX':center+7.5,
        'firstY':100,'contourX':center,'contourY':101,'units':'mm',
        'centerSnapTarget':{'kind':'midpoint','entityId':top['id']}})
    e=next(e for e in draft()['parameters']['entities'] if e['type']=='ellipse')
    call('sketch.constraint',{'type':'horizontal','entityIds':[e['firstAxisId']]})
    for axis,value in ((e['firstAxisId'],15),(e['secondAxisId'],2)):
        call('sketch.dimension',{'kind':'linear','entityId':axis,'value':value,'units':'mm'})
    assert raw('app.state')['activeSession']['solverDiagnostics']['dof']==0
    sketch_id=draft()['id']
    call('capture.screenshot',{'path':str(out/'thumb-visible.png')});time.sleep(2)
    call('sketch.finish')
    before=raw('document.inspect')
    sketch=next(s for s in before['evaluation']['sketches'] if s['featureId']==sketch_id)
    profiles=[x['profileId'] for x in sketch['profiles'] if e['id'] in x['sourceEntityIds'] and x['areaMm2']<30]
    assert profiles
    stage('Cut: Start Offset -10 mm; Distance 5 mm')
    call('solid.extrude',{'sketchId':sketch_id,'profileIds':profiles,'start':'offset',
        'startOffset':-10,'distance':5,'units':'mm','operation':'cut','targetBodyIds':[body['bodyId']]})
    final=raw('document.inspect')
    actual=next(b for b in final['evaluation']['bodies'] if b['ownerId']==owner)
    expected=next(b for b in source['evaluation']['bodies'] if b['ownerId']==owner)
    assert actual['valid'] and actual['closed']
    assert math.isclose(actual['volumeMm3'],expected['volumeMm3'],abs_tol=1e-6)
    assert math.isclose(body['volumeMm3']-actual['volumeMm3'],math.pi*7.5*5/2,abs_tol=1e-3)
    call('view.home');call('selection.set',{'bodyId':''})
    call('capture.screenshot',{'path':str(out/'thumb-cut-visible.png')})
    stage('same final Lid volume; source model remains untouched')
    call('document.save',{'path':str(out/'focused-replay.alloy')});time.sleep(2)
    stop()
    (out/'receipt.json').write_text(json.dumps({'scope':'Focused replay, not a second full creation run',
        'sourcePath':str(Path(a.source).resolve()),'source':source,'prefix':prefix,'final':final,
        'recording':recording,'transcript':transcript},indent=2)+'\n')
print('PASS: visible thumb creation and matching actual Lid volume')
