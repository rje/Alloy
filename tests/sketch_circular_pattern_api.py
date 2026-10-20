#!/usr/bin/env python3
"""Retained circular Full/Angle and printable six-hole Cut through the real app."""
import argparse,json,math,struct
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,log):
    gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated']
    session=call('sketch.begin',{'plane':'XY','name':'Retained circular holes'})
    def draft():return call('command.preview',{'session':session['id']})['authoredDraft']
    call('sketch.addPoint',{'x':25,'y':25,'units':'mm'});center=draft()['parameters']['entities'][0]
    call('sketch.addCircle',{'centerX':37,'centerY':25,'diameter':3,'units':'mm'});seed=draft()['parameters']['entities'][-1]
    call('sketch.dimension',{'kind':'radius','entityId':seed['id'],'value':1.5,'units':'mm'});radius=draft()['parameters']['dimensions'][-1]['id']
    before=draft();dof=call('app.state')['activeSession']['solverDiagnostics']['dof'];history=call('app.state')['activeSession']['draftHistory']
    args={'type':'circular','sourceEntityId':seed['id'],'centerPointId':center['pointId'],'mode':'full','quantity':6,'totalAngleDeg':360}
    preview=call('sketch.patternPreview',args);assert draft()==before and call('app.state')['activeSession']['draftHistory']==history
    assert preview['diagnostics']['dof']==dof
    call('sketch.lookAt');assert call('app.state')['viewport']['gpu']['draftNormalEdgeCount']==6
    call('capture.screenshot',{'path':str(out/'full-preview.png')});call('sketch.patternCancel');assert draft()==before
    preview=call('sketch.patternPreview',args);group=preview['patternGroup'];call('sketch.patternAccept');full=draft()
    assert next(e for e in full['parameters']['entities'] if e['id']==seed['id'])==seed
    ids={r['k']:r['entityId'] for r in group['instances']}
    def verify(s):
        g=s['parameters']['patternGroups'][0];es={e['id']:e for e in s['parameters']['entities']};ps={p['id']:p for p in s['parameters']['points']}
        source=es[seed['id']];start=ps[source['centerPointId']];origin=ps[center['pointId']];x=start['uMm']-origin['uMm'];y=start['vMm']-origin['vMm']
        for k,eid in {0:seed['id'],**{r['k']:r['entityId'] for r in g['instances']}}.items():
            angle=math.radians(g['totalAngleDeg'])*k/(g['quantity']-(g['mode']=='angle'));e=es[eid];p=ps[e['centerPointId']]
            assert math.isclose(p['uMm'],origin['uMm']+x*math.cos(angle)-y*math.sin(angle),abs_tol=1e-6)
            assert math.isclose(p['vMm'],origin['vMm']+x*math.sin(angle)+y*math.cos(angle),abs_tol=1e-6)
            assert math.isclose(e['radiusMm'],source['radiusMm'],abs_tol=1e-6)
    verify(full)
    larger=call('sketch.patternPreview',dict(args,groupId=group['id'],quantity=8))['patternGroup'];assert ids.items()<={r['k']:r['entityId'] for r in larger['instances']}.items();call('sketch.patternAccept');verify(draft());call('history.undo');assert draft()==full
    angle_args=dict(args,groupId=group['id'],mode='angle',quantity=4,totalAngleDeg=120)
    call('sketch.patternPreview',angle_args);call('sketch.patternAccept');angle120=draft();verify(angle120)
    call('sketch.patternPreview',dict(angle_args,totalAngleDeg=150));call('sketch.patternAccept');angle150=draft();verify(angle150)
    call('sketch.lookAt');call('capture.screenshot',{'path':str(out/'angle150.png')})
    call('history.undo');assert draft()==angle120
    call('history.redo');assert draft()==angle150
    for bad in ({'quantity':2.5},{'centerPointId':seed['centerPointId']},{'centerPointId':'missing-point'},{'totalAngleDeg':0},{'totalAngleDeg':360},{'totalAngleDeg':2}):
        call('sketch.patternPreview',dict(angle_args,**bad),error='sketch_pattern_failed');assert draft()==angle150
    call('history.undo');call('history.undo');assert draft()==full
    call('sketch.finish');sketch_id=full['id']
    call('solid.box',{'width':55,'depth':50,'height':5,'units':'mm','name':'Circular hole plate'});body=call('document.inspect')['evaluation']['bodies'][0]
    profiles=call('document.inspect')['evaluation']['sketches'][0]['profiles'];assert len(profiles)==6
    profile_ids={p['profileId'] for p in profiles}
    call('solid.extrude',{'sketchId':sketch_id,'profileIds':list(profile_ids),'distance':5,'units':'mm','operation':'cut','targetBodyIds':[body['bodyId']]})
    initial=call('document.inspect');assert math.isclose(initial['evaluation']['bodies'][0]['volumeMm3'],13750-6*math.pi*1.5**2*5,abs_tol=1e-5)
    session=call('sketch.edit',{'sketchId':sketch_id});call('sketch.dimensionUpdate',{'dimensionId':radius,'value':2,'units':'mm'})
    call('sketch.movePoint',{'pointId':center['pointId'],'x':26,'y':24,'units':'mm'})
    call('sketch.movePoint',{'pointId':seed['centerPointId'],'x':38,'y':26,'units':'mm'});verify(draft());call('sketch.finish')
    final=call('document.inspect');volume=final['evaluation']['bodies'][0]['volumeMm3'];assert math.isclose(volume,13750-6*math.pi*4*5,abs_tol=1e-5)
    assert {p['profileId'] for p in final['evaluation']['sketches'][0]['profiles']}==profile_ids
    call('history.undo');assert call('document.inspect')['authored']==initial['authored']
    call('history.redo');assert call('document.inspect')['authored']==final['authored']
    call('view.home');call('capture.screenshot',{'path':str(out/'circular-hole-plate.png')})
    saved=out/'circular-hole-plate.alloy';call('document.save',{'path':str(saved)});call('document.open',{'path':str(saved)});assert call('document.inspect')['authored']==final['authored']
    session=call('sketch.edit',{'sketchId':sketch_id});baseline=draft();history=call('app.state')['activeSession']['draftHistory']
    call('sketch.patternPreview',dict(args,groupId=group['id'],quantity=4),error='sketch_pattern_dependency')
    assert draft()==baseline and call('app.state')['activeSession']['draftHistory']==history;call('sketch.cancel')
    stl=out/'circular-hole-plate.stl';call('document.exportStl',{'path':str(stl),'bodyId':body['bodyId'],'units':'mm','format':'binary','surfaceDeviationMm':.01,'normalDeviationDeg':5})
    mesh=inspect_stl(stl,True);assert len(mesh['components'])==1 and mesh['eulerCharacteristic']==-10 and math.isclose(mesh['signedVolume'],volume,rel_tol=1e-4)
    centers=[(26+12*math.cos(k*math.pi/3)-2*math.sin(k*math.pi/3),24+12*math.sin(k*math.pi/3)+2*math.cos(k*math.pi/3)) for k in range(6)];samples=[[] for _ in centers]
    data=stl.read_bytes()
    for i in range(struct.unpack_from('<I',data,80)[0]):
        values=struct.unpack_from('<12fH',data,84+50*i);vertices=[values[3:6],values[6:9],values[9:12]]
        if max(v[2] for v in vertices)-min(v[2] for v in vertices)<1:continue
        if all(any(abs(v[axis]-edge)<1e-5 for axis,edge in ((0,0),(0,55),(1,0),(1,50))) for v in vertices):continue
        matched=[k for k,(u,v) in enumerate(centers) if all(abs(math.hypot(p[0]-u,p[1]-v)-2)<1e-5 for p in vertices)]
        assert len(matched)==1,(vertices,matched);samples[matched[0]].extend(vertices)
    assert all(len(v)>100 and min(p[2] for p in v)==0 and max(p[2] for p in v)==5 for v in samples)
    (out/'report.json').write_text(json.dumps({'passed':True,'gpu':gpu,'full':full,'angle120':angle120,'angle150':angle150,'finalAuthored':final['authored'],'volumeMm3':volume,'mesh':mesh,'independentHoleCentersMm':centers,'holeWallSamples':[len(v) for v in samples]},indent=2))
    (out/'transcript.json').write_text(json.dumps(log,indent=2))
