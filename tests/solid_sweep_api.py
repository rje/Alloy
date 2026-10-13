#!/usr/bin/env python3
"""Retained Sweep kernel/API foundation; analytic seeds do not qualify sketch gestures."""
import argparse,json,math
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,transcript):
    def inspect():return call('document.inspect')
    def draft():return call('app.state')['activeSession']['draftSketch']
    call('sketch.create',{'plane':'XZ','name':'Circular section'});call('sketch.addCircle',{'centerX':0,'centerY':0,'diameter':2,'units':'mm'});profile=draft();call('sketch.finish');profile_id=inspect()['evaluation']['sketches'][0]['profiles'][0]['profileId']
    call('sketch.create',{'plane':'XY','name':'Analytic path foundation'})
    def line(x1,y1,x2,y2):call('sketch.line',{'x1':x1,'y1':y1,'x2':x2,'y2':y2,'units':'mm'})
    def arc(x,y,r,angle):call('sketch.addArc',{'mode':'center','centerX':x,'centerY':y,'radius':r,'startAngleDeg':angle,'sweepAngleDeg':-90,'units':'mm'})
    line(0,0,0,33);arc(7,33,7,180);arc(7,33,7,90);line(14,33,14,-14.5);arc(8.5,-14.5,5.5,0);arc(8.5,-14.5,5.5,-90);line(3,-14.5,3,20);arc(7,20,4,180);arc(7,20,4,90);line(11,20,11,0);path=draft();call('sketch.finish')
    args={'profileSketchId':profile['id'],'profileId':profile_id,'pathSketchId':path['id'],'pathEntityIds':[e['id'] for e in path['parameters']['entities']],'pathStartPointId':path['parameters']['entities'][0]['startPointId']}
    before=inspect();length=135+16.5*math.pi
    def verify(geometry,radius=1,extra=0):assert geometry['valid'] and geometry['closed'] and geometry['topology']['solids']==1 and math.isclose(geometry['pathLengthMm'],length+extra,abs_tol=1e-7) and math.isclose(geometry['volumeMm3'],math.pi*radius**2*(length+extra),abs_tol=1e-5)
    session=call('command.begin',{'command':'solid.sweep','inputs':args});assert not session['nativeControlsAvailable'];preview=call('command.preview',{'session':session['id']});verify(preview['geometry']);assert inspect()['authored']==before['authored'];call('view.home');call('capture.screenshot',{'path':str(out/'sweep-preview.png')});call('command.cancel',{'session':session['id']});assert inspect()['authored']==before['authored'] and inspect()['undo']==before['undo']
    session=call('command.begin',{'command':'solid.sweep','inputs':args});preview=call('command.preview',{'session':session['id']});body_id=preview['bodyId'];feature_id=preview['featureId'];call('command.commit',{'session':session['id']});accepted=inspect();body=accepted['evaluation']['bodies'][0];verify(body);assert body['bodyId']==body_id and body['featureId']==feature_id and body['ownerId']==accepted['authored']['documentId'];assert call('app.state')['activeSession']=={};call('history.undo');assert inspect()['authored']==before['authored'];call('history.redo');assert inspect()['authored']==accepted['authored']
    session=call('feature.editDialog',{'featureId':feature_id});preview=call('command.preview',{'session':session['id']});assert preview['bodyId']==body_id;call('command.cancel',{'session':session['id']});assert inspect()['authored']==accepted['authored']
    bad_before=inspect();bad={**args,'featureId':feature_id,'pathEntityIds':['00000000-0000-4000-8000-000000000001']+args['pathEntityIds'][1:]};error=call('solid.sweep',bad,error='evaluation_failed');assert inspect()['authored']==accepted['authored'] and inspect()['revision']==bad_before['revision'] and inspect()['undo']==bad_before['undo']
    pp=profile['parameters'];pp['entities'][0]['radiusMm']=.8;call('feature.edit',{'featureId':profile['id'],'parameters':pp});points=path['parameters']['points'];end=path['parameters']['entities'][-1]['endPointId'];next(q for q in points if q['id']==end)['vMm']=-2;call('feature.edit',{'featureId':path['id'],'parameters':{'points':points}});edited=inspect();verify(edited['evaluation']['bodies'][0],.8,2);assert edited['evaluation']['bodies'][0]['bodyId']==body_id and [f['faceId'] for f in edited['evaluation']['bodies'][0]['supportFaces']]==[f['faceId'] for f in body['supportFaces']]
    file=out/'sweep-foundation.alloy';call('document.save',{'path':str(file)});call('document.open',{'path':str(file)});assert inspect()['authored']==edited['authored'];call('view.home');call('capture.screenshot',{'path':str(out/'sweep-committed.png')});state=call('app.state')
    # Direct command must leave no session and keep feature/body identity.
    call('solid.sweep',{**args,'featureId':feature_id});assert call('app.state')['activeSession']=={};assert inspect()['evaluation']['bodies'][0]['bodyId']==body_id
    (out/'report.json').write_text(json.dumps({'passed':True,'scope':'kernel/shared API only; analytic sketch seeds; native Sweep controls and full source workflow pending','pathLengthMm':length,'initialVolumeMm3':length*math.pi,'accepted':accepted,'edited':edited,'gpu':state['viewport']},indent=2));(out/'transcript.json').write_text(json.dumps(transcript,indent=2))
