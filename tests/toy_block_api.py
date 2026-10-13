#!/usr/bin/env python3
"""Complete toy-block tutorial using retained feature/roof/offset references."""
import argparse
import json
import math
import time
from contextlib import ExitStack
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--app',required=True);p.add_argument('--output',required=True)
p.add_argument('--record-video',type=Path)
a=p.parse_args();out=Path(a.output).resolve()
receipt={'result':'incomplete','source':'external knowledge/tutorials/toy-block/toy-block-ledger.json','stages':[]}
try:
    with application(a.app,out) as (raw,transcript), ExitStack() as cleanup:
        receipt['transcript']=transcript;recording=False
        def stop():
            global recording
            if recording:
                recording=False
                receipt['recording']=raw('recording.stop')
        cleanup.callback(stop)
        if a.record_video:
            movie=a.record_video.resolve();assert not movie.is_relative_to(Path(__file__).resolve().parents[1])
            movie.parent.mkdir(parents=True,exist_ok=True)
            raw('window.resize',{'width':1400,'height':900});raw('recording.start',{'path':str(movie),'fps':10});recording=True
        def step(text):
            if recording:raw('recording.step',{'text':text})
        def call(command,parameters=None):
            result=raw(command,parameters)
            if recording and command.startswith(('sketch.','solid.','command.commit')):time.sleep(.65)
            return result
        def inspect():return call('document.inspect')
        def body():return inspect()['evaluation']['bodies'][0]
        def draft():return call('app.state')['activeSession']['draftSketch']
        def project(u,v):
            q=call('view.projectSketchPoint',{'sketchId':draft()['id'],'uMm':u,'vMm':v});return {'x':q['x'],'y':q['y']}
        def finish():
            call('sketch.finish');return inspect()['evaluation']['sketches'][-1]
        def support(face):return {k:face[k] for k in ('producerFeatureId','bodyId','faceId')}
        def stage(name):
            current=body();assert current['valid'] and current['closed']
            call('view.home');call('capture.screenshot',{'path':str(out/(name+'.png'))})
            receipt['stages'].append({'name':name,'body':current});print('PASS checkpoint',name,flush=True)
        def axis(coordinate,quantity,extent):
            for e in body()['solidEdges']:
                if not e['available']:continue
                delta=[y-x for x,y in zip(e['startMm'],e['endMm'])]
                if abs(delta[coordinate])>1 and math.isclose(abs(delta[coordinate]),math.sqrt(sum(x*x for x in delta)),abs_tol=1e-6):
                    return {'edgeId':e['edgeId'],'quantity':quantity,'extentMm':math.copysign(extent,delta[coordinate])}
            raise AssertionError('Missing retained straight axis')
        assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
        step('Create a 31.8 by 15.8 mm base, extrude 9.6 mm')
        call('sketch.begin',{'plane':'XY','name':'Block footprint'})
        call('sketch.rectangle',{'x1':0,'y1':0,'x2':31.8,'y2':15.8,'units':'mm'})
        call('sketch.constraint',{'type':'fixed','pointIds':[draft()['parameters']['entities'][0]['startPointId']]})
        for e,kind,value in zip(draft()['parameters']['entities'][:2],['horizontal','vertical'],[31.8,15.8]):
            call('sketch.dimension',{'kind':kind,'entityId':e['id'],'value':value,'units':'mm'})
        base=finish();call('solid.extrude',{'sketchId':base['sketchId'],'profileId':base['profiles'][0]['profileId'],'distance':9.6,'units':'mm'})
        target=body()['bodyId'];cap=next(f for f in body()['supportFaces'] if f['role']=='extrude.endCap')
        stage('01-base')
        call('document.save',{'path':str(out/'toy-block.alloy')})
        step('Sketch a 5 mm stud, 3.9 mm from each adjacent edge; Join 1.7 mm')
        call('sketch.begin',{'support':support(cap),'name':'Stud'})
        call('sketch.addCircle',{'centerX':3.9,'centerY':3.9,'diameter':5,'units':'mm'})
        circle=draft()['parameters']['entities'][-1]
        projected=call('app.state')['activeSession']['projectionContext']['lines']
        corners=[(line['startPointId'],line['u1Mm'],line['v1Mm']) for line in projected]+[(line['endPointId'],line['u2Mm'],line['v2Mm']) for line in projected]
        origin=next(point for point,u,v in corners if abs(u)<1e-6 and abs(v)<1e-6)
        for kind in ('horizontal','vertical'):
            call('sketch.dimension',{'kind':kind,'pointIds':[origin,circle['centerPointId']],'value':3.9,'units':'mm'})
        call('sketch.dimension',{'kind':'diameter','entityId':circle['id'],'value':5,'units':'mm'})
        stud=finish();call('solid.extrude',{'sketchId':stud['sketchId'],'profileId':stud['profiles'][0]['profileId'],'distance':1.7,'operation':'join','targetBodyIds':[target]})
        stud_seed=inspect()['authored']['features'][-1]
        step('Pattern the stud feature: 4 over 24 mm, 2 over 8 mm')
        call('solid.rectangularPattern',{'sourceFeatureId':stud_seed['id'],'targetBodyId':target,'axes':[axis(0,4,24),axis(1,2,8)]})
        stage('02-eight-studs')
        bottom=next(f for f in body()['supportFaces'] if f['role']=='extrude.base' and f['frame']['normal'][2]<-.999)
        step('Shell inward 1.49 mm from the bottom face')
        call('solid.shell',{'targetBodyId':target,'removedFace':support(bottom),'thicknessMm':1.49})
        stage('03-hollow-block')
        roofs=[f for f in body()['supportFaces'] if f['frame']['normal'][2]<-.999 and abs(f['frame']['origin'][2]-8.11)<1e-5]
        assert roofs,'Missing actual inner roof support'
        roof=roofs[0];call('sketch.begin',{'support':support(roof),'name':'Retained tube centers'})
        context=call('app.state')['activeSession']['projectionContext']
        centers=context['points'];assert len(centers)==8
        assert roof['role']=='shell-inner'
        frame=roof['frame']
        def uv(world):
            delta=[x-y for x,y in zip(world,frame['origin'])]
            return [sum(x*y for x,y in zip(delta,frame[k])) for k in ('uAxis','vAxis')]
        step('Snap a construction diagonal between the two recessed stud centers')
        call('sketch.tool',{'tool':'line'})
        expected_centers=[]
        for world in ([3.9,3.9,8.11],[11.9,11.9,8.11]):
            u,v=uv(world);reference=next(q for q in centers if math.isclose(q['uMm'],u,abs_tol=1e-6) and math.isclose(q['vMm'],v,abs_tol=1e-6))
            expected_centers.append(reference['id'])
            point=project(u,v);snap=call('sketch.pointerMove',point)['snap']
            assert snap['status']=='acquired' and snap['target']=={'kind':'point','pointId':reference['id']}
            call('sketch.placePoint',point)
        assert len(set(expected_centers))==2
        assert set(q['id'] for q in draft()['parameters']['pointReferences'])==set(expected_centers)
        guide=draft()['parameters']['entities'][-1]
        call('sketch.construction',{'entityIds':[guide['id']],'construction':True})
        step('Draw diameter 6.3 mm at the retained diagonal midpoint; offset inward 1 mm')
        call('sketch.tool',{'tool':'circle'});point=project(*uv([7.9,7.9,8.11]))
        snap=call('sketch.pointerMove',point)['snap'];assert snap['target']=={'kind':'midpoint','entityId':guide['id']}
        call('sketch.placePoint',point);call('sketch.circleDiameterPreview',{'value':6.3,'units':'mm'});call('sketch.circleDiameterAccept')
        outer=draft()['parameters']['entities'][-1]['id']
        midpoint=next(q for q in draft()['parameters']['constraints'] if q['type']=='midpoint')
        assert midpoint['entityIds']==[guide['id']]
        points={q['id']:(q['uMm'],q['vMm']) for q in draft()['parameters']['points']}
        # Endpoints may be directly retained support points rather than local copies.
        points.update({q['id']:(q['uMm'],q['vMm']) for q in centers})
        assert math.isclose(math.dist(points[guide['startPointId']],points[guide['endPointId']]),math.sqrt(128),abs_tol=1e-6)
        call('sketch.circleOffset',{'sourceEntityId':outer,'distance':-1,'units':'mm'})
        offset=next(q for q in draft()['parameters']['constraints'] if q['type']=='circleOffset')
        assert offset['entityIds'][0]==outer and offset['distanceMm']==-1
        tube=finish();annulus=next(r for r in tube['profiles'] if math.isclose(r['areaMm2'],math.pi*(3.15**2-2.15**2),abs_tol=1e-5))
        step('Join the annular profile 8.1 mm, then pattern 3 over 16 mm')
        call('solid.extrude',{'sketchId':tube['sketchId'],'profileId':annulus['profileId'],'distance':8.1,'operation':'join','targetBodyIds':[target]})
        tube_seed=inspect()['authored']['features'][-1]
        call('solid.rectangularPattern',{'sourceFeatureId':tube_seed['id'],'targetBodyId':target,'axes':[axis(0,3,16)]})
        stage('04-three-hollow-tubes')
        selected=[];counts=[0,0,0]
        for e in body()['solidEdges']:
            if not e['available']:continue
            r=e['provenance'];which=None
            if r.get('sourceSketchId')==stud['sketchId'] and r.get('role')=='endRim':which=0
            elif r.get('sourceSketchId')==base['sketchId'] and r.get('role') in ('endRim','sideCorner'):which=1
            elif r.get('sourceSketchId')==tube['sketchId'] and r.get('sourceEntityId')==outer and r.get('role')=='endRim':which=2
            if which is not None:selected.append(e['edgeId']);counts[which]+=1
        assert counts==[8,8,3],counts
        step('Fillet the 19 specified outer edges by 0.2 mm')
        call('solid.fillet',{'targetBodyId':target,'edgeIds':selected,'radiusMm':.2,'tangentChain':True})
        call('selection.set',{'bodyId':''})
        stage('05-complete-toy-block')
        step('Inspect the three hollow tubes and underside rounding')
        call('view.orientation',{'name':'bottom'})
        call('capture.screenshot',{'path':str(out/'06-finished-underside.png')})
        if recording:time.sleep(2)
        call('view.home')
        state=inspect();assert len(state['evaluation']['bodies'])==1
        path=out/'toy-block.alloy';call('document.save',{'path':str(path)})
        step('Save the editable model and export a millimeter STL')
        mesh_path=out/'toy-block-mm.stl'
        receipt['export']=call('document.exportStl',{'path':str(mesh_path),'bodyId':target,'units':'mm','format':'binary','surfaceDeviationMm':.03,'normalDeviationDeg':10})
        receipt['mesh']=inspect_stl(mesh_path,True);assert len(receipt['mesh']['components'])==1
        assert math.isclose(receipt['mesh']['signedVolume'],body()['volumeMm3'],rel_tol=.01)
        stop()
        # One upstream edit traverses the entire completed feature chain.
        call('feature.edit',{'featureId':stud_seed['id'],'parameters':{'distanceMm':1.9}})
        assert body()['valid'] and body()['closed']
        call('history.undo');assert inspect()['authored']==state['authored']
        call('history.redo');call('history.undo');assert inspect()['authored']==state['authored']
        call('document.new',{'discard':True});call('document.open',{'path':str(path),'discard':True})
        assert inspect()['authored']==state['authored']
        receipt.update(result='api-pass',model=str(path))
finally:
    out.mkdir(parents=True,exist_ok=True);(out/'toy-block-api.json').write_text(json.dumps(receipt,indent=2))
