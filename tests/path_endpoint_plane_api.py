#!/usr/bin/env python3
"""GEO-003 real-app retained fit-spline Start/End path planes."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application

parser=argparse.ArgumentParser()
parser.add_argument('--app',required=True)
parser.add_argument('--output',required=True)
args=parser.parse_args()
out=Path(args.output).resolve()
out.mkdir(parents=True,exist_ok=True)

with application(args.app,out) as (call,ledger):
    call('window.resize',{'width':1440,'height':900})
    call('sketch.begin',{'plane':'YZ','name':'Shoehorn spine'})
    call('sketch.splineAdd',{'points':[{'uMm':0,'vMm':0},{'uMm':50,'vMm':16},{'uMm':130,'vMm':0}]})
    draft=call('app.state')['activeSession']['draftSketch']
    path_id=draft['id']
    spline=next(e for e in draft['parameters']['entities'] if e['type']=='fitSpline')
    call('sketch.finish')
    def reference(endpoint):
        return {'sketchId':path_id,'entityId':spline['id'],'endpoint':endpoint}
    start=call('construction.pathPlane',{'path':reference(0),'name':'Start section plane'})['planeId']
    end=call('construction.pathPlane',{'path':reference(1),'name':'End section plane'})['planeId']
    evaluation=call('document.inspect')['evaluation']
    frames={p['featureId']:p['frame'] for p in evaluation['constructionPlanes']}
    assert frames[start]['origin']==[0,0,0]
    assert all(abs(a-b)<1e-7 for a,b in zip(frames[end]['origin'],[0,130,0]))
    for frame in frames.values():
        n=frame['normal'];u=frame['uAxis'];v=frame['vAxis']
        assert abs(sum(a*b for a,b in zip(n,u)))<1e-8
        assert abs(sum(a*b for a,b in zip(n,v)))<1e-8
        assert math.isclose(sum(a*a for a in n),1,abs_tol=1e-8)
        assert u[0]>.999999
    call('selection.planeSet',{'planeId':end})
    call('sketch.create',{'planeId':end,'name':'End profile'})
    section_id=call('app.state')['activeSession']['draftSketch']['id']
    call('sketch.rectangle',{'x1':-15,'y1':-5,'x2':15,'y2':5,'units':'mm'})
    call('sketch.finish')
    baseline=call('document.inspect')
    old_normal=frames[end]['normal']
    call('sketch.edit',{'sketchId':path_id})
    call('sketch.movePoint',{'pointId':spline['pointIds'][1],'x':55,'y':30,'units':'mm'})
    call('sketch.finish')
    changed=call('document.inspect')
    changed_frame=next(p['frame'] for p in changed['evaluation']['constructionPlanes'] if p['featureId']==end)
    assert sum((a-b)**2 for a,b in zip(changed_frame['normal'],old_normal))>.0001
    edit_section=call('sketch.edit',{'sketchId':section_id})
    assert call('app.state')['activeSession']['resolvedFrame']==changed_frame
    call('command.cancel',{'session':edit_section['id']})
    call('history.undo')
    assert call('document.inspect')['authored']==baseline['authored']
    call('history.redo')
    assert call('document.inspect')['authored']==changed['authored']
    call('construction.pathPlane',{'featureId':start,'path':reference(1)})
    edited=call('document.inspect')
    assert next(p['frame'] for p in edited['evaluation']['constructionPlanes'] if p['featureId']==start)['origin']==changed_frame['origin']
    call('history.undo')
    assert call('document.inspect')['authored']==changed['authored']
    before=call('document.inspect')['authored']
    call('construction.pathPlane',{'featureId':start,'path':{'sketchId':path_id,
        'entityId':'00000000-0000-0000-0000-000000000001','endpoint':0}},error='evaluation_failed')
    assert call('document.inspect')['authored']==before
    path=out/'endpoint-path-planes.alloy'
    call('document.save',{'path':str(path)})
    call('document.new',{'discard':True})
    call('document.open',{'path':str(path)})
    reopened=call('document.inspect')
    assert reopened['authored']==before
    assert next(p['frame'] for p in reopened['evaluation']['constructionPlanes'] if p['featureId']==end)==changed_frame
    call('view.home')
    call('capture.screenshot',{'path':str(out/'path-planes.png')})
    (out/'report.json').write_text(json.dumps({'passed':True,'startPlaneId':start,'endPlaneId':end,
        'sketchId':path_id,'splineEntityId':spline['id'],'attachedSketchId':section_id,
        'sourceEditMovedNormal':True},indent=2))
    (out/'transcript.json').write_text(json.dumps(ledger,indent=2))
