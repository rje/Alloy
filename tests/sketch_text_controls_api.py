#!/usr/bin/env python3
"""Bounded real-app Sketch Text control and joined-profile receipt."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application

parser=argparse.ArgumentParser()
parser.add_argument('--app',required=True)
parser.add_argument('--output',required=True)
args=parser.parse_args()
out=Path(args.output).resolve();out.mkdir(parents=True,exist_ok=True)
with application(args.app,out) as (call,transcript):
    try:
        call('sketch.begin',{'plane':'XY','units':'mm'})
        call('sketch.rectangle',{'x1':0,'y1':-30,'x2':60,'y2':0,'units':'mm'})
        base=call('app.state')['activeSession']['draftSketch']['id']
        call('sketch.finish')
        profile=call('document.inspect')['evaluation']['sketches'][0]['profiles'][0]['profileId']
        call('solid.extrude',{'sketchId':base,'profileId':profile,'distance':2,'units':'mm'})
        body=call('document.inspect')['evaluation']['bodies'][0]
        face=next(f for f in body['supportFaces'] if f['frame']['normal']==[0,0,1] and math.isclose(f['frame']['origin'][2],2))
        support={k:face[k] for k in ('producerFeatureId','bodyId','faceId')}
        call('sketch.text',{'support':support,'name':'Raised initials','units':'mm'})
        call('sketch.textDialog')
        before=call('app.state')['activeSession']['draftSketch']
        call('sketch.textPreview',{'text':'DSB','heightMm':14,'uMm':5,'vMm':-20})
        assert call('app.state')['activeSession']['draftSketch']==before
        call('sketch.textCancel')
        assert call('app.state')['activeSession']['draftSketch']==before
        call('sketch.textDialog')
        call('sketch.textPreview',{'text':'DSB','fontId':'dejavu-sans-2.37-bold','heightMm':14,'uMm':5,'vMm':-20})
        call('capture.dialog',{'path':str(out/'text-dialog.png')})
        call('capture.screenshot',{'path':str(out/'text-preview.png')})
        sketch_id=call('app.state')['activeSession']['draftSketch']['id']
        project=lambda u,v:call('view.projectSketchPoint',{'sketchId':sketch_id,'uMm':u,'vMm':v})
        a=project(5,-20);b=project(50,-5)
        pointer=lambda phase,mode,start,end:call('sketch.textHandle',{'mode':mode,'phase':phase,
            'startX':start['x'],'startY':start['y'],'x':end['x'],'y':end['y']})
        pointer('begin','move',a,a);pointer('move','move',a,b);pointer('end','move',a,b)
        text=call('app.state')['activeSession']['pendingText']
        assert math.isclose(text['uMm'],50,abs_tol=.2) and math.isclose(text['vMm'],-5,abs_tol=.2)
        arm=max(8,14*1.35)
        a=project(50+arm,-5);b=project(50-arm,-5)
        pointer('begin','rotate',a,a);pointer('move','rotate',a,b);pointer('end','rotate',a,b)
        text=call('app.state')['activeSession']['pendingText']
        assert math.isclose(abs(text['angleDeg']),180,abs_tol=1)
        call('sketch.textAccept')
        sketch=call('app.state')['activeSession']['draftSketch'];sketch_id=sketch['id']
        assert sketch['parameters']['texts'][0]['fontId']=='dejavu-sans-2.37-bold'
        assert math.isclose(abs(sketch['parameters']['texts'][0]['angleDeg']),180,abs_tol=1)
        call('sketch.finish')
        evaluation=call('document.inspect')['evaluation']
        profiles=evaluation['sketches'][-1]['profiles']
        assert [p['innerLoopCount'] for p in profiles]==[1,0,2]
        call('solid.extrude',{'sketchId':sketch_id,'profileIds':[p['profileId'] for p in profiles],
            'distance':1.5,'operation':'join','targetBodyIds':[body['bodyId']],'units':'mm'})
        assert call('document.inspect')['bodies']==1
        call('view.orientation',{'name':'top'});call('view.home')
        call('capture.screenshot',{'path':str(out/'text-joined.png')})
        report={'passed':True,'textId':text['id'],'fontId':text['fontId'],'placement':[text['uMm'],text['vMm'],text['angleDeg']],
            'innerLoops':[p['innerLoopCount'] for p in profiles],'gpu':call('app.state')['viewport']['gpu']}
        (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    finally:
        (out/'transcript.json').write_text(json.dumps(transcript,indent=2)+'\n')
