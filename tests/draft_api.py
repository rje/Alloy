#!/usr/bin/env python3
"""SOL-086 bounded one-face Draft through the real local application."""
import argparse,json,math,uuid
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
def ref(f):return {k:f[k] for k in ('producerFeatureId','bodyId','faceId')}
def near(a,b,t=1e-5):assert math.isclose(a,b,abs_tol=t,rel_tol=1e-9),(a,b)
def expected(angle,flip=False):return 16000-( -1 if flip else 1)*8000*math.tan(math.radians(angle))
def checkbody(b,angle,cut=False):
 assert b['valid'] and b['closed'] and b['topology']['solids']==1
 near(b['volumeMm3'],expected(angle)-(8*math.pi if cut else 0))
 f=next(f for f in b['supportFaces'] if f['role']=='draft.side');n=f['frame']['normal'];near(n[0],0);near(n[1],-math.cos(math.radians(angle)));near(n[2],math.sin(math.radians(angle)));return f
with application(a.app,out) as (call,log):
 call('solid.box',{'width':40,'depth':20,'height':20,'units':'mm','name':'Draft stock'})
 call('construction.plane',{'source':{'plane':'XY'},'offset':0,'units':'mm','name':'Neutral'})
 initial=call('document.inspect');b=initial['evaluation']['bodies'][0];source=b['bodyId'];front=next(f for f in b['supportFaces'] if f['role']=='box.y0');plane=initial['authored']['features'][-1]['id'];args={'targetBodyId':source,'face':ref(front),'neutralPlaneId':plane,'angleDeg':5,'flipPullDirection':False}
 session=call('command.begin',{'command':'solid.draft','inputs':{}});sid=session['id'];call('command.update',{'session':sid,'inputs':args});preview=call('command.preview',{'session':sid});assert preview['previewed'];checkbody(preview['geometry'],5)
 for angle,flip in [(-5,False),(5,True),(-5,True)]:
  call('command.update',{'session':sid,'inputs':{'angleDeg':angle,'flipPullDirection':flip}});g=call('command.preview',{'session':sid})['geometry'];near(g['volumeMm3'],expected(angle,flip))
 assert call('document.inspect')['authored']==initial['authored'];call('command.cancel',{'session':sid});cancel=call('document.inspect');assert cancel['authored']==initial['authored'] and cancel['undo']==initial['undo'] and cancel['dirty']==initial['dirty']
 call('solid.draft',args);created=call('document.inspect');f=checkbody(created['evaluation']['bodies'][0],5);draftid=created['authored']['features'][-1]['id'];assert len(created['authored']['features'])==3
 call('history.undo');assert call('document.inspect')['authored']==initial['authored'];call('history.redo');assert call('document.inspect')['authored']==created['authored']
 ss=call('sketch.begin',{'support':ref(f),'name':'Drafted wall bore'});frame=ss['resolvedFrame'];center=[20,10*math.tan(math.radians(5)),10];delta=[a-b for a,b in zip(center,frame['origin'])];u=sum(a*b for a,b in zip(delta,frame['uAxis']));v=sum(a*b for a,b in zip(delta,frame['vAxis']));call('sketch.addCircle',{'centerX':u,'centerY':v,'diameter':4,'units':'mm'});call('sketch.finish');attached=call('document.inspect');sketch=attached['evaluation']['sketches'][-1];call('solid.extrude',{'sketchId':sketch['featureId'],'profileId':sketch['profiles'][0]['profileId'],'distance':-2,'units':'mm','operation':'cut','targetBodyIds':[source]});downstream=call('document.inspect');checkbody(downstream['evaluation']['bodies'][0],5,True)
 edit=call('feature.editDialog',{'featureId':draftid});assert edit['inputs']['angleDeg']==5;call('command.update',{'session':edit['id'],'inputs':{'angleDeg':3}});ep=call('command.preview',{'session':edit['id']});checkbody(ep['geometry'],3);call('command.commit',{'session':edit['id']});edited=call('document.inspect');ef=checkbody(edited['evaluation']['bodies'][0],3,True);assert ref(ef)==ref(f);assert edited['authored']['features'][3:]==downstream['authored']['features'][3:];assert edited['evaluation']['sketches'][-1]['frame']==ef['frame']
 call('history.undo');assert call('document.inspect')['authored']==downstream['authored'];call('history.redo');assert call('document.inspect')['authored']==edited['authored']
 # Undo the edit and Cut independently, then restore both.
 call('history.undo');call('history.undo');assert call('document.inspect')['authored']==attached['authored'];call('history.redo');call('history.redo');assert call('document.inspect')['authored']==edited['authored']
 call('solid.draft',{'featureId':draftid,'angleDeg':80},error='evaluation_failed');assert call('document.inspect')['authored']==edited['authored']
 bad=call('command.begin',{'command':'solid.draft','inputs':{'featureId':draftid}});call('command.update',{'session':bad['id'],'inputs':{'angleDeg':80}});call('command.preview',{'session':bad['id']},error='draft_topology_change');call('command.commit',{'session':bad['id']},error='draft_topology_change');assert call('document.inspect')['authored']==edited['authored'];call('command.update',{'session':bad['id'],'inputs':{'angleDeg':3}});call('command.preview',{'session':bad['id']});call('command.cancel',{'session':bad['id']})
 saved=out/'drafted-wall-cut.alloy';call('document.save',{'path':str(saved)});call('document.new',{'discard':True});call('document.open',{'path':str(saved)});reopened=call('document.inspect');assert reopened['authored']==edited['authored'] and reopened['evaluation']['bodies']==edited['evaluation']['bodies']
 stl=out/'drafted-wall-cut.stl';call('document.exportStl',{'path':str(stl),'bodyId':source,'units':'mm','format':'binary','surfaceDeviationMm':.02,'normalDeviationDeg':5});mesh=inspect_stl(stl,True);assert len(mesh['components'])==1;near(mesh['components'][0]['signedVolume'],edited['evaluation']['bodies'][0]['volumeMm3'],.08)
 for lo,hi,wantlo,wanthi in zip(mesh['bounds'][0],mesh['bounds'][1],[0,0,0],[40,20,20]):near(lo,wantlo,.02);near(hi,wanthi,.02)
 # Rectangular Extrude is the second admitted source family.
 call('document.new',{'discard':True});call('sketch.begin',{'plane':'XY'});call('sketch.rectangle',{'x1':0,'y1':0,'x2':40,'y2':20,'units':'mm'});call('sketch.finish');sk=call('document.inspect')['evaluation']['sketches'][0];call('solid.extrude',{'sketchId':sk['featureId'],'profileId':sk['profiles'][0]['profileId'],'distance':20,'units':'mm'});call('construction.plane',{'source':{'plane':'XY'},'offset':0,'units':'mm'});state=call('document.inspect');eb=state['evaluation']['bodies'][0];side=next(f for f in eb['supportFaces'] if f['frame']['normal'][1]<-.99);call('solid.draft',{'targetBodyId':eb['bodyId'],'face':ref(side),'neutralPlaneId':state['authored']['features'][-1]['id'],'angleDeg':5,'flipPullDirection':False});checkbody(call('document.inspect')['evaluation']['bodies'][0],5)
 (out/'receipt.json').write_text(json.dumps({'passed':True,'initial':initial,'preview':preview,'created':created,'downstream':downstream,'edited':edited,'reopened':reopened,'stl':mesh,'calls':len(log),'transcript':log},indent=2)+'\n')
print('PASS one-face Draft core/API: signed preview/flip/cancel, exact frames, downstream Cut, edit/history/refusal/reopen/STL, rectangular Extrude source')
