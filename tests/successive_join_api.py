#!/usr/bin/env python3
"""Actual-app source Trim and successive Join; native keys remain a separate receipt."""
import argparse,json,math
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);args=p.parse_args();out=Path(args.output).resolve()
with application(args.app,out) as (call,log):
    assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
    session=call('sketch.begin',{'plane':'XY','name':'Support source'})
    def draft():return call('command.preview',{'session':session['id']})['authoredDraft']
    def entity_at(u,v):
        d=draft()['parameters'];pts={p['id']:(p['uMm'],p['vMm']) for p in d['points']}
        for e in d['entities']:
            if e['type']!='line':continue
            a,b=pts[e['startPointId']],pts[e['endPointId']];dx,dy=b[0]-a[0],b[1]-a[1];t=((u-a[0])*dx+(v-a[1])*dy)/(dx*dx+dy*dy)
            if 0<=t<=1 and math.hypot(u-a[0]-t*dx,v-a[1]-t*dy)<1e-5:return e['id']
        raise AssertionError(('missing line',u,v))
    call('sketch.rectangle',{'x1':0,'y1':0,'x2':32,'y2':-10,'units':'mm'})
    call('sketch.line',{'x1':0,'y1':-5,'x2':7.5,'y2':-5,'units':'mm'})
    call('sketch.addCircle',{'centerX':7.5,'centerY':-5,'diameter':15,'units':'mm'})
    for u,v in [(7.5,0),(0,-2.5),(0,-7.5),(4,-5),(7.5,-10)]:call('sketch.trim',{'entityId':entity_at(u,v),'uMm':u,'vMm':v})
    for x in (23,26,29):call('sketch.rectangle',{'x1':x,'y1':0,'x2':x+3,'y2':-10,'units':'mm'})
    call('sketch.finish');initial=call('document.inspect');sketch=initial['evaluation']['sketches'][0];assert len(sketch['profiles'])==5
    ids={}
    for name,u in [('disk',7.5),('bridge',18),('inner',24.5),('middle',27.5),('outer',30.5)]:
        point=call('view.projectSketchPoint',{'plane':'XY','uMm':u,'vMm':-5});picked=call('selection.pick',{'x':point['x'],'y':point['y']});assert picked['kind']=='profile';ids[name]=picked['profileId']
    records=[]
    def stage(name):
        state=call('document.inspect');records.append({'stage':name,'state':state});call('capture.screenshot',{'path':str(out/(name+'.png'))});return state
    call('solid.extrude',{'sketchId':sketch['sketchId'],'profileId':ids['disk'],'distance':20,'units':'mm'})
    base=stage('01-disk20');b0=base['evaluation']['bodies'][0]['bodyId'];cap=base['evaluation']['bodies'][0]['supportFaces'][0]
    inputs={'sketchId':sketch['sketchId'],'profileIds':[ids['bridge'],ids['middle']],'distance':4,'units':'mm','operation':'join'}
    s=call('command.begin',{'command':'solid.extrude','inputs':inputs});preview=call('command.preview',{'session':s['id']});reserved=preview['resultBodyIds'];assert len(reserved)==2
    assert call('command.preview',{'session':s['id']})['resultBodyIds']==reserved
    call('capture.screenshot',{'path':str(out/'02-join4-preview.png')});call('command.cancel',{'session':s['id']});assert call('document.inspect')['authored']==base['authored']
    call('solid.extrude',inputs);intermediate=stage('02-join4');assert intermediate['bodies']==2;b1=next(b['bodyId'] for b in intermediate['evaluation']['bodies'] if b['bodyId']!=b0);join=intermediate['authored']['features'][-1]
    call('solid.extrude',{'sketchId':sketch['sketchId'],'profileId':ids['outer'],'distance':24,'units':'mm','operation':'join'});assert stage('03-outer24')['bodies']==2
    call('solid.extrude',{'sketchId':sketch['sketchId'],'profileId':ids['inner'],'distance':22,'units':'mm','operation':'join'});final=stage('04-inner22');assert final['bodies']==1 and final['evaluation']['bodies'][0]['bodyId']==b0
    body=final['evaluation']['bodies'][0];assert any(f['faceId']==cap['faceId'] and f['producerFeatureId']==cap['producerFeatureId'] for f in body['supportFaces'])
    bridge_area=next(p['areaMm2'] for p in sketch['profiles'] if p['profileId']==ids['bridge']);assert math.isclose(body['volumeMm3'],math.pi*56.25*20+bridge_area*4+30*(4+24+22),rel_tol=1e-8)
    edit=call('feature.editDialog',{'featureId':join['id']});early=call('command.preview',{'session':edit['id']});assert early['evaluatedBodyCount']==2 and len(early['resultBodies'])==2
    assert sorted(early['resultBodyIds'])==sorted(b['bodyId'] for b in intermediate['evaluation']['bodies'])
    call('capture.screenshot',{'path':str(out/'05-history-join4-preview.png')});call('command.cancel',{'session':edit['id']});assert call('document.inspect')['authored']==final['authored']
    call('solid.extrude',{**inputs,'featureId':join['id'],'distance':5});assert call('document.inspect')['evaluation']['bodies'][0]['bodyId']==b0
    call('history.undo');assert call('document.inspect')['authored']==final['authored'];call('history.redo');call('history.undo')
    call('solid.extrude',{**inputs,'featureId':join['id'],'distance':-4},'evaluation_failed');assert call('document.inspect')['authored']==final['authored']
    path=out/'support-successive.alloy';call('document.save',{'path':str(path)});call('document.open',{'path':str(path)});assert call('document.inspect')['authored']==final['authored']
    (out/'report.json').write_text(json.dumps({'passed':True,'stages':records},indent=2));(out/'transcript.json').write_text(json.dumps(log,indent=2))
