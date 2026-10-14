#!/usr/bin/env python3
"""Retained planar-face section through Alloy's local application control API."""
import argparse
import json
from pathlib import Path
from app_harness import application

parser = argparse.ArgumentParser()
parser.add_argument('--app', required=True)
parser.add_argument('--output', required=True)
args = parser.parse_args()
out = Path(args.output).resolve()

with application(args.app, out) as (call, transcript):
    inspect = lambda: call('document.inspect')
    gpu = lambda: call('app.state')['viewport']['gpu']
    call('solid.box', {'width':40, 'depth':30, 'height':20, 'units':'mm'})
    call('view.home')
    before = inspect()
    body = before['evaluation']['bodies'][0]
    face = next(f for f in body['supportFaces'] if f['role']=='box.z1')
    support = {k:face[k] for k in ('producerFeatureId','bodyId','faceId')}
    point = call('view.projectFacePoint', {'support':support,'uMm':20,'vMm':15})
    assert call('selection.facePick', {'x':point['x'],'y':point['y']})['support']==support
    session = call('inspect.sectionDialog', {'name':'Top section'})
    assert session['inputs']['plane']=='face' and session['inputs']['support']==support
    call('command.update', {'session':session['id'],'inputs':{'offsetMm':-4.0}})
    preview = gpu()['sectionAnalysis']
    assert preview['resolvedPlane']['origin']==[0,0,16] and preview['resolvedPlane']['normal']==[0,0,1]
    call('capture.screenshot', {'path':str(out/'preview.png')})
    call('command.cancel', {'session':session['id']})
    assert inspect()==before and gpu()['sectionAnalysis']=={}
    call('inspect.section', {'plane':'face','support':support,'offsetMm':-4.0,'name':'Top section'})
    first = inspect()
    assert first['authored']['features']==before['authored']['features']
    row = call('analysis.inspect')['analyses'][0]
    assert row['availability']=='available' and row['resolvedPlane']['origin']==[0,0,16]
    analysis_id = row['id']
    call('inspect.section', {'analysisId':analysis_id,'flip':True})
    flipped = call('analysis.inspect')['analyses'][0]
    assert flipped['resolvedPlane']['origin']==row['resolvedPlane']['origin']
    assert flipped['resolvedPlane']['normal']==[0,0,-1]
    call('capture.screenshot', {'path':str(out/'flipped.png')})
    call('solid.box', {'featureId':body['featureId'],'width':40,'depth':30,'height':28,'units':'mm'})
    followed = call('analysis.inspect')['analyses'][0]
    assert followed['support']==support and followed['resolvedPlane']['origin']==[0,0,24]
    saved = inspect()['authored']
    path = out/'selected-face.alloy'
    call('document.save', {'path':str(path)})
    call('document.open', {'path':str(path)})
    assert inspect()['authored']==saved and call('analysis.inspect')['analyses'][0]['resolvedPlane']['origin']==[0,0,24]
    call('feature.delete', {'featureId':body['featureId']})
    unavailable = call('analysis.inspect')['analyses'][0]
    assert unavailable['availability']=='unavailable' and unavailable['support']==support
    assert gpu()['sectionClippedActors']==0
    call('inspect.section', {'plane':'face','support':support}, error='support_unavailable')
    call('history.undo')
    restored = call('analysis.inspect')['analyses'][0]
    assert restored['availability']=='available' and restored['resolvedPlane']['origin']==[0,0,24]
    assert gpu()['sectionClippedActors']==1
    (out/'report.json').write_text(json.dumps({'passed':True,'calls':len(transcript),'support':support,'analysisId':analysis_id,'gpu':gpu()},indent=2))
    (out/'transcript.json').write_text(json.dumps(transcript,indent=2))
