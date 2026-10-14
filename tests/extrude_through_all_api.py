#!/usr/bin/env python3
"""Real-app retained one-target Extrude Cut All."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application

parser=argparse.ArgumentParser()
parser.add_argument('--app',required=True)
parser.add_argument('--output',required=True)
args=parser.parse_args()
out=Path(args.output).resolve()
with application(args.app,out) as (call,transcript):
    assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
    call('solid.box',{'width':20,'depth':20,'height':10,'x':0,'y':0,'z':0,'units':'mm'})
    box=call('document.inspect')['authored']['features'][0]
    target=box['bodyId']
    sketch=call('sketch.begin',{'plane':'XY','name':'Through cutter'})['draftSketch']['id']
    call('sketch.addCircle',{'centerX':10,'centerY':10,'diameter':4,'units':'mm'})
    call('sketch.finish')
    source=call('document.inspect')
    profile=source['evaluation']['sketches'][-1]['profiles'][0]['profileId']
    inputs={'sketchId':sketch,'profileId':profile,'extent':'all','operation':'cut','targetBodyIds':[target]}
    session=call('solid.extrudeDialog',inputs)
    preview=call('command.preview',{'session':session['id']})
    assert preview['resolvedDistanceMm']>10 and math.isclose(preview['geometry']['volumeMm3'],4000-40*math.pi,abs_tol=1e-4)
    assert call('document.inspect')==source
    call('view.home')
    call('capture.dialog',{'path':str(out/'through-all-dialog.png')})
    call('capture.screenshot',{'path':str(out/'through-all-preview.png')})
    call('command.cancel',{'session':session['id']})
    assert call('document.inspect')==source
    call('solid.extrude',inputs)
    accepted=call('document.inspect')
    feature=accepted['authored']['features'][-1]
    assert feature['parameters']['extent']=='all' and 'distanceMm' not in feature['parameters']
    assert math.isclose(accepted['evaluation']['bodies'][0]['volumeMm3'],4000-40*math.pi,abs_tol=1e-4)
    call('solid.box',{'featureId':box['id'],'width':20,'depth':20,'height':20,'x':0,'y':0,'z':0,'units':'mm'})
    tall=call('document.inspect')
    assert math.isclose(tall['evaluation']['bodies'][0]['volumeMm3'],8000-80*math.pi,abs_tol=1e-4)
    assert tall['evaluation']['bodies'][0]['distanceMm']>20
    call('history.undo')
    assert call('document.inspect')['authored']==accepted['authored']
    call('history.redo')
    edit=call('feature.editDialog',{'featureId':feature['id']})
    assert edit['inputs']['extent']=='all' and edit['inputs']['flip'] is False and 'distance' not in edit['inputs']
    call('command.cancel',{'session':edit['id']})
    path=out/'through-all-cut.alloy'
    call('document.save',{'path':str(path)})
    call('document.open',{'path':str(path),'discard':True})
    assert call('document.inspect')['authored']==tall['authored']
    call('view.home')
    call('capture.screenshot',{'path':str(out/'through-all-reopened.png')})
    (out/'receipt.json').write_text(json.dumps({'source':source,'accepted':accepted,'tall':tall,'transcript':transcript},indent=2)+'\n')
print('PASS real app All preview/Cancel, through target resize, edit, history and reopen')
