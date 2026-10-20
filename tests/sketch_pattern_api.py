#!/usr/bin/env python3
"""Retained rectangular hole pattern through the real local application API."""
import argparse,json,math,sys,struct
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,log):
    gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated']
    session=call('sketch.begin',{'plane':'XY','name':'Retained six hole grid'})
    def draft():return call('command.preview',{'session':session['id']})['authoredDraft']
    axes=[]
    for x,y in ((10,0),(0,10)):
        call('sketch.line',{'x1':0,'y1':0,'x2':x,'y2':y,'units':'mm'})
        axes.append(draft()['parameters']['entities'][-1]['id'])
        call('sketch.construction',{'entityIds':[axes[-1]],'construction':True})
    call('sketch.addCircle',{'centerX':8,'centerY':7,'diameter':3,'units':'mm'})
    seed=draft()['parameters']['entities'][-1]
    call('sketch.dimension',{'kind':'radius','entityId':seed['id'],'value':1.5,'units':'mm'})
    radius=draft()['parameters']['dimensions'][-1]['id']
    before=draft();history=call('app.state')['activeSession']['draftHistory']
    args={'sourceEntityId':seed['id'],'direction1EntityId':axes[0],'direction2EntityId':axes[1],'quantity1':2,'quantity2':3,'spacing1Mm':14,'spacing2Mm':11}
    preview=call('sketch.patternPreview',args);assert draft()==before
    assert call('app.state')['activeSession']['draftHistory']==history
    call('sketch.lookAt');assert call('app.state')['viewport']['gpu']['draftNormalEdgeCount']==6;call('capture.screenshot',{'path':str(out/'grid-preview.png')})
    call('sketch.patternCancel');assert draft()==before
    preview=call('sketch.patternPreview',args);group=preview['patternGroup'];assert len(group['instances'])==5
    call('sketch.patternAccept');committed=draft()
    assert committed['parameters']['entities'][2]==seed
    ids={(v['i'],v['j']):v['entityId'] for v in group['instances']}
    def verify(s,r,u,v):
        es={e['id']:e for e in s['parameters']['entities']};ps={p['id']:p for p in s['parameters']['points']}
        for k,eid in {(0,0):seed['id'],**ids}.items():
            e=es[eid];center=ps[e['centerPointId']]
            assert math.isclose(e['radiusMm'],r,abs_tol=1e-6)
            assert math.isclose(center['uMm'],u+14*k[0],abs_tol=1e-6) and math.isclose(center['vMm'],v+11*k[1],abs_tol=1e-6)
    verify(committed,1.5,8,7)
    call('history.undo');assert draft()==before
    call('history.redo');assert draft()==committed
    for bad in ({'quantity1':2.5},{'spacing1Mm':0},{'spacing1Mm':2},{'direction2EntityId':axes[0]}):
        call('sketch.patternPreview',dict(args,groupId=group['id'],**bad),error='sketch_pattern_failed');assert draft()==committed
    edit=dict(args,groupId=group['id'],quantity1=3,quantity2=4,spacing1Mm=15,spacing2Mm=12)
    larger=call('sketch.patternPreview',edit)['patternGroup'];assert ids.items() <= {(v['i'],v['j']):v['entityId'] for v in larger['instances']}.items()
    call('sketch.patternAccept');call('history.undo');assert draft()==committed
    call('sketch.finish');sketch_id=committed['id']
    call('solid.box',{'width':42,'depth':40,'height':5,'units':'mm','name':'Asymmetric hole plate'})
    body=call('document.inspect')['evaluation']['bodies'][0]
    inspection=call('document.inspect');profiles=inspection['evaluation']['sketches'][0]['profiles'];assert len(profiles)==6
    call('solid.extrude',{'sketchId':sketch_id,'profileIds':[p['profileId'] for p in profiles],'distance':5,'units':'mm','operation':'cut','targetBodyIds':[body['bodyId']]})
    initial_cut=call('document.inspect');assert math.isclose(initial_cut['evaluation']['bodies'][0]['volumeMm3'],42*40*5-6*math.pi*1.5**2*5,abs_tol=1e-5)
    session=call('sketch.edit',{'sketchId':sketch_id})
    call('sketch.dimensionUpdate',{'dimensionId':radius,'value':2,'units':'mm'})
    call('sketch.movePoint',{'pointId':seed['centerPointId'],'x':9,'y':8,'units':'mm'})
    verify(draft(),2,9,8)
    call('sketch.finish')
    result=call('document.inspect');volume=result['evaluation']['bodies'][0]['volumeMm3'];assert math.isclose(volume,42*40*5-6*math.pi*4*5,abs_tol=1e-5),volume
    call('view.home');call('capture.screenshot',{'path':str(out/'six-hole-plate.png')})
    saved=out/'six-hole-plate.alloy';call('document.save',{'path':str(saved)});call('document.open',{'path':str(saved)});assert call('document.inspect')['authored']==result['authored']
    # Reducing the explicit Cut profile set's pattern must refuse before any draft/history edit.
    session=call('sketch.edit',{'sketchId':sketch_id});baseline=draft();history=call('app.state')['activeSession']['draftHistory']
    call('sketch.patternPreview',dict(args,groupId=group['id'],quantity2=2),error='sketch_pattern_dependency')
    assert draft()==baseline and call('app.state')['activeSession']['draftHistory']==history
    assert call('document.inspect')['authored']==result['authored']
    call('sketch.cancel')
    stl=out/'six-hole-plate.stl';call('document.exportStl',{'path':str(stl),'bodyId':body['bodyId'],'units':'mm','format':'binary','surfaceDeviationMm':.01,'normalDeviationDeg':5})
    # Existing independent mesh parser checks closed oriented components and volume.
    sys.path.insert(0,str(Path(__file__).resolve().parent));from stl_export_api import inspect_stl
    mesh=inspect_stl(stl,True);assert len(mesh["components"])==1 and mesh["eulerCharacteristic"]==-10
    assert math.isclose(mesh["signedVolume"],volume,rel_tol=1e-4)
    centers=[(9+14*i,8+11*j) for i in range(2) for j in range(3)];hole_vertices=[[] for _ in centers]
    data=stl.read_bytes();triangle_count=struct.unpack_from('<I',data,80)[0]
    for index in range(triangle_count):
        values=struct.unpack_from('<12fH',data,84+50*index);vertices=[values[3:6],values[6:9],values[9:12]]
        if max(v[2] for v in vertices)-min(v[2] for v in vertices)<1:continue
        # Interior vertical wall vertices must lie on one independently predicted circle.
        if all(any(abs(v[axis]-edge)<1e-5 for axis,edge in ((0,0),(0,42),(1,0),(1,40))) for v in vertices):continue
        matched=[]
        for k,(u,v) in enumerate(centers):
            if all(abs(math.hypot(p[0]-u,p[1]-v)-2)<1e-5 for p in vertices):matched.append(k)
        assert len(matched)==1,(vertices,matched)
        hole_vertices[matched[0]].extend(vertices)
    assert all(len(v)>100 and min(p[2] for p in v)==0 and max(p[2] for p in v)==5 for v in hole_vertices)
    (out/'report.json').write_text(json.dumps({'passed':True,'gpu':gpu,'group':group,'volumeMm3':volume,'mesh':mesh,'independentHoleCentersMm':centers,'holeWallVertexSamples':[len(v) for v in hole_vertices],'authored':result['authored']},indent=2))
    (out/'transcript.json').write_text(json.dumps(log,indent=2))
