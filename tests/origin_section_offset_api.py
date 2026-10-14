#!/usr/bin/env python3
"""Origin Section Analysis Distance and Flip through the real application."""
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
    section = lambda: call('app.state')['viewport']['gpu']['sectionAnalysis']
    call('solid.box', {'width':64,'depth':80,'height':30,'units':'mm'})
    call('view.home')
    before = inspect()
    call('inspect.section', {'plane':'XZ','name':'Width'})
    legacy = inspect()
    assert len(legacy['authored']['analyses'][0])==6
    analysis_id = legacy['authored']['analyses'][0]['id']
    call('inspect.section', {'analysisId':analysis_id,'offsetMm':32,'flip':True})
    moved = inspect()
    plane = section()['resolvedPlane']
    assert plane=={'origin':[0,32,0],'normal':[0,-1,0]}
    assert moved['authored']['features']==before['authored']['features']
    assert moved['evaluation']['bodies'][0]['volumeMm3']==before['evaluation']['bodies'][0]['volumeMm3']
    call('view.orientation', {'name':'back'})
    call('capture.screenshot', {'path':str(out/'origin-offset-flipped.png')})
    call('history.undo')
    assert inspect()['authored']==legacy['authored'] and section()['resolvedPlane']['origin']==[0,0,0]
    call('history.redo')
    assert inspect()['authored']==moved['authored'] and section()['resolvedPlane']==plane
    path=out/'origin-offset.alloy'
    call('document.save', {'path':str(path)})
    call('document.open', {'path':str(path)})
    assert section()['resolvedPlane']==plane and inspect()['authored']==moved['authored']
    gpu=call('app.state')['viewport']['gpu']
    assert gpu['hardwareAccelerated'] and gpu['sectionClippedActors']==1
    (out/'report.json').write_text(json.dumps({'passed':True,'calls':len(transcript),'resolvedPlane':plane,'gpu':gpu},indent=2))
    (out/'transcript.json').write_text(json.dumps(transcript,indent=2))
