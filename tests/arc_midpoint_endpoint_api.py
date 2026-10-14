#!/usr/bin/env python3
"""SKT-068 real-app attached arc and line midpoint workflow."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application

parser=argparse.ArgumentParser()
parser.add_argument('--app',required=True)
parser.add_argument('--output',required=True)
args=parser.parse_args()
out=Path(args.output).resolve();out.mkdir(parents=True,exist_ok=True)

with application(args.app,out) as (call,ledger):
    call('window.resize',{'width':1440,'height':1000})
    call('sketch.begin',{'plane':'YZ','name':'Shoehorn spine'})
    call('sketch.splineAdd',{'points':[{'uMm':0,'vMm':0},{'uMm':50,'vMm':16},{'uMm':130,'vMm':0}]})
    path=call('app.state')['activeSession']['draftSketch']
    spline=path['parameters']['entities'][0]
    call('sketch.finish')
    def plane(endpoint):
        return call('construction.pathPlane',{'path':{'sketchId':path['id'],'entityId':spline['id'],
            'endpoint':endpoint},'name':('Start' if endpoint==0 else 'End')+' Plane'})['planeId']
    start_plane,end_plane=plane(0),plane(1)
    call('sketch.begin',{'planeId':start_plane,'name':'Front arc'})
    call('sketch.addArc',{'mode':'threePoint','startX':-25,'startY':10,'throughX':0,'throughY':0,
        'endX':25,'endY':10,'units':'mm'})
    draft=call('app.state')['activeSession']['draftSketch'];arc=draft['parameters']['entities'][0]
    call('sketch.constraint',{'type':'midpoint','entityIds':[arc['id']],'origin':True})
    p=call('app.state')['activeSession']['draftSketch']['parameters'];origin=p['originPointId']
    call('sketch.dimension',{'kind':'horizontal','pointIds':[arc['startPointId'],arc['endPointId']],
        'value':50,'units':'mm'})
    for endpoint in ('startPointId','endPointId'):
        call('sketch.dimension',{'kind':'vertical','pointIds':[origin,arc[endpoint]],
            'value':10,'units':'mm'})
    front=call('app.state')['activeSession']['draftSketch']
    front_id=front['id']
    call('sketch.finish')
    call('sketch.begin',{'planeId':end_plane,'name':'Rear line'})
    call('sketch.line',{'x1':-15,'y1':0,'x2':15,'y2':0,'units':'mm'})
    last=call('app.state')['activeSession']['draftSketch'];line=last['parameters']['entities'][0]
    call('sketch.constraint',{'type':'midpoint','entityIds':[line['id']],'origin':True})
    call('sketch.constraint',{'type':'horizontal','entityIds':[line['id']]})
    call('sketch.dimension',{'kind':'linear','pointIds':[line['startPointId'],line['endPointId']],
        'value':30,'units':'mm'})
    rear_id=call('app.state')['activeSession']['draftSketch']['id']
    call('sketch.finish')
    def inspected(): return call('document.inspect')
    baseline=inspected()
    features={f['id']:f for f in baseline['authored']['features']}
    front=features[front_id];params=front['parameters'];arc=next(e for e in params['entities'] if e['type']=='arc')
    points={p['id']:p for p in params['points']};center=points[arc['centerPointId']];target=points[params['originPointId']]
    half=math.radians(arc['startAngleDeg']+arc['sweepAngleDeg']/2)
    assert math.isclose(target['uMm'],center['uMm']+arc['radiusMm']*math.cos(half),abs_tol=1e-6)
    assert math.isclose(target['vMm'],center['vMm']+arc['radiusMm']*math.sin(half),abs_tol=1e-6)
    assert len([c for c in params['constraints'] if c['type']=='midpoint' and c['entityIds']==[arc['id']]])==1
    frames={p['featureId']:p['frame'] for p in baseline['evaluation']['constructionPlanes']}
    call('sketch.edit',{'sketchId':path['id']})
    call('sketch.movePoint',{'pointId':spline['pointIds'][1],'x':55,'y':30,'units':'mm'})
    call('sketch.finish')
    changed=inspected()
    moved={p['featureId']:p['frame'] for p in changed['evaluation']['constructionPlanes']}
    assert moved[start_plane]['normal']!=frames[start_plane]['normal']
    assert moved[end_plane]['normal']!=frames[end_plane]['normal']
    for section_id,plane_id in ((front_id,start_plane),(rear_id,end_plane)):
        edit=call('sketch.edit',{'sketchId':section_id})
        assert call('app.state')['activeSession']['resolvedFrame']==moved[plane_id]
        call('command.cancel',{'session':edit['id']})
    call('history.undo')
    assert inspected()['authored']==baseline['authored']
    call('history.redo')
    assert inspected()['authored']==changed['authored']
    saved=out/'arc-midpoint-endpoints.alloy'
    call('document.save',{'path':str(saved)})
    call('document.new',{'discard':True})
    call('document.open',{'path':str(saved)})
    assert inspected()['authored']==changed['authored']
    (out/'report.json').write_text(json.dumps({'passed':True,'startPlaneId':start_plane,
        'endPlaneId':end_plane,'frontArcSketchId':front_id,'rearLineSketchId':rear_id,
        'arcMidpointMm':[target['uMm'],target['vMm']],'sourceEditFollowed':True},indent=2))
    (out/'transcript.json').write_text(json.dumps(ledger,indent=2))
