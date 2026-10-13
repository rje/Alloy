#!/usr/bin/env python3
"""Actual Centerline selection and screen-derived Revolve gesture through shared authority."""
import argparse,json,math
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,transcript):
    call('sketch.create',{'plane':'XZ','name':'Centerline Revolve fixture'})
    call('sketch.rectangle',{'x1':10,'y1':0,'x2':20,'y2':15,'units':'mm'})
    call('sketch.line',{'x1':0,'y1':-5,'x2':0,'y2':20,'units':'mm'})
    source=call('app.state')['activeSession']['draftSketch'];axis=source['parameters']['entities'][-1]['id'];call('sketch.centerline',{'entityIds':[axis],'centerline':True});call('sketch.finish');call('view.home');before=call('document.inspect')
    s=call('solid.revolveDialog');assert s['previewed'] and s['inputs']['axisEntityId']==axis and s['selectionRole']=='ready'
    call('command.update',{'session':s['id'],'inputs':{'extent':'partial','angleDeg':90}});call('command.preview',{'session':s['id']});state=call('app.state');cue=state['viewport']['gpu']['revolveCue'];assert cue['dragAvailable'];c=cue['centerScreen'];u=cue['basisUScreen'];v=cue['basisVScreen'];r=math.radians(70);target={k:c[k]+math.cos(r)*(u[k]-c[k])+math.sin(r)*(v[k]-c[k]) for k in ('x','y')}
    call('solid.revolveDrag',{**cue['endScreen'],'phase':'begin'});drag=call('solid.revolveDrag',{**target,'phase':'move'});drag=call('solid.revolveDrag',{**target,'phase':'end'});assert math.isclose(drag['inputs']['angleDeg'],70,abs_tol=1e-7) and math.isclose(drag['geometry']['volumeMm3'],875*math.pi,abs_tol=1e-5);call('capture.screenshot',{'path':str(out/'shared-pointer-partial.png')});call('command.cancel',{'session':s['id']});assert call('document.inspect')['authored']==before['authored']
    s=call('solid.revolveDialog');call('command.update',{'session':s['id'],'inputs':{'extent':'full','angleDeg':360}});call('command.preview',{'session':s['id']});call('command.commit',{'session':s['id']});accepted=call('document.inspect');assert math.isclose(accepted['evaluation']['bodies'][0]['volumeMm3'],4500*math.pi,abs_tol=1e-5);path=out/'revolve-controls.alloy';call('document.save',{'path':str(path)});call('document.open',{'path':str(path)});assert call('document.inspect')['authored']==accepted['authored'];call('view.home');call('capture.screenshot',{'path':str(out/'full-committed.png')})
    # Two centerlines deliberately disable inference; no arbitrary axis chosen.
    call('document.new',{'discard':True});call('sketch.create',{'plane':'XZ'});call('sketch.rectangle',{'x1':10,'y1':0,'x2':20,'y2':15,'units':'mm'})
    for x in (0,1):
        call('sketch.line',{'x1':x,'y1':-5,'x2':x,'y2':20,'units':'mm'});axis=call('app.state')['activeSession']['draftSketch']['parameters']['entities'][-1]['id'];call('sketch.centerline',{'entityIds':[axis],'centerline':True})
    call('sketch.finish');s=call('solid.revolveDialog');assert not s['previewed'] and 'axisEntityId' not in s['inputs'] and s['selectionRole']=='profile';call('command.cancel',{'session':s['id']})
    geometry=call('document.inspect')['evaluation']['sketches'][0];call('solid.revolve',{'sketchId':geometry['sketchId'],'profileId':geometry['profiles'][0]['profileId'],'axisEntityId':axis});assert call('app.state')['activeSession']=={};feature=call('document.inspect')['authored']['features'][-1];edit=call('feature.editDialog',{'featureId':feature['id']});assert edit['previewed'];call('command.cancel',{'session':edit['id']});assert call('app.state')['activeSession']=={}
    (out/'report.json').write_text(json.dumps({'passed':True,'scope':'shared pointer-coordinate gesture; native Qt evidence separate','partialAngleDeg':drag['inputs']['angleDeg'],'accepted':accepted},indent=2));(out/'transcript.json').write_text(json.dumps(transcript,indent=2))
