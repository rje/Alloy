#!/usr/bin/env python3
"""One real-entrypoint, bounded Sketch Text / Join / storage receipt."""
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
    def inspect():return call('document.inspect')
    try:
        call('sketch.begin',{'plane':'XY','name':'Floor outline','units':'mm'})
        call('sketch.rectangle',{'x1':0,'y1':-30,'x2':60,'y2':0,'units':'mm'})
        floor_sketch=call('app.state')['activeSession']['draftSketch']['id'];call('sketch.finish')
        floor_profile=inspect()['evaluation']['sketches'][0]['profiles'][0]['profileId']
        call('solid.extrude',{'sketchId':floor_sketch,'profileId':floor_profile,'distance':2,'units':'mm','name':'Key floor'})
        initial=inspect();floor=initial['evaluation']['bodies'][0]
        top=next(f for f in floor['supportFaces'] if f['frame']['normal']==[0,0,1] and math.isclose(f['frame']['origin'][2],2,abs_tol=1e-8))
        support={key:top[key] for key in ('producerFeatureId','bodyId','faceId')}
        call('sketch.begin',{'support':support,'name':'Raised initials','units':'mm'})
        created=call('sketch.textAdd',{'text':'DSB','height':14,'x':5,'y':-20,'angleDeg':0,'units':'mm'})
        source=created['draftSketch'];sketch_id=source['id'];text_id=source['parameters']['texts'][0]['id']
        call('sketch.finish')
        after_sketch=inspect();sketch=after_sketch['evaluation']['sketches'][-1]
        profiles=sketch['profiles'];assert len(profiles)==3 and [p['innerLoopCount'] for p in profiles]==[1,0,2]
        profile_ids=[p['profileId'] for p in profiles]
        call('solid.extrude',{'sketchId':sketch_id,'profileIds':profile_ids,'distance':1.5,'operation':'join',
                              'targetBodyIds':[floor['bodyId']],'units':'mm','name':'Raised letters'})
        joined=inspect();assert joined['bodies']==1
        area=sum(p['areaMm2'] for p in profiles)
        assert math.isclose(joined['evaluation']['bodies'][0]['volumeMm3'],3600+1.5*area,rel_tol=1e-5)
        before=joined['authored']
        call('sketch.edit',{'sketchId':sketch_id})
        call('sketch.textUpdate',{'textId':text_id,'text':'DS8','height':13,'x':7,'y':-19,'angleDeg':0,'units':'mm'})
        call('sketch.finish')
        edited=inspect();assert edited['bodies']==1
        current=edited['evaluation']['sketches'][-1]['profiles']
        assert [p['profileId'] for p in current]==profile_ids and [p['innerLoopCount'] for p in current]==[1,0,2]
        assert edited['evaluation']['bodies'][0]['volumeMm3']!=joined['evaluation']['bodies'][0]['volumeMm3']
        assert edited['authored']['features'][-2]['parameters']['texts'][0]['text']=='DS8'
        call('history.undo');assert inspect()['authored']==before
        call('history.redo');assert inspect()['authored']==edited['authored']
        path=out/'raised-initials.alloy';call('document.save',{'path':str(path)})
        call('document.open',{'path':str(path)})
        assert inspect()['authored']==edited['authored']
        call('view.orientation',{'name':'top'});call('view.home')
        call('capture.screenshot',{'path':str(out/'raised-initials.png')})
        gpu=call('app.state')['viewport']['gpu']
        (out/'report.json').write_text(json.dumps({'passed':True,'fontSha256':source['parameters']['texts'][0]['fontSha256'],
            'profileIds':profile_ids,'initialInnerLoops':[p['innerLoopCount'] for p in profiles],
            'editedInnerLoops':[p['innerLoopCount'] for p in current],
            'initialVolumeMm3':joined['evaluation']['bodies'][0]['volumeMm3'],
            'editedVolumeMm3':edited['evaluation']['bodies'][0]['volumeMm3'],
            'gpu':{'backend':gpu['backend'],'vendor':gpu['vendor'],'renderer':gpu['renderer'],'version':gpu['version'],
                   'hardwareAccelerated':gpu['hardwareAccelerated'],'framebufferWidth':gpu['framebufferWidth'],
                   'framebufferHeight':gpu['framebufferHeight']}},indent=2)+'\n')
    finally:
        (out/'transcript.json').write_text(json.dumps(transcript,indent=2)+'\n')
