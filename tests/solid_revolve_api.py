#!/usr/bin/env python3
"""Bounded Revolve API/geometry evidence; native controls are explicitly separate."""
import argparse,json,math
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,transcript):
    def inspect():return call('document.inspect')
    def draft():return call('app.state')['activeSession']['draftSketch']
    def close_sketch():
        s=draft();call('sketch.finish');state=inspect();geometry=next(x for x in state['evaluation']['sketches'] if x['sketchId']==s['id']);assert len(geometry['profiles'])==1;return s,geometry['profiles'][0]['profileId']
    component=call('component.create',{'name':'Revolved fixture'});owner=component['definitionId']
    call('sketch.create',{'plane':'XZ','name':'Radial10–20 height15 — Alloy fixture'})
    call('sketch.rectangle',{'x1':10,'y1':0,'x2':20,'y2':15,'units':'mm'})
    call('sketch.line',{'x1':0,'y1':-5,'x2':0,'y2':20,'units':'mm'});axis=draft()['parameters']['entities'][-1];call('sketch.construction',{'entityIds':[axis['id']],'construction':True})
    sketch,profile=close_sketch();args={'sketchId':sketch['id'],'profileId':profile,'axisEntityId':axis['id']}
    source=inspect();session=call('command.begin',{'command':'solid.revolve','inputs':args});assert isinstance(session['nativeControlsAvailable'], bool)
    preview=call('command.preview',{'session':session['id']});assert preview['previewed'] and math.isclose(preview['geometry']['volumeMm3'],4500*math.pi,abs_tol=1e-5);reserved=(preview['featureId'],preview['bodyId'])
    call('command.update',{'session':session['id'],'inputs':{'extent':'partial','angleDeg':70}});preview=call('command.preview',{'session':session['id']});assert (preview['featureId'],preview['bodyId'])==reserved and math.isclose(preview['geometry']['volumeMm3'],875*math.pi,abs_tol=1e-5)
    call('view.home');call('capture.screenshot',{'path':str(out/'partial-preview.png')});call('command.cancel',{'session':session['id']});assert inspect()['authored']==source['authored'] and inspect()['undo']==source['undo']
    session=call('command.begin',{'command':'solid.revolve','inputs':args});call('command.preview',{'session':session['id']});call('command.commit',{'session':session['id']});full=inspect();body=full['evaluation']['bodies'][0];feature=next(f for f in full['authored']['features'] if f['type']=='revolve');assert body['ownerId']==owner and feature['ownerId']==owner and math.isclose(body['volumeMm3'],4500*math.pi,abs_tol=1e-5)
    edit=call('feature.editDialog',{'featureId':feature['id']});call('command.update',{'session':edit['id'],'inputs':{'extent':'partial','angleDeg':180}});call('command.preview',{'session':edit['id']});call('command.commit',{'session':edit['id']});half=inspect();assert half['evaluation']['bodies'][0]['bodyId']==body['bodyId'] and math.isclose(half['evaluation']['bodies'][0]['volumeMm3'],2250*math.pi,abs_tol=1e-5);call('history.undo');assert inspect()['authored']==full['authored'];call('history.redo');assert inspect()['authored']==half['authored'];half=inspect()
    edit=call('feature.editDialog',{'featureId':feature['id']});call('command.update',{'session':edit['id'],'inputs':{'axisEntityId':call('document.inspect')['authored']['documentId']}});call('command.preview',{'session':edit['id']},error='evaluation_failed');call('command.commit',{'session':edit['id']},error='evaluation_failed');assert inspect()['authored']==half['authored'] and inspect()['revision']==half['revision'];call('command.cancel',{'session':edit['id']})
    call('sketch.edit',{'sketchId':sketch['id']})
    for q in sketch['parameters']['points']:
        if q['id'] in (axis['startPointId'],axis['endPointId']):call('sketch.movePoint',{'pointId':q['id'],'x':1,'y':q['vMm'],'units':'mm'})
    call('sketch.finish');moved=inspect();assert math.isclose(moved['evaluation']['bodies'][0]['volumeMm3'],2100*math.pi,abs_tol=1e-5)
    model=out/'retained-revolve.alloy';call('document.save',{'path':str(model)});call('document.open',{'path':str(model)});assert inspect()['authored']==moved['authored'];call('view.home');call('capture.screenshot',{'path':str(out/'retained-revolve.png')})
    # Independent stepped radial silhouette, values are Alloy fixture choices.
    call('document.new',{'discard':True})
    call('sketch.create',{'plane':'XZ','name':'Stepped profile — Alloy fixture'});coords=[(0,0),(20,0),(20,10),(10,10),(10,20),(0,20),(0,0)]
    for start,end in zip(coords,coords[1:]):call('sketch.line',{'x1':start[0],'y1':start[1],'x2':end[0],'y2':end[1],'units':'mm'})
    stepped=draft();boundary_axis=stepped['parameters']['entities'][-1]['id'];s,profile=close_sketch();call('solid.revolve',{'sketchId':s['id'],'profileId':profile,'axisEntityId':boundary_axis});state=inspect();stepbody=state['evaluation']['bodies'][-1];assert math.isclose(stepbody['volumeMm3'],5000*math.pi,abs_tol=1e-5) and stepbody['valid'];call('document.save',{'path':str(out/'stepped-revolve.alloy')});call('view.home');call('capture.screenshot',{'path':str(out/'stepped-revolve.png')})
    (out/'report.json').write_text(json.dumps({'passed':True,'scope':'shared API and real OCCT geometry; native controls pending216','final':state},indent=2));(out/'transcript.json').write_text(json.dumps(transcript,indent=2))
