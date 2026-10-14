#!/usr/bin/env python3
"""Real-app body-then-Sketches component organization without geometry copies."""
import argparse
import json
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl

parser=argparse.ArgumentParser()
parser.add_argument('--app',required=True)
parser.add_argument('--output',required=True)
args=parser.parse_args()
out=Path(args.output).resolve()
with application(args.app,out) as (call,transcript):
    assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
    root=call('document.inspect')['authored']['documentId']
    call('sketch.begin',{'plane':'XY','name':'Source sketch'})
    call('sketch.centerRectangle',{'centerX':0,'centerY':0,'width':20,'height':10,'units':'mm'})
    call('sketch.finish')
    source=call('document.inspect')
    sketch=source['evaluation']['sketches'][0]
    sketch_id=sketch['sketchId']
    call('solid.extrude',{'sketchId':sketch_id,'profileId':sketch['profiles'][0]['profileId'],'distance':5,'units':'mm'})
    created=call('component.create',{'name':'Finished part','activate':False})
    owner,occurrence=created['definitionId'],created['occurrenceId']
    base=call('document.inspect')
    body=base['evaluation']['bodies'][0]
    body_id=body['bodyId']
    call('component.reparent',{'destinationId':owner,'bodyId':body_id})
    first=call('document.inspect')
    assert first['evaluation']['bodies'][0]['ownerId']==owner
    assert first['evaluation']['sketches'][0]['ownerId']==root
    assert first['authored']['features']==base['authored']['features']
    assert first['evaluation']['bodies'][0]['boundsMm']==body['boundsMm']
    call('view.home')
    call('capture.screenshot',{'path':str(out/'body-moved-sketches-root.png')})
    call('component.reparent',{'destinationId':owner,'sketchIds':[sketch_id]})
    both=call('document.inspect')
    assert both['evaluation']['sketches'][0]['ownerId']==owner
    assert both['authored']['features']==base['authored']['features']
    assert both['revision']==base['revision']+2
    call('capture.screenshot',{'path':str(out/'body-and-sketches-moved.png')})
    extrude=both['authored']['features'][1]
    edit_session=call('feature.editDialog',{'featureId':extrude['id']})
    call('command.preview',{'session':edit_session['id']})
    call('command.cancel',{'session':edit_session['id']})
    assert call('document.inspect')['authored']==both['authored']
    call('solid.move',{'targetBodyId':body_id,'xMm':1},error='evaluation_failed')
    assert call('document.inspect')['authored']==both['authored']
    call('history.undo')
    assert call('document.inspect')['authored']==first['authored']
    call('history.undo')
    assert call('document.inspect')['authored']==base['authored']
    call('history.redo')
    call('history.redo')
    assert call('document.inspect')['authored']==both['authored']
    call('solid.extrude',{'featureId':extrude['id'],'sketchId':sketch_id,
        'profileId':sketch['profiles'][0]['profileId'],'distance':8,'units':'mm'})
    edited=call('document.inspect')
    assert edited['evaluation']['bodies'][0]['ownerId']==owner
    assert edited['evaluation']['bodies'][0]['bodyId']==body_id
    assert abs(edited['evaluation']['bodies'][0]['volumeMm3']-1600)<1e-5
    path=out/'organized.alloy'
    call('document.save',{'path':str(path)})
    call('document.open',{'path':str(path),'discard':True})
    reopened=call('document.inspect')
    assert reopened['authored']==edited['authored']
    stl=out/'finished-component.stl'
    export=call('document.exportStl',{'occurrenceId':occurrence,'path':str(stl),
        'units':'mm','format':'binary','surfaceDeviationMm':0.1,'normalDeviationDeg':10})
    assert export['bodyIds']==[body_id] and export['bodyCount']==1
    mesh=inspect_stl(stl,True)
    assert abs(mesh['bounds'][1][2]-mesh['bounds'][0][2]-8)<0.01
    call('view.home')
    call('capture.screenshot',{'path':str(out/'organized-reopened.png')})
    (out/'receipt.json').write_text(json.dumps({'base':base,'bodyOnly':first,'both':both,
        'edited':edited,'reopened':reopened,'export':export,'mesh':mesh,'transcript':transcript},indent=2)+'\n')
print('PASS real app body then Sketches ownership, refusal, source edit, history, reopen and scoped export')
