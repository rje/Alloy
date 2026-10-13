#!/usr/bin/env python3
"""Bounded straw-bung Join checkpoint through the live Alloy command authority."""
import argparse,json,math,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools'))
from alloyctl import choose_endpoint,request
p=argparse.ArgumentParser();p.add_argument('--ready-file',required=True);p.add_argument('--output',required=True);a=p.parse_args()
out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True);ep=choose_endpoint(ready_file=a.ready_file);transcript=[]
def call(c,args=None):
 r=request(ep,c,args,timeout=30);transcript.append({'command':c,'args':args or {},'response':r});assert r['ok'],r;return r['result']
def inspect():return call('document.inspect')
def circle(d,support=None):
 call('sketch.begin',{'support':support} if support else {'plane':'XY'})
 call('sketch.addCircle',{'centerX':0,'centerY':0,'diameter':d,'units':'mm'});call('sketch.finish')
 doc=inspect();sk=doc['evaluation']['sketches'][-1];return sk['sketchId'],sk['profiles'][0]['profileId']
def cap(body):return {k:body['supportFaces'][0][k] for k in ('producerFeatureId','bodyId','faceId')}
result={'status':'failed','transcript':transcript}
try:
 call('document.new',{'discard':True});s,pid=circle(26)
 call('solid.extrude',{'sketchId':s,'profileId':pid,'distance':3,'units':'mm'});base=inspect()['evaluation']['bodies'][0];target=base['bodyId']
 s,pid=circle(19,cap(base));before=inspect()['authored']
 call('selection.profileSet',{'sketchId':s,'profileId':pid});session=call('solid.extrudeDialog')
 call('command.update',{'session':session['id'],'inputs':{'operation':'join','distance':8}})
 preview=call('command.preview',{'session':session['id']});assert preview['bodyId']==target and preview['geometryEvaluated']
 call('view.home');call('capture.screenshot',{'path':str(out/'join-preview.png')});call('command.cancel',{'session':session['id']});assert inspect()['authored']==before
 session=call('solid.extrudeDialog',{'sketchId':s,'profileId':pid,'operation':'join'})
 call('command.update',{'session':session['id'],'inputs':{'distance':8}});call('command.preview',{'session':session['id']});call('command.commit',{'session':session['id']})
 doc=inspect();joined=doc['authored'];join=joined['features'][-1];body=doc['evaluation']['bodies'][0]
 assert doc['bodies']==1 and body['bodyId']==target and body['featureId']==base['featureId'] and 'bodyId' not in join
 assert body['valid'] and body['closed'] and math.isclose(body['volumeMm3'],math.pi*(169*3+90.25*8),rel_tol=1e-9)
 assert cap(body)['producerFeatureId']==join['id'];call('view.home');call('capture.screenshot',{'path':str(out/'joined-bung.png')})
 call('history.undo');assert inspect()['authored']==before;call('history.redo');assert inspect()['authored']==joined
 third,third_pid=circle(6.2,cap(body));call('solid.extrude',{'sketchId':third,'profileId':third_pid,'distance':30});assert inspect()['bodies']==2
 call('solid.extrude',{'featureId':join['id'],'sketchId':s,'profileId':pid,'distance':10});doc=inspect();assert doc['bodies']==2
 assert math.isclose(doc['evaluation']['bodies'][0]['volumeMm3'],math.pi*(169*3+90.25*10),rel_tol=1e-9)
 call('view.home');call('capture.screenshot',{'path':str(out/'edited-bung-tool.png')})
 stable=doc['authored'];call('document.save',{'path':str(out/'straw-bung-join.alloy')});call('document.new',{'discard':True});call('document.open',{'path':str(out/'straw-bung-join.alloy'),'discard':True});assert inspect()['authored']==stable
 result.update(status='passed',covers=['Join shared dialog preview/cancel/commit','one retained target','third circle on joined cap','upstream Join distance and tool recompute','exact undo/redo and native save/reopen'],initialGpu=call('app.state'))
finally:(out/'profile-join.json').write_text(json.dumps(result,indent=2)+'\n')
