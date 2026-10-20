#!/usr/bin/env python3
"""Exact skew direction edits and independent STL from the native-picked model."""
import argparse,json,math,struct,uuid
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--native-evidence',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve();native=Path(a.native_evidence).resolve();receipt=json.loads((native/'native-report.json').read_text());assert receipt['skew']
def verify(sketch):
    p=sketch['parameters'];g=p['patternGroups'][0];es={r['id']:r for r in p['entities']};ps={r['id']:r for r in p['points']};source=es[g['sourceEntityId']];center=ps[source['centerPointId']];axes=[]
    for axis in (1,2):
        line=es[g[f'direction{axis}EntityId']];start,end=(ps[line[k]] for k in ('startPointId','endPointId'));du,dv=end['uMm']-start['uMm'],end['vMm']-start['vMm'];length=math.hypot(du,dv);axes.append((du/length*g[f'spacing{axis}Mm'],dv/length*g[f'spacing{axis}Mm']))
    centers=[(center['uMm'],center['vMm'])]
    for r in g['instances']:
        for role,key in (('circle','entityId'),('point','pointId')):assert r[key]==str(uuid.uuid5(uuid.UUID(g['id']),f"alloy-sketch-rectangular/1/{g['sourceEntityId']}/{r['i']}/{r['j']}/{role}"))
        point=ps[r['pointId']];xy=(center['uMm']+r['i']*axes[0][0]+r['j']*axes[1][0],center['vMm']+r['i']*axes[0][1]+r['j']*axes[1][1]);assert all(math.isclose(point[k],v,abs_tol=1e-6) for k,v in zip(('uMm','vMm'),xy));assert es[r['entityId']]['radiusMm']==source['radiusMm'];centers.append(xy)
    return centers
with application(a.app,out) as (call,log):
    gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated']
    call('document.open',{'path':str(native/'native-picked-grid.alloy')});initial=call('document.inspect');assert initial['authored']==receipt['authored']
    sketch=next(f for f in initial['authored']['features'] if f['id']==receipt['sketchId']);group=sketch['parameters']['patternGroups'][0];assert (group['quantity1'],group['quantity2'],group['spacing1Mm'],group['spacing2Mm'])==(2,3,16,12);native_centers=verify(sketch)
    native_mesh=inspect_stl(native/'native-picked-grid.stl',True);assert len(native_mesh['components'])==1 and native_mesh['eulerCharacteristic']==-10
    profiles={p['profileId'] for p in initial['evaluation']['sketches'][0]['profiles']};assert len(profiles)==6
    session=call('sketch.edit',{'sketchId':receipt['sketchId']})
    def draft():return call('command.preview',{'session':session['id']})['authoredDraft']
    call('sketch.dimensionUpdate',{'dimensionId':receipt['radiusDimensionId'],'value':2,'units':'mm'});call('sketch.movePoint',{'pointId':receipt['sourceCenterPointId'],'x':9,'y':8,'units':'mm'});verify(draft())
    es={e['id']:e for e in sketch['parameters']['entities']};axis2=es[group['direction2EntityId']]['endPointId'];axis1=es[group['direction1EntityId']]['endPointId']
    for point,x,y in ((axis2,32,24),(axis1,36,12)):
        call('sketch.movePoint',{'pointId':point,'x':x,'y':y,'units':'mm'});assert draft()['parameters']['patternGroups'][0]==group;verify(draft())
    centers=verify(draft());baseline=draft();history=call('app.state')['activeSession']['draftHistory']
    # Parallel/antiparallel and degenerate source edits must not publish any intent.
    for x,y in ((30,10),(-30,-10),(0,0)):
        call('sketch.movePoint',{'pointId':axis2,'x':x,'y':y,'units':'mm'},error='sketch_solve_failed');assert draft()==baseline and call('app.state')['activeSession']['draftHistory']==history
    args={k:group[k] for k in ('sourceEntityId','direction1EntityId','direction2EntityId','quantity1','quantity2','spacing1Mm','spacing2Mm')};args['groupId']=group['id']
    call('sketch.patternPreview',dict(args,quantity2=2),error='sketch_pattern_dependency');assert draft()==baseline and call('app.state')['activeSession']['draftHistory']==history
    call('sketch.finish');final=call('document.inspect');assert {p['profileId'] for p in final['evaluation']['sketches'][0]['profiles']}==profiles
    volume=final['evaluation']['bodies'][0]['volumeMm3'];assert math.isclose(volume,10400-6*math.pi*4*5,abs_tol=1e-5)
    call('history.undo');assert call('document.inspect')['authored']==initial['authored'];call('history.redo');assert call('document.inspect')['authored']==final['authored']
    saved=out/'skew-grid-edited.alloy';call('document.save',{'path':str(saved)});call('document.open',{'path':str(saved)});assert call('document.inspect')['authored']==final['authored']
    call('view.home');call('capture.screenshot',{'path':str(out/'skew-grid-edited-solid.png')})
    stl=out/'skew-grid-edited.stl';call('document.exportStl',{'path':str(stl),'bodyId':final['evaluation']['bodies'][0]['bodyId'],'units':'mm','format':'binary','surfaceDeviationMm':.01,'normalDeviationDeg':5});mesh=inspect_stl(stl,True);assert len(mesh['components'])==1 and mesh['eulerCharacteristic']==-10 and math.isclose(mesh['signedVolume'],volume,rel_tol=1e-4)
    samples=[[] for _ in centers];data=stl.read_bytes()
    for i in range(struct.unpack_from('<I',data,80)[0]):
        values=struct.unpack_from('<12fH',data,84+50*i);vertices=[values[3:6],values[6:9],values[9:12]]
        if max(v[2] for v in vertices)-min(v[2] for v in vertices)<1:continue
        if all(any(abs(v[axis]-edge)<1e-5 for axis,edge in ((0,0),(0,52),(1,0),(1,40))) for v in vertices):continue
        matched=[k for k,(u,v) in enumerate(centers) if all(abs(math.hypot(p[0]-u,p[1]-v)-2)<1e-5 for p in vertices)];assert len(matched)==1;samples[matched[0]].extend(vertices)
    assert all(len(v)>100 and min(p[2] for p in v)==0 and max(p[2] for p in v)==5 for v in samples)
    (out/'report.json').write_text(json.dumps({'passed':True,'nativeAuthored':receipt['authored'],'finalAuthored':final['authored'],'gpu':gpu,'volumeMm3':volume,'nativeMesh':native_mesh,'finalMesh':mesh,'nativeHoleCentersMm':native_centers,'holeCentersMm':centers,'holeSamples':[len(v) for v in samples]},indent=2));(out/'transcript.json').write_text(json.dumps(log,indent=2))
