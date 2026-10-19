#!/usr/bin/env python3
"""Bounded one open Line thin solid Extrude, actual app and independent STL."""
import argparse,json,math,struct
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
def near(x,y,tol=1e-5): assert math.isclose(x,y,abs_tol=tol),(x,y)
def geometry(b,thickness=4,side='center',cut=False):
 assert b['valid'] and b['closed'] and b['topology']['solids']==1;near(b['volumeMm3'],40*thickness*20-(math.pi/2 if cut else 0));bounds=b['boundsMm'];low=0 if side=='side1' else -thickness if side=='side2' else -thickness/2
 for x,y in zip(bounds['min'],[0,low,0]):near(x,y)
 for x,y in zip(bounds['max'],[40,low+thickness,20]):near(x,y)
def mesh(call,name,b):
 path=out/(name+'.stl');call('document.exportStl',{'path':str(path),'bodyId':b['bodyId'],'units':'mm','format':'binary','surfaceDeviationMm':0.01,'normalDeviationDeg':5})
 data=path.read_bytes();count=struct.unpack_from('<I',data,80)[0];assert len(data)==84+count*50;points=[];volume=0
 for n in range(count):
  q=struct.unpack_from('<12f',data,84+50*n)[3:];x,y,z=q[:3];u,v,w=q[3:6];r,s,t=q[6:];volume+=(x*(v*t-w*s)+y*(w*r-u*t)+z*(u*s-v*r))/6;points.extend([q[:3],q[3:6],q[6:]])
 for axis in range(3):near(min(v[axis] for v in points),b['boundsMm']['min'][axis]);near(max(v[axis] for v in points),b['boundsMm']['max'][axis])
 near(abs(volume),b['volumeMm3'],0.02);return {'triangles':count,'independentVolumeMm3':abs(volume)}
meshes={}
with application(a.app,out) as (call,transcript):
 gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated']
 call('sketch.begin',{'plane':'XY'});call('sketch.line',{'x1':0,'y1':0,'x2':40,'y2':0,'units':'mm'});call('sketch.finish')
 source=call('document.inspect');sketch=source['evaluation']['sketches'][0];entity=sketch['entities'][0]
 identity={'sketchId':sketch['featureId'],'sourceEntityId':entity['id'],'units':'mm','operation':'newBody','direction':'oneSide','start':'profilePlane','extent':'distance','distance':20,'wallThickness':4}
 for side in ['center','side1','side2']:
  session=call('solid.extrudeDialog',{**identity,'wallLocation':side});preview=call('command.preview',{'session':session['id']});geometry(preview['geometry'],side=side)
  call('view.home');call('capture.screenshot',{'path':str(out/f'{side}-preview.png')});call('capture.dialog',{'path':str(out/f'{side}-dialog.png')});call('command.cancel',{'session':session['id']});assert call('document.inspect')==source
 call('solid.extrude',{**identity,'wallLocation':'center'});created=call('document.inspect');feature=created['authored']['features'][-1];fid=feature['id'];body=created['evaluation']['bodies'][0];geometry(body);meshes['center']=mesh(call,'center',body)
 assert feature['parameters']['sourceEntityId']==entity['id'] and 'profileId' not in feature['parameters'];assert len(body['supportFaces'])==1 and not body['solidEdges']
 call('history.undo');assert call('document.inspect')['authored']==source['authored'];call('history.redo');assert call('document.inspect')['authored']==created['authored']
 cap=body['supportFaces'][0];support={k:cap[k] for k in ('producerFeatureId','bodyId','faceId')}
 call('sketch.begin',{'support':support,'name':'Far cap circle'});call('sketch.addCircle',{'centerX':20,'centerY':0,'diameter':2,'units':'mm'});call('sketch.finish');child=call('document.inspect')['evaluation']['sketches'][-1]
 call('solid.extrude',{'sketchId':child['featureId'],'profileId':child['profiles'][0]['profileId'],'distance':-0.5,'units':'mm','operation':'cut','targetBodyIds':[body['bodyId']]});downstream=call('document.inspect');geometry(downstream['evaluation']['bodies'][0],cut=True)
 edit=call('feature.editDialog',{'featureId':fid});assert edit['inputs']['sourceEntityId']==entity['id'] and edit['inputs']['wallThickness']==4
 call('command.update',{'session':edit['id'],'inputs':{'wallThickness':6}});call('command.preview',{'session':edit['id']});call('command.commit',{'session':edit['id']});edited=call('document.inspect');geometry(edited['evaluation']['bodies'][0],6,cut=True)
 assert edited['authored']['features'][2:]==downstream['authored']['features'][2:];assert edited['evaluation']['bodies'][0]['supportFaces'][0]['faceId']==cap['faceId'];near(edited['evaluation']['sketches'][-1]['frame']['origin'][2],20)
 call('history.undo');assert call('document.inspect')['authored']==downstream['authored'];call('history.redo');assert call('document.inspect')['authored']==edited['authored']
 for extra in [{'wallThickness':0},{'wallThickness':-1},{'wallThickness':1e6},{'wallLocation':'bad'},{'taperAngleDeg':1},{'direction':'symmetric','measurement':'halfLength'}]:
  call('solid.extrude',{**identity,'featureId':fid,'wallLocation':'center',**extra},error='invalid_arguments');assert call('document.inspect')['authored']==edited['authored']
 path=out/'thin-downstream.alloy';call('document.save',{'path':str(path)});call('document.open',{'path':str(path),'discard':True});assert call('document.inspect')['authored']==edited['authored'];geometry(call('document.inspect')['evaluation']['bodies'][0],6,cut=True)
 meshes['edited-cut']=mesh(call,'edited-cut',edited['evaluation']['bodies'][0]);call('solid.extrude',{**identity,'featureId':fid,'wallLocation':'center'});restored=call('document.inspect');geometry(restored['evaluation']['bodies'][0],cut=True);assert restored['authored']['features'][1]['id']==fid and restored['authored']['features'][1]['parameters']['sourceEntityId']==entity['id'];meshes['restored-cut']=mesh(call,'restored-cut',restored['evaluation']['bodies'][0]);call('document.save',{'path':str(out/'thin-restored.alloy')});call('view.home');call('capture.screenshot',{'path':str(out/'committed-cut.png')})
(out/'receipt.json').write_text(json.dumps({'gpu':gpu,'sourceSketch':sketch,'meshes':meshes,'transcript':transcript},indent=2)+'\n')
print('PASS open Line thin: three sides, Cancel, retained edit/cap Cut, atomic refusal, Undo/reopen, independent STL and GPU')
