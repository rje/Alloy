#!/usr/bin/env python3
"""Nested-cubes source workflow via real Alloy. Checkpoints never claim the full gate."""
import argparse
import json
import math
import time
from contextlib import ExitStack
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl


def support(face):
    return {k: face[k] for k in ('producerFeatureId', 'bodyId', 'faceId')}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--app', required=True)
    p.add_argument('--output', required=True)
    p.add_argument('--checkpoint', choices=['outer','complete'], default='complete')
    p.add_argument('--record-video', type=Path)
    p.add_argument('--pace', type=float, default=.65)
    a = p.parse_args()
    out = Path(a.output).resolve()
    receipt = {'result': 'incomplete', 'scope': a.checkpoint+' API workflow; native and visual source review separate',
               'source': 'external knowledge/tutorials/warwick/nested-cubes-ledger.json', 'stages': []}
    try:
        with application(a.app, out) as (raw_call, transcript), ExitStack() as guard:
            recording = False
            def stop_video():
                nonlocal recording
                if recording:
                    receipt['recording'] = raw_call('recording.stop')
                    recording = False
            guard.callback(stop_video)
            def call(command, parameters=None, error=None):
                labels = {'sketch.begin':'Create sketch','sketch.rectangle':'Draw the 25 mm square',
                          'sketch.addCircle':'Draw a 15 mm face bore','sketch.finish':'Finish sketch',
                          'solid.shell':'Shell inward by 2 mm','solid.mirror':'Mirror the shelled half',
                          'solid.fillet':'Round all 12 outside edges', 'construction.plane':'Offset a plane for the inner cube',
                          'sketch.centerRectangle':'Draw the centered 12 mm square', 'document.exportStl':'Export both bodies for printing'}
                if recording and command in labels: raw_call('recording.step', {'text':labels[command]})
                if recording and command=='solid.extrude': raw_call('recording.step', {'text':f"Extrude {parameters['distance']} mm: {parameters.get('operation','new body')}"})
                result=raw_call(command, parameters, error)
                if recording and (command.startswith(('sketch.','solid.','construction.')) or command=='command.commit'): time.sleep(a.pace)
                return result
            if a.record_video:
                video=a.record_video.resolve()
                assert not video.is_relative_to(Path(__file__).resolve().parents[1]), 'Keep media outside the codebase'
                video.parent.mkdir(parents=True,exist_ok=True)
                raw_call('window.resize', {'width':1400,'height':900})
                raw_call('recording.start', {'path':str(video),'fps':10})
                recording=True
                raw_call('recording.step', {'text':'Build nested cubes from a blank design'})
            receipt['transcript'] = transcript
            assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
            def inspect():
                return call('document.inspect')
            def bodies():
                return inspect()['evaluation']['bodies']
            def stage(name, count, volume=None):
                current = bodies()
                assert len(current) == count, (name, len(current))
                for b in current:
                    assert b['valid'] and b['closed'], (name, b)
                if volume is not None:
                    assert math.isclose(sum(b['volumeMm3'] for b in current), volume, abs_tol=1e-4), name
                call('view.home')
                call('capture.screenshot', {'path': str(out / (name+'.png'))})
                receipt['stages'].append({'name': name, 'bodies': current})
                print('PASS checkpoint:', name, flush=True)
            session = call('sketch.begin', {'plane':'XY', 'name':'Outer cube base'})
            call('sketch.rectangle', {'x1':0,'y1':0,'x2':25,'y2':25,'units':'mm'})
            draft = call('command.preview', {'session':session['id']})['authoredDraft']
            for edge, kind in zip(draft['parameters']['entities'][:2], ['horizontal','vertical']):
                call('sketch.dimension', {'kind':kind,'entityId':edge['id'],'value':25,'units':'mm'})
            call('sketch.finish')
            base = inspect()['evaluation']['sketches'][-1]
            call('solid.extrude', {'sketchId':base['sketchId'],'profileId':base['profiles'][0]['profileId'],
                                  'distance':12.5,'units':'mm'})
            stage('01-base',1,7812.5)
            lower = bodies()[0]
            top = next(f for f in lower['supportFaces'] if f['role']=='extrude.endCap')
            call('solid.shell', {'targetBodyId':lower['bodyId'],'removedFace':support(top),'thicknessMm':2})
            stage('02-shell',1,3182)
            call('solid.mirror', {'sourceBodyId':lower['bodyId'],'mirrorPlane':support(top)})
            stage('03-mirror',2,6364)
            upper = next(b for b in bodies() if b['bodyId'] != lower['bodyId'])
            combine = call('command.begin', {'command':'solid.combine', 'inputs':{
                'operation':'join','targetBodyId':upper['bodyId'],'toolBodyIds':[lower['bodyId']],'keepTools':False}})
            call('command.preview', {'session':combine['id']})
            call('command.commit', {'session':combine['id']})
            stage('04-hollow-cube',1,6364)
            for axis in range(3):
                stock = bodies()[0]
                choices = [f for f in stock['supportFaces'] if f['frame']['normal'][axis] > .999
                           and abs(f['frame']['origin'][axis]-25) < 1e-6]
                assert choices, ('missing outward face', axis, stock['supportFaces'])
                face = choices[0]
                center = [12.5,12.5,12.5]; center[axis] = 25
                f = face['frame']
                delta = [c-o for c,o in zip(center,f['origin'])]
                u = sum(x*y for x,y in zip(delta,f['uAxis']))
                v = sum(x*y for x,y in zip(delta,f['vAxis']))
                session = call('sketch.begin', {'support':support(face),'name':f'Face bore {axis+1}'})
                call('sketch.addCircle', {'centerX':u,'centerY':v,'diameter':15,'units':'mm'})
                circle = call('command.preview', {'session':session['id']})['authoredDraft']['parameters']['entities'][-1]['id']
                call('sketch.dimension', {'kind':'diameter','entityId':circle,'value':15,'units':'mm'})
                call('sketch.finish')
                sketch = inspect()['evaluation']['sketches'][-1]
                call('solid.extrude', {'sketchId':sketch['sketchId'],'profileId':sketch['profiles'][0]['profileId'],
                    'distance':(-37 if axis==0 else -31),'units':'mm','operation':'cut','targetBodyIds':[stock['bodyId']]})
                stage(f'05-cut-{axis+1}',1,6364-(axis+1)*4*math.pi*7.5**2)
            # Fixture selectors identify source outside corners, then persist actual edge IDs.
            edges = {}
            for e in bodies()[0]['solidEdges']:
                if not e['available']: continue
                first, last = e['startMm'], e['endMm']
                constant = [i for i in range(3) if abs(first[i]-last[i])<1e-6
                            and min(abs(first[i]),abs(first[i]-25))<1e-6]
                if len(constant)!=2: continue
                if not math.isclose(math.dist(first,last),25,abs_tol=1e-6): continue
                key = tuple(sorted(tuple(round(x,5) for x in pt) for pt in (first,last)))
                edges.setdefault(key,e['edgeId'])
            assert len(edges)==12, ('named outer corner edges',len(edges))
            call('solid.fillet', {'targetBodyId':bodies()[0]['bodyId'],'edgeIds':list(edges.values()),'radiusMm':1.5,'tangentChain':True})
            stage('06-rounded-outer-cube',1)
            top = next(f for f in bodies()[0]['supportFaces'] if f['frame']['normal'][2]>.999
                       and abs(f['frame']['origin'][2]-25)<1e-6)
            plane = call('construction.plane', {'source':{'support':support(top)},'offset':-18.5,'units':'mm','name':'Inner cube plane'})['planeId']
            stage('07-inner-plane',1)
            if a.checkpoint=='outer':
                call('document.save', {'path':str(out/'outer-cube-checkpoint.alloy')})
                receipt['result']='checkpoint-pass'
                return
            frame=next(p['frame'] for p in inspect()['evaluation']['constructionPlanes'] if p['featureId']==plane)
            delta=[12.5-frame['origin'][0],12.5-frame['origin'][1],6.5-frame['origin'][2]]
            center=[sum(x*y for x,y in zip(delta,frame[axis])) for axis in ('uAxis','vAxis')]
            call('selection.planeSet', {'planeId':plane})
            call('sketch.create', {'name':'Centered inner cube'})
            call('sketch.centerRectangle', {'centerX':center[0],'centerY':center[1],'width':12,'height':12,'units':'mm'})
            call('sketch.finish')
            inside=inspect()['evaluation']['sketches'][-1]
            call('solid.extrude', {'sketchId':inside['sketchId'],'profileId':inside['profiles'][0]['profileId'], 'distance':12,'units':'mm'})
            stage('08-complete-nested-cubes',2)
            inner=bodies()[-1]
            assert math.isclose(inner['volumeMm3'],1728,abs_tol=1e-5)
            model=out/'nested-cubes.alloy'
            call('document.save', {'path':str(model)})
            mesh_path=out/'nested-cubes-mm.stl'
            receipt['export']=call('document.exportStl', {'path':str(mesh_path),'bodyIds':[b['bodyId'] for b in bodies()],
                'units':'mm','format':'binary','surfaceDeviationMm':.05,'normalDeviationDeg':10})
            mesh=inspect_stl(mesh_path,True)
            assert len(mesh['components'])==2
            inner_mesh=min(mesh['components'],key=lambda c:c['signedVolume'])
            assert math.isclose(inner_mesh['signedVolume'],1728,abs_tol=.01)
            assert inner_mesh['bounds']==[[6.5,6.5,6.5],[18.5,18.5,18.5]], inner_mesh
            receipt['mesh']=mesh
            stop_video()
            accepted=inspect()
            shell=next(f for f in accepted['authored']['features'] if f['type']=='shell')
            call('solid.shell', dict(shell['parameters'],featureId=shell['id'],thicknessMm=1.8))
            assert len(bodies())==2 and bodies()[0]['valid']
            call('history.undo')
            assert inspect()['authored']==accepted['authored']
            call('history.redo')
            call('history.undo')
            assert inspect()['authored']==accepted['authored']
            call('document.new', {'discard':True})
            call('document.open', {'path':str(model),'discard':True})
            reopened=inspect()
            assert reopened['authored']==accepted['authored']
            assert [b['volumeMm3'] for b in reopened['evaluation']['bodies']]==[b['volumeMm3'] for b in accepted['evaluation']['bodies']]
            receipt['result']='api-pass'
            receipt['model']=str(model)
    finally:
        out.mkdir(parents=True,exist_ok=True)
        (out/'nested-cubes-api.json').write_text(json.dumps(receipt,indent=2))


if __name__ == '__main__':
    main()
