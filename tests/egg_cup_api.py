#!/usr/bin/env python3
"""Complete public egg-cup workflow through shared commands; native review separate."""
import argparse
import json
import math
import time
from contextlib import ExitStack
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--app',required=True)
p.add_argument('--output',required=True)
p.add_argument('--record-video',type=Path)
a=p.parse_args();out=Path(a.output).resolve()
receipt={'result':'incomplete','source':'external knowledge/tutorials/warwick/egg-cup-ledger.json'}
try:
    with application(a.app,out) as (raw,transcript), ExitStack() as cleanup:
        receipt['transcript']=transcript
        recording=False
        def stop():
            global recording
            if recording:receipt['recording']=raw('recording.stop');recording=False
        cleanup.callback(stop)
        if a.record_video:
            movie=a.record_video.resolve()
            assert not movie.is_relative_to(Path(__file__).resolve().parents[1])
            movie.parent.mkdir(parents=True,exist_ok=True)
            raw('window.resize',{'width':1400,'height':900})
            raw('recording.start',{'path':str(movie),'fps':10});recording=True
        def step(text):
            if recording:raw('recording.step',{'text':text})
        def call(command,parameters=None):
            result=raw(command,parameters)
            if recording and command.startswith(('sketch.','construction.','solid.','command.commit')):time.sleep(.8)
            return result
        assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
        previous=None;references=[];planes=[]
        for i,(diameter,z) in enumerate(((38,0),(16,18),(30,38),(44,58))):
            step(f'Circle diameter {diameter} mm at height {z} mm')
            if i:
                plane=call('construction.plane',{'source':{'sketchId':previous},'offset':18 if i==1 else 20,'units':'mm'})['planeId']
                planes.append(plane);call('selection.planeSet',{'planeId':plane});call('sketch.create',{'name':f'Section {diameter} at {z}'})
            else:call('sketch.begin',{'plane':'XY','name':'Base section 38'})
            call('sketch.addCircle',{'centerX':0,'centerY':0,'diameter':diameter,'units':'mm'})
            call('sketch.finish');sketch=call('document.inspect')['evaluation']['sketches'][-1]
            previous=sketch['sketchId'];references.append({'sketchId':previous,'profileId':sketch['profiles'][0]['profileId']})
            call('view.home')
        call('capture.screenshot',{'path':str(out/'01-four-sections.png')})
        step('Loft through all four profiles in bottom-to-top order')
        session=call('solid.loftDialog',{'profiles':references,'name':'Egg cup Loft'})
        preview=call('command.preview',{'session':session['id']});assert preview['geometry']['valid']
        call('capture.screenshot',{'path':str(out/'02-loft-preview.png')})
        call('command.commit',{'session':session['id']})
        state=call('document.inspect');body=state['evaluation']['bodies'][0]
        for feature in state['authored']['features']:
            if feature['type'] in ('sketch','constructionPlane'):call('feature.visibility',{'featureId':feature['id'],'visible':False})
        cap=next(f for f in body['supportFaces'] if f['role']=='loft.end')
        step('Remove the top face and Shell inward by 2 mm')
        call('solid.shell',{'targetBodyId':body['bodyId'],'removedFace':{k:cap[k] for k in ('producerFeatureId','bodyId','faceId')},'thicknessMm':2})
        call('selection.set',{'bodyId':''});call('view.home')
        call('capture.screenshot',{'path':str(out/'03-egg-cup.png')})
        state=call('document.inspect');body=state['evaluation']['bodies'][0]
        assert len(state['evaluation']['bodies'])==1 and body['valid'] and body['closed']
        assert math.isclose(body['boundsMm']['max'][2],58,abs_tol=1e-4)
        path=out/'egg-cup.alloy';call('document.save',{'path':str(path)})
        step('Save the editable cup and export a millimeter STL')
        mesh_path=out/'egg-cup-mm.stl'
        receipt['export']=call('document.exportStl',{'path':str(mesh_path),'bodyId':body['bodyId'],'units':'mm','format':'binary','surfaceDeviationMm':.05,'normalDeviationDeg':10})
        receipt['mesh']=inspect_stl(mesh_path,True)
        assert len(receipt['mesh']['components'])==1
        assert math.isclose(receipt['mesh']['signedVolume'],body['volumeMm3'],rel_tol=.01)
        stop()
        call('document.new',{'discard':True});call('document.open',{'path':str(path),'discard':True})
        assert call('document.inspect')['authored']==state['authored']
        receipt.update(result='api-pass',model=str(path),geometry=body)
finally:
    out.mkdir(parents=True,exist_ok=True);(out/'egg-cup-api.json').write_text(json.dumps(receipt,indent=2))
