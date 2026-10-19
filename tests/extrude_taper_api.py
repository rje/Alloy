#!/usr/bin/env python3
"""Bounded retained polygon taper through the actual application and independent STL readback."""
import argparse,json,math,struct
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
def near(x,y,tol=1e-5):assert math.isclose(x,y,abs_tol=tol),(x,y)
def rect_volume(angle):
 t=math.tan(math.radians(angle));return 10*(800+600*t+400/3*t*t)
def geometry(b,volume,h):
 near(b['volumeMm3'],volume);near(b['boundsMm']['min'][2],0);near(b['boundsMm']['max'][2],h)
def mesh(call,name,b):
 path=out/(name+'.stl');call('document.exportStl',{'path':str(path),'bodyId':b['bodyId'],'units':'mm','format':'binary','surfaceDeviationMm':0.05,'normalDeviationDeg':5})
 data=path.read_bytes();count=struct.unpack_from('<I',data,80)[0];assert len(data)==84+count*50;points=[];volume=0
 for n in range(count):
  q=struct.unpack_from('<12f',data,84+50*n)[3:];x,y,z=q[:3];u,v,w=q[3:6];r,s,t=q[6:];volume+=(x*(v*t-w*s)+y*(w*r-u*t)+z*(u*s-v*r))/6;points.extend([q[:3],q[3:6],q[6:]])
 for axis in range(3):near(min(v[axis] for v in points),b['boundsMm']['min'][axis]);near(max(v[axis] for v in points),b['boundsMm']['max'][axis])
 near(abs(volume),b['volumeMm3'],0.01)
 return {'triangles':count,'independentVolumeMm3':abs(volume)}
meshes={}
with application(a.app,out) as (call,transcript):
 gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated']
 call('sketch.begin',{'plane':'XY'});call('sketch.rectangle',{'x1':-20,'y1':-10,'x2':20,'y2':10,'units':'mm'});call('sketch.finish')
 source=call('document.inspect');sketch=source['evaluation']['sketches'][0];identity={'sketchId':sketch['featureId'],'profileId':sketch['profiles'][0]['profileId'],'units':'mm','operation':'newBody','direction':'oneSide','start':'profilePlane','extent':'distance','distance':10}
 for angle in [0,-5,5]:
  session=call('solid.extrudeDialog',{**identity,'taperAngleDeg':angle});preview=call('command.preview',{'session':session['id']});geometry(preview['geometry'],rect_volume(angle),10)
  call('view.home');call('capture.screenshot',{'path':str(out/f'rectangle-{angle}-preview.png')});call('capture.dialog',{'path':str(out/f'rectangle-{angle}-dialog.png')});call('command.cancel',{'session':session['id']});assert call('document.inspect')==source
 call('solid.extrude',{**identity,'taperAngleDeg':-5});created=call('document.inspect');feature=created['authored']['features'][-1];fid=feature['id'];geometry(created['evaluation']['bodies'][0],rect_volume(-5),10)
 meshes['rectangle-inward']=mesh(call,'rectangle-inward',created['evaluation']['bodies'][0])
 cap=next(f for f in created['evaluation']['bodies'][0]['supportFaces'] if f['role']=='extrude.endCap');support={k:cap[k] for k in ('producerFeatureId','bodyId','faceId')}
 call('sketch.begin',{'support':support,'name':'Far cap circle'});call('sketch.addCircle',{'centerX':0,'centerY':0,'diameter':4,'units':'mm'});call('sketch.finish');attached=call('document.inspect');child=attached['evaluation']['sketches'][-1]
 call('solid.extrude',{'sketchId':child['featureId'],'profileId':child['profiles'][0]['profileId'],'distance':2,'units':'mm','operation':'newBody'})
 downstream=call('document.inspect');assert len(downstream['evaluation']['bodies'])==2
 edit=call('feature.editDialog',{'featureId':fid});assert edit['inputs']['taperAngleDeg']==-5
 call('command.update',{'session':edit['id'],'inputs':{'taperAngleDeg':-3}});call('command.preview',{'session':edit['id']});call('command.commit',{'session':edit['id']})
 edited=call('document.inspect');geometry(edited['evaluation']['bodies'][0],rect_volume(-3),10);assert edited['authored']['features'][2:]==downstream['authored']['features'][2:];assert edited['evaluation']['bodies'][0]['supportFaces'][0]['faceId']==cap['faceId'];near(edited['evaluation']['sketches'][-1]['frame']['origin'][2],10);near(edited['evaluation']['bodies'][1]['volumeMm3'],8*math.pi)
 call('history.undo');assert call('document.inspect')['authored']==downstream['authored'];call('history.redo');assert call('document.inspect')['authored']==edited['authored']
 for value in [-80,-90,90]:
  call('solid.extrude',{**identity,'featureId':fid,'taperAngleDeg':value},error='evaluation_failed' if value==-80 else 'invalid_arguments');assert call('document.inspect')['authored']==edited['authored']
 call('solid.extrude',{**identity,'featureId':fid,'distance':10});assert call('document.inspect')['authored']==edited['authored']
 path=out/'taper-downstream.alloy';call('document.save',{'path':str(path)});call('document.open',{'path':str(path),'discard':True});assert call('document.inspect')['authored']==edited['authored']
 for angle in [5,0]:
  call('solid.extrude',{**identity,'featureId':fid,'taperAngleDeg':angle});state=call('document.inspect');geometry(state['evaluation']['bodies'][0],rect_volume(angle),10);meshes[f'rectangle-{angle}']=mesh(call,f'rectangle-{angle}',state['evaluation']['bodies'][0])
 call('document.new',{'discard':True});call('sketch.begin',{'plane':'XY'});points=[(0,0),(40,0),(40,10),(10,10),(10,30),(0,30)]
 for i,(x,y) in enumerate(points):
  xx,yy=points[(i+1)%len(points)];call('sketch.line',{'x1':x,'y1':y,'x2':xx,'y2':yy,'units':'mm'})
 call('sketch.finish');state=call('document.inspect');sketch=state['evaluation']['sketches'][0]
 args={'sketchId':sketch['featureId'],'profileId':sketch['profiles'][0]['profileId'],'distance':8,'taperAngleDeg':-5,'operation':'newBody','direction':'oneSide','start':'profilePlane','extent':'distance','units':'mm'}
 session=call('solid.extrudeDialog',args);preview=call('command.preview',{'session':session['id']});t=math.tan(math.radians(5));geometry(preview['geometry'],4800-70*t*64+4/3*t*t*512,8);call('view.home');call('capture.screenshot',{'path':str(out/'concave-preview.png')});call('command.commit',{'session':session['id']});state=call('document.inspect');meshes['concave-inward']=mesh(call,'concave-inward',state['evaluation']['bodies'][0]);concave_id=state['authored']['features'][-1]['id']
 edit=call('feature.editDialog',{'featureId':concave_id});call('command.update',{'session':edit['id'],'inputs':{'taperAngleDeg':-3}});preview=call('command.preview',{'session':edit['id']});tt=math.tan(math.radians(3));geometry(preview['geometry'],4800-70*tt*64+4/3*tt*tt*512,8);call('command.commit',{'session':edit['id']});call('history.undo');assert call('document.inspect')['authored']==state['authored'];call('history.redo');state=call('document.inspect');meshes['concave']=mesh(call,'concave',state['evaluation']['bodies'][0]);path=out/'concave.alloy';call('document.save',{'path':str(path)});call('document.open',{'path':str(path),'discard':True});assert call('document.inspect')['authored']==state['authored']
(out/'receipt.json').write_text(json.dumps({'gpu':gpu,'meshes':meshes,'transcript':transcript},indent=2)+'\n')
print('PASS signed convex/concave taper, preview/Cancel, edit/downstream solid, undo/reopen, independent STL bounds/volume')
