#!/usr/bin/env python3
"""Complete named-component die workflow; shared API movie, separate native review."""
import argparse
import json
import math
import time
from contextlib import ExitStack
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--app',required=True);parser.add_argument('--output',required=True)
parser.add_argument('--record-video',type=Path)
a=parser.parse_args();out=Path(a.output).resolve()
receipt={'result':'incomplete','source':'external knowledge/tutorials/warwick/library-die-ledger.md','fixture':'20 mm cube, 1 mm rounding, diameter3 pips; learner-chosen dimensions in source','stages':[]}
try:
    with application(a.app,out) as (raw,transcript), ExitStack() as cleanup:
        receipt['transcript']=transcript;recording=False
        def stop():
            global recording
            if recording:
                recording=False;receipt['recording']=raw('recording.stop')
        cleanup.callback(stop)
        if a.record_video:
            movie=a.record_video.resolve();assert not movie.is_relative_to(Path(__file__).resolve().parents[1])
            movie.parent.mkdir(parents=True,exist_ok=True)
            raw('window.resize',{'width':1400,'height':900});raw('recording.start',{'path':str(movie),'fps':10});recording=True
        def call(command,parameters=None):
            result=raw(command,parameters)
            if recording and command.startswith(('sketch.','solid.','command.commit')):time.sleep(.5)
            return result
        def step(text):
            if recording:raw('recording.step',{'text':text})
        def inspect():return call('document.inspect')
        def body():return next(b for b in inspect()['evaluation']['bodies'] if b['ownerId']==definition)
        def stage(name):
            current=body();assert current['valid'] and current['closed']
            call('capture.screenshot',{'path':str(out/(name+'.png'))})
            receipt['stages'].append({'name':name,'body':current});print('PASS',name,flush=True)
        model=out/'game-pieces.alloy'
        step('Save a new design and create the die component')
        call('document.save',{'path':str(model)})
        component=call('component.create',{'name':'die'});definition=component['definitionId'];occurrence=component['occurrenceId']
        step('Center a 20 by 20 mm square and extrude 20 mm')
        call('sketch.begin',{'plane':'XY','name':'Die base'})
        call('sketch.centerRectangle',{'centerX':0,'centerY':0,'width':20,'height':20,'units':'mm'})
        call('sketch.finish');sketch=inspect()['evaluation']['sketches'][-1]
        call('solid.extrude',{'sketchId':sketch['sketchId'],'profileId':sketch['profiles'][0]['profileId'],'distance':20,'units':'mm'})
        call('view.home');stage('01-cube');assert abs(body()['volumeMm3']-8000)<1e-6
        original=inspect()['authored'];target=body()['bodyId'];edges=[e['edgeId'] for e in body()['solidEdges'] if e['available']];assert len(edges)==12
        step('Compare Chamfer and Fillet previews, cancelling each trial')
        for command,field in [('chamfer','distanceMm'),('fillet','radiusMm')]:
            arguments={'targetBodyId':target,'edgeIds':edges,field:1}
            if command=='fillet':arguments['tangentChain']=True
            session=call('solid.'+command+'Dialog',arguments);preview=call('command.preview',{'session':session['id']});assert preview['geometry']['valid']
            call('capture.screenshot',{'path':str(out/(command+'-preview.png'))});call('command.cancel',{'session':session['id']});assert inspect()['authored']==original
        step('Accept 1 mm rounding on all twelve cube edges')
        session=call('solid.filletDialog',{'targetBodyId':target,'edgeIds':edges,'radiusMm':1,'tangentChain':True});call('command.commit',{'session':session['id']})
        stage('02-rounded');rounded=inspect();rounding=rounded['authored']['features'][-1];before=body()['volumeMm3']
        source_faces=body()['supportFaces'];face_ids={f['faceId'] for f in source_faces};assert len(face_ids)==6
        rows=[]
        for face in source_faces:
            frame=face['frame'];axis=max(range(3),key=lambda k:abs(frame['normal'][k]));positive=frame['normal'][axis]>0
            count={(2,True):1,(2,False):6,(0,True):2,(0,False):5,(1,True):3,(1,False):4}[axis,positive]
            view={(2,True):'top',(2,False):'bottom',(0,True):'right',(0,False):'left',(1,True):'back',(1,False):'front'}[axis,positive]
            offsets=[]
            if count in (1,3,5):offsets.append((0,0))
            if count in (2,3):offsets.extend([(-5,-5),(5,5)])
            if count in (4,5):offsets.extend((x,y) for x in (-5,5) for y in (-5,5))
            if count==6:offsets.extend((x,y) for x in (-5,5) for y in (-5,0,5))
            center=[0,0,10];center[axis]=(20 if positive else 0) if axis==2 else (10 if positive else -10)
            uv=[sum((center[k]-frame['origin'][k])*frame[basis][k] for k in range(3)) for basis in ('uAxis','vAxis')]
            rows.append({'count':count,'view':view,'support':{k:face[k] for k in ('bodyId','producerFeatureId','faceId')},'centers':[(uv[0]+x,uv[1]+y) for x,y in offsets]})
        receipt['pipTable']=rows;placed=0
        for row in sorted(rows,key=lambda r:r['count']):
            step(f"Place {row['count']} spherical pips on the {row['view']} face")
            call('view.orientation',{'name':row['view']})
            for u,v in row['centers']:
                if placed:session=call('solid.repeatSphere',{'support':row['support']})
                else:session=call('solid.sphereDialog',{'support':row['support'],'diameterMm':3})
                assert session['inputs']['operation']=='cut' and 'uMm' not in session['inputs'] and 'vMm' not in session['inputs']
                point=call('view.projectFacePoint',{'support':row['support'],'uMm':u,'vMm':v})
                call('selection.pick',{'x':point['x'],'y':point['y']})
                # Pixel placement is genuinely exercised; canonical fixture coordinates
                # then remove screen quantization from the deterministic volume/layout.
                call('command.update',{'session':session['id'],'inputs':{'uMm':u,'vMm':v}})
                preview=call('command.preview',{'session':session['id']});assert preview['geometry']['valid']
                call('command.commit',{'session':session['id']});placed+=1
                current=body();assert {f['faceId'] for f in current['supportFaces']}==face_ids
                assert abs(before-current['volumeMm3']-placed*2.25*math.pi)<1e-5
            stage(f"03-face-{row['count']}")
        assert placed==21
        call('selection.set',{'bodyId':''});call('view.home');stage('04-complete-die')
        state=inspect();assert all(f['ownerId']==definition for f in state['authored']['features'])
        step('Save the editable component and export a millimeter STL')
        call('component.activate',{'definitionId':definition});call('document.save',{'path':str(model)})
        mesh_path=out/'die-mm.stl';receipt['export']=call('document.exportStl',{'occurrenceId':occurrence,'path':str(mesh_path),'units':'mm','format':'binary','surfaceDeviationMm':.05,'normalDeviationDeg':8})
        assert receipt['export']['bodyIds']==[target];receipt['mesh']=inspect_stl(mesh_path,True)
        assert len(receipt['mesh']['components'])==1
        assert all(abs(receipt['mesh']['bounds'][1][k]-receipt['mesh']['bounds'][0][k]-20)<.01 for k in range(3))
        if recording:time.sleep(2)
        stop()
        call('feature.edit',{'featureId':rounding['id'],'parameters':{**rounding['parameters'],'radiusMm':.8}})
        assert body()['valid'] and {f['faceId'] for f in body()['supportFaces']}==face_ids
        call('history.undo');assert inspect()['authored']==state['authored'];call('history.redo');call('history.undo')
        call('document.open',{'path':str(model)});assert inspect()['authored']==state['authored']
        receipt.update(result='api-pass',model=str(model),body=body())
finally:
    out.mkdir(parents=True,exist_ok=True);(out/'die-tutorial-api.json').write_text(json.dumps(receipt,indent=2))
