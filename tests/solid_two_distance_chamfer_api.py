#!/usr/bin/env python3
"""SOL-088 exact unequal edge chamfer through Alloy's real application."""
import argparse,json,math
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
def ref(f):return {k:f[k] for k in ('bodyId','producerFeatureId','faceId')}
def near(x,y,t=1e-5):assert abs(x-y)<t,(x,y)
with application(a.app,out) as (call,log):
 call('sketch.begin',{'plane':'XY'});call('sketch.rectangle',{'x1':0,'y1':0,'x2':30,'y2':20,'units':'mm'});call('sketch.finish');sk=call('document.inspect')['evaluation']['sketches'][0]
 ex={'sketchId':sk['featureId'],'profileId':sk['profiles'][0]['profileId'],'distance':10,'units':'mm'};call('solid.extrude',ex);source=call('document.inspect');b=source['evaluation']['bodies'][0];body=b['bodyId'];edge=next(e for e in b['solidEdges'] if all(abs(e[k][1])<1e-7 and abs(e[k][2]-10)<1e-7 for k in ['startMm','endMm']));top=next(f for f in b['supportFaces'] if f['frame']['normal'][2]>.99)
 args={'targetBodyId':body,'edgeIds':[edge['edgeId']],'mode':'twoDistance','distanceOneMm':2,'distanceTwoMm':1,'referenceFace':ref(top),'flip':False}
 # Shared edge-change route clears a previously incident face before reassigning.
 front=next(f for f in b['supportFaces'] if f['frame']['normal'][1]<-.99);other=next(e for e in b['solidEdges'] if all(abs(e[k][1]-20)<1e-7 and abs(e[k][2]-10)<1e-7 for k in ['startMm','endMm']))
 switching=call('solid.chamferDialog',{**args,'referenceFace':ref(front)});call('command.update',{'session':switching['id'],'inputs':{'edgeIds':[other['edgeId']]}});switched=call('command.preview',{'session':switching['id']});assert switched['previewed'] and switched['inputs']['referenceFace']!=ref(front);call('command.cancel',{'session':switching['id']});assert call('document.inspect')['authored']==source['authored']
 session=call('solid.chamferDialog',args);sid=session['id'];preview=call('command.preview',{'session':sid});near(preview['geometry']['volumeMm3'],5970)
 call('command.update',{'session':sid,'inputs':{'flip':True}});flipped=call('command.preview',{'session':sid});near(flipped['geometry']['volumeMm3'],5970);assert flipped['geometry']['flip']
 call('command.cancel',{'session':sid});assert call('document.inspect')['authored']==source['authored']
 session=call('solid.chamferDialog',args);call('command.commit',{'session':session['id']});created=call('document.inspect');feature=created['authored']['features'][-1]['id'];call('history.undo');assert call('document.inspect')['authored']==source['authored'];call('history.redo');assert call('document.inspect')['authored']==created['authored']
 # Unchanged back face supports an actual blind circular Cut.
 back=next(f for f in created['evaluation']['bodies'][0]['supportFaces'] if f['frame']['normal'][1]>.99);ss=call('sketch.begin',{'support':ref(back)});frame=ss['resolvedFrame'];center=[15,20,5];delta=[x-y for x,y in zip(center,frame['origin'])];u=sum(x*y for x,y in zip(delta,frame['uAxis']));v=sum(x*y for x,y in zip(delta,frame['vAxis']));call('sketch.addCircle',{'centerX':u,'centerY':v,'diameter':2,'units':'mm'});call('sketch.finish');cs=call('document.inspect')['evaluation']['sketches'][-1];call('solid.extrude',{'sketchId':cs['featureId'],'profileId':cs['profiles'][0]['profileId'],'distance':-1,'units':'mm','operation':'cut','targetBodyIds':[body]});downstream=call('document.inspect');near(downstream['evaluation']['bodies'][0]['volumeMm3'],5970-math.pi)
 edit=call('feature.editDialog',{'featureId':feature});assert edit['inputs']['referenceFace']==ref(top);call('command.update',{'session':edit['id'],'inputs':{'distanceOneMm':3,'distanceTwoMm':2,'flip':True}});ep=call('command.preview',{'session':edit['id']});near(ep['geometry']['volumeMm3'],5910);call('command.commit',{'session':edit['id']});edited=call('document.inspect');near(edited['evaluation']['bodies'][0]['volumeMm3'],5910-math.pi);assert edited['authored']['features'][3:]==downstream['authored']['features'][3:]
 call('history.undo');assert call('document.inspect')['authored']==downstream['authored'];call('history.redo');assert call('document.inspect')['authored']==edited['authored']
 for key,value in [('distanceOneMm',0),('distanceTwoMm',0),('distanceOneMm',-1),('distanceTwoMm',-1),('distanceOneMm',100)]:
  bad=call('feature.editDialog',{'featureId':feature});call('command.update',{'session':bad['id'],'inputs':{key:value}});call('command.preview',{'session':bad['id']},error='invalid_arguments' if value<=0 else 'evaluation_failed');call('command.commit',{'session':bad['id']},error='invalid_arguments' if value<=0 else 'evaluation_failed');stable=call('document.inspect');assert stable['authored']==edited['authored'] and stable['undo']==edited['undo'];call('command.update',{'session':bad['id'],'inputs':{'distanceOneMm':3,'distanceTwoMm':2}});call('command.preview',{'session':bad['id']});call('command.cancel',{'session':bad['id']})
 ex['featureId']=source['authored']['features'][1]['id'];ex['distance']=12;call('solid.extrude',ex);upstream=call('document.inspect');near(upstream['evaluation']['bodies'][0]['volumeMm3'],7110-math.pi)
 saved=out/'two-distance-chamfer.alloy';call('document.save',{'path':str(saved)});call('document.new',{'discard':True});call('document.open',{'path':str(saved)});reopened=call('document.inspect');assert reopened['authored']==upstream['authored'] and reopened['evaluation']['bodies']==upstream['evaluation']['bodies']
 stl=out/'two-distance-chamfer.stl';call('document.exportStl',{'path':str(stl),'bodyId':body,'units':'mm','format':'binary','surfaceDeviationMm':.02,'normalDeviationDeg':5});mesh=inspect_stl(stl,True);assert len(mesh['components'])==1;near(mesh['components'][0]['signedVolume'],7110-math.pi,.03)
 for actual,wanted in zip(mesh['bounds'],[[0,0,0],[30,20,12]]):
  for x,y in zip(actual,wanted):near(x,y,.02)
 (out/'receipt.json').write_text(json.dumps({'passed':True,'source':source,'preview':preview,'flipped':flipped,'created':created,'downstream':downstream,'edited':edited,'upstream':upstream,'reopened':reopened,'stl':mesh,'calls':len(log),'transcript':log},indent=2)+'\n')
print('PASS Two Distance Chamfer API preview/Flip/Cancel/commit/edit/downstream/refusal/history/reopen/STL')
