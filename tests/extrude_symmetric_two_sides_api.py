#!/usr/bin/env python3
"""Real-entrypoint symmetric and independent two-side New Body lifecycle."""
import argparse,json,math,struct
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
def geometry(b,lo,hi):
    assert math.isclose(b['boundsMm']['min'][2],lo,abs_tol=1e-5),b
    assert math.isclose(b['boundsMm']['max'][2],hi,abs_tol=1e-5),b
    assert math.isclose(b['volumeMm3'],1200*(hi-lo),abs_tol=1e-5),b
with application(a.app,out) as (call,transcript):
    gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated']
    call('sketch.begin',{'plane':'XY'});call('sketch.rectangle',{'x1':-20,'y1':-15,'x2':20,'y2':15,'units':'mm'});call('sketch.finish')
    source=call('document.inspect');sketch=source['evaluation']['sketches'][0];sid=sketch['featureId'];pid=sketch['profiles'][0]['profileId']
    identity={'sketchId':sid,'profileId':pid,'units':'mm','operation':'newBody'}
    for mode,measure,distance,negative,lo,hi in [('symmetric','halfLength',10,None,-10,10),('symmetric','wholeLength',20,None,-10,10),('twoSides',None,14,8,-8,14)]:
        args={**identity,'direction':mode,'distance':distance}
        if measure:args['measurement']=measure
        if negative:args['negativeDistance']=negative
        session=call('solid.extrudeDialog',args);preview=call('command.preview',{'session':session['id']});geometry(preview['geometry'],lo,hi)
        call('view.home');call('capture.screenshot',{'path':str(out/(mode+(measure or '')+'-preview.png'))});call('capture.dialog',{'path':str(out/(mode+(measure or '')+'-dialog.png'))})
        call('command.cancel',{'session':session['id']});assert call('document.inspect')==source
        call('solid.extrude',args);created=call('document.inspect');feature=created['authored']['features'][-1];fid=feature['id'];geometry(created['evaluation']['bodies'][0],lo,hi)
        call('history.undo');assert call('document.inspect')['authored']==source['authored'];call('history.redo');assert call('document.inspect')['authored']==created['authored']
        edit=call('feature.editDialog',{'featureId':fid});assert edit['inputs']['direction']==mode
        updates={'distance':24 if measure=='wholeLength' else 12 if measure else 16}
        if negative:updates['negativeDistance']=9
        call('command.update',{'session':edit['id'],'inputs':updates});call('command.preview',{'session':edit['id']});call('command.commit',{'session':edit['id']})
        edited=call('document.inspect');assert edited['authored']['features'][-1]['id']==fid;assert edited['authored']['features'][-1]['parameters']['profileId']==pid
        geometry(edited['evaluation']['bodies'][0],-9 if negative else -12,16 if negative else 12)
        # Direct shared API edits preserve omitted direction/measurement/second reach.
        call('solid.extrude',{**identity,'featureId':fid,'distance':updates['distance']});assert call('document.inspect')['authored']==edited['authored']
        for invalid in [0,-1]:
            call('solid.extrude',{**args,'featureId':fid,'distance':invalid},error='evaluation_failed')
            assert call('document.inspect')['authored']==edited['authored']
        if negative:
            call('solid.extrude',{**args,'featureId':fid,'negativeDistance':0},error='invalid_arguments');assert call('document.inspect')['authored']==edited['authored']
        if negative:
            cap=next(f for f in edited['evaluation']['bodies'][0]['supportFaces'] if f['role']=='extrude.endCap')
            support={k:cap[k] for k in ('producerFeatureId','bodyId','faceId')}
            call('sketch.begin',{'support':support,'name':'Positive cap circle'})
            call('sketch.addCircle',{'centerX':0,'centerY':0,'diameter':4,'units':'mm'});call('sketch.finish')
            attached=call('document.inspect')['authored']['features'][-1]
            call('solid.extrude',{**identity,'featureId':fid,'distance':18,'negativeDistance':10})
            edited=call('document.inspect');geometry(edited['evaluation']['bodies'][0],-10,18)
            assert edited['authored']['features'][-1]==attached
            attached_eval=edited['evaluation']['sketches'][-1]
            assert math.isclose(attached_eval['frame']['origin'][2],18,abs_tol=1e-6),attached_eval
        path=out/(mode+(measure or '')+'.alloy');call('document.save',{'path':str(path)});call('document.open',{'path':str(path),'discard':True});assert call('document.inspect')['authored']==edited['authored']
        mesh=out/(mode+(measure or '')+'.stl');body=call('document.inspect')['evaluation']['bodies'][0]
        call('document.exportStl',{'path':str(mesh),'bodyId':body['bodyId'],'units':'mm','format':'binary','surfaceDeviationMm':0.1,'normalDeviationDeg':10})
        data=mesh.read_bytes();count=struct.unpack_from('<I',data,80)[0];assert len(data)==84+50*count
        points=[];volume=0
        for n in range(count):
            v=struct.unpack_from('<12f',data,84+50*n)[3:];x,y,z=v[:3];u,vv,w=v[3:6];r,s,t=v[6:];volume+=(x*(vv*t-w*s)+y*(w*r-u*t)+z*(u*s-vv*r))/6;points.extend([v[:3],v[3:6],v[6:]])
        assert math.isclose(abs(volume),body['volumeMm3'],abs_tol=1e-3);assert math.isclose(min(v[2] for v in points),body['boundsMm']['min'][2],abs_tol=1e-5);assert math.isclose(max(v[2] for v in points),body['boundsMm']['max'][2],abs_tol=1e-5)
        call('document.new',{'discard':True});call('sketch.begin',{'plane':'XY'});call('sketch.rectangle',{'x1':-20,'y1':-15,'x2':20,'y2':15,'units':'mm'});call('sketch.finish')
        source=call('document.inspect');sketch=source['evaluation']['sketches'][0];sid=sketch['featureId'];pid=sketch['profiles'][0]['profileId'];identity={'sketchId':sid,'profileId':pid,'units':'mm','operation':'newBody'}
(out/'receipt.json').write_text(json.dumps({'gpu':gpu,'transcript':transcript},indent=2)+'\n')
print('PASS: three modes, preview/cancel, exact bounds/volume, shared edit, atomic refusal, history/reopen and independent STL readback')
