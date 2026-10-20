#!/usr/bin/env python3
"""Normal 3D construction reference, shared lifecycle and unchanged printable solid."""
import argparse,json,math,uuid
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--native-evidence',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve();native=Path(a.native_evidence).resolve();receipt=json.loads((native/'native-report.json').read_text())
with application(a.app,out) as (call,log):
    call('document.open',{'path':str(native/'normal-axis-profile.alloy')});initial=call('document.inspect');assert initial['authored']==receipt['authored'];sid=receipt['sketchId'];axis=receipt['axis'];aid=axis['id']
    call('feature.edit',{'featureId':sid,'parameters':{'constructionLines3d':[]}},error='axis3d_owned');assert call('document.inspect')['authored']==initial['authored']
    mesh=inspect_stl(native/'normal-axis-profile.stl',True);assert len(mesh['components'])==1 and mesh['eulerCharacteristic']==2 and math.isclose(mesh['signedVolume'],1952,abs_tol=1e-6) and mesh['bounds']==[[9.,6.,0.],[32.,22.,8.]]
    info=call('sketch.axis3dInspect',{'sketchId':sid})['axes'][0];assert info['id']==aid and info['direction']==[0,0,1] and info['lengthMm']==10 and info['referenceOnly'] and info['solverVariables']==0 and not info['profileBoundary']
    session=call('sketch.edit',{'sketchId':sid})
    def state():return call('app.state')['activeSession']
    def draft():return state()['draftSketch']
    source=draft();history=state()['draftHistory'];args={'axisId':aid,'start':{'u':5,'v':-2,'w':0},'end':{'u':5,'v':-2,'w':15},'units':'mm'}
    call('sketch.axis3d',args,error='axis3d_mode_required');call('sketch.3dMode',{'enabled':True});call('sketch.axis3dStart');assert state()['axis3dInteraction']['active']
    for command,params in [('sketch.finish',{}),('command.commit',{'session':session['id']})]:call(command,params,error='pending_axis3d');assert draft()==source and state()['draftHistory']==history
    call('sketch.axis3dCancel');assert not state().get('axis3dInteraction');call('sketch.axis3dStart');call('sketch.axis3d',args);assert not state().get('axis3dInteraction');edited=draft();changed=edited['parameters']['constructionLines3d'][0]
    for key in ('id','start','end'):
        if key=='id':assert changed[key]==axis[key]
        else:assert changed[key]['id']==axis[key]['id']
    for key in ('points','entities','constraints','dimensions'):assert edited['parameters'].get(key)==source['parameters'].get(key)
    h=state()['draftHistory']
    bad=[dict(args,end={'u':5,'v':-2,'w':v}) for v in (0,-1,100001)]
    bad += [dict(args,end={'u':6,'v':-2,'w':15}),dict(args,start={'u':5,'v':-2,'w':1}),dict(args,start={'u':'5','v':-2,'w':0}),dict(args,axisId=str(uuid.uuid4())),dict(args,start={'u':5,'v':-2,'w':0,'id':str(uuid.uuid4())}),dict(args,start={'u':100001,'v':-2,'w':0},end={'u':100001,'v':-2,'w':15})]
    for params in bad:call('sketch.axis3d',params,error='sketch_axis3d_failed');assert draft()==edited and state()['draftHistory']==h
    call('sketch.axis3d',dict(args,axisId=''),error='sketch_axis3d_failed');assert draft()==edited and state()['draftHistory']==h
    call('history.undo');assert draft()==source;call('history.redo');assert draft()==edited
    call('sketch.axis3dDelete',{'axisId':str(uuid.uuid4())},error='sketch_axis3d_failed');assert draft()==edited
    call('sketch.axis3dDelete',{'axisId':aid});assert not call('sketch.axis3dInspect')['axes'];call('history.undo');assert draft()==edited
    call('sketch.axis3dStart');call('command.cancel',{'session':session['id']});assert not call('app.state')['activeSession'] and call('document.inspect')['authored']==initial['authored']
    call('sketch.edit',{'sketchId':sid});call('sketch.3dMode',{'enabled':True});call('sketch.axis3d',args);call('sketch.finish');final=call('document.inspect');assert final['evaluation']['bodies']==initial['evaluation']['bodies'];assert [r['profileId'] for r in final['evaluation']['sketches'][0]['profiles']]==[r['profileId'] for r in initial['evaluation']['sketches'][0]['profiles']]
    call('history.undo');assert call('document.inspect')['authored']==initial['authored'];call('history.redo');assert call('document.inspect')['authored']==final['authored'];call('document.save',{'path':str(out/'edited-normal-axis.alloy')});call('document.open',{'path':str(out/'edited-normal-axis.alloy')});assert call('document.inspect')['authored']==final['authored']
    call('document.exportStl',{'path':str(out/'edited-normal-axis.stl'),'bodyId':final['evaluation']['bodies'][0]['bodyId'],'units':'mm','format':'binary','surfaceDeviationMm':.01,'normalDeviationDeg':5});edited_mesh=inspect_stl(out/'edited-normal-axis.stl',True);assert edited_mesh==mesh
    call('view.home');call('capture.screenshot',{'path':str(out/'unchanged-asymmetric-solid.png')});gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated'];call('sketch.edit',{'sketchId':sid});call('sketch.3dMode',{'enabled':True});call('sketch.lookAt');call('view.zoom',{'factor':.6});call('capture.screenshot',{'path':str(out/'edited-normal-axis.png')});call('sketch.cancel')
    (out/'report.json').write_text(json.dumps({'passed':True,'nativeMesh':mesh,'editedMesh':edited_mesh,'editedAxis':changed,'gpu':gpu,'final':final},indent=2));(out/'transcript.json').write_text(json.dumps(log,indent=2))
