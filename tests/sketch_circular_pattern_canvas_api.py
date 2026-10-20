#!/usr/bin/env python3
"""Reopen the actual Qt-picked model through Alloy's real executable and inspect STL."""
import argparse,json,math,struct
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--native-evidence',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve();native=Path(a.native_evidence).resolve();receipt=json.loads((native/'native-report.json').read_text())
with application(a.app,out) as (call,log):
    gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated']
    call('document.open',{'path':str(native/'native-picked-circle.alloy')});initial=call('document.inspect');assert initial['authored']==receipt['authored']
    sketch=next(f for f in initial['authored']['features'] if f['id']==receipt['sketchId']);group=sketch['parameters']['patternGroups'][0];assert (group['type'],group['quantity'],group['mode'],group['totalAngleDeg'])==('circular',6,'full',360)
    native_mesh=inspect_stl(native/'native-picked-circle.stl',True);assert len(native_mesh['components'])==1 and native_mesh['eulerCharacteristic']==-10
    profiles={p['profileId'] for p in initial['evaluation']['sketches'][0]['profiles']};assert len(profiles)==6
    call('sketch.edit',{'sketchId':receipt['sketchId']});call('sketch.dimensionUpdate',{'dimensionId':receipt['radiusDimensionId'],'value':2,'units':'mm'});call('sketch.movePoint',{'pointId':receipt['centerPointId'],'x':26,'y':24,'units':'mm'});call('sketch.movePoint',{'pointId':receipt['sourceCenterPointId'],'x':38,'y':26,'units':'mm'});call('sketch.finish')
    final=call('document.inspect');assert {p['profileId'] for p in final['evaluation']['sketches'][0]['profiles']}==profiles
    volume=final['evaluation']['bodies'][0]['volumeMm3'];assert math.isclose(volume,12500-6*math.pi*4*5,abs_tol=1e-5)
    call('history.undo');assert call('document.inspect')['authored']==initial['authored']
    call('history.redo');assert call('document.inspect')['authored']==final['authored']
    saved=out/'picked-circle-edited.alloy';call('document.save',{'path':str(saved)});call('document.open',{'path':str(saved)});assert call('document.inspect')['authored']==final['authored']
    edit=call('sketch.edit',{'sketchId':receipt['sketchId']});baseline=call('command.preview',{'session':edit['id']})['authoredDraft'];history=call('app.state')['activeSession']['draftHistory']
    call('sketch.patternPreview',dict(type='circular',groupId=group['id'],sourceEntityId=receipt['sourceEntityId'],centerPointId=receipt['centerPointId'],mode='full',quantity=4,totalAngleDeg=360),error='sketch_pattern_dependency')
    assert call('command.preview',{'session':edit['id']})['authoredDraft']==baseline and call('app.state')['activeSession']['draftHistory']==history;call('sketch.cancel')
    call('view.home');call('capture.screenshot',{'path':str(out/'picked-circle-edited-solid.png')})
    stl=out/'picked-circle-edited.stl';call('document.exportStl',{'path':str(stl),'bodyId':final['evaluation']['bodies'][0]['bodyId'],'units':'mm','format':'binary','surfaceDeviationMm':.01,'normalDeviationDeg':5});mesh=inspect_stl(stl,True);assert len(mesh['components'])==1 and mesh['eulerCharacteristic']==-10 and math.isclose(mesh['signedVolume'],volume,rel_tol=1e-4)
    centers=[(26+12*math.cos(k*math.pi/3)-2*math.sin(k*math.pi/3),24+12*math.sin(k*math.pi/3)+2*math.cos(k*math.pi/3)) for k in range(6)];samples=[[] for _ in centers];data=stl.read_bytes()
    for i in range(struct.unpack_from('<I',data,80)[0]):
        values=struct.unpack_from('<12fH',data,84+50*i);vertices=[values[3:6],values[6:9],values[9:12]]
        if max(v[2] for v in vertices)-min(v[2] for v in vertices)<1:continue
        if all(any(abs(v[axis]-edge)<1e-5 for axis,edge in ((0,0),(0,50),(1,0),(1,50))) for v in vertices):continue
        matched=[k for k,(u,v) in enumerate(centers) if all(abs(math.hypot(p[0]-u,p[1]-v)-2)<1e-5 for p in vertices)];assert len(matched)==1;samples[matched[0]].extend(vertices)
    assert all(len(v)>100 and min(p[2] for p in v)==0 and max(p[2] for p in v)==5 for v in samples)
    (out/'report.json').write_text(json.dumps({'passed':True,'nativeAuthored':receipt['authored'],'finalAuthored':final['authored'],'gpu':gpu,'volumeMm3':volume,'nativeMesh':native_mesh,'finalMesh':mesh,'holeCentersMm':centers,'holeSamples':[len(v) for v in samples]},indent=2));(out/'transcript.json').write_text(json.dumps(log,indent=2))
