#!/usr/bin/env python3
"""SRF-014 real-entrypoint Two Sides open-curve surface extrusion."""
import argparse
import json
import uuid
from pathlib import Path
from app_harness import application

parser=argparse.ArgumentParser()
parser.add_argument('--app',required=True)
parser.add_argument('--output',required=True)
args=parser.parse_args();out=Path(args.output).resolve();out.mkdir(parents=True,exist_ok=True)

def surface(state,identity):
    return next(body for body in state['evaluation']['bodies'] if body['bodyId']==identity)

with application(args.app,out,command_timeout=90) as (call,ledger):
    call('window.resize',{'width':1400,'height':980})
    assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
    call('sketch.begin',{'plane':'XZ','name':'Back and bottom source'})
    call('sketch.splineAdd',{'points':[{'uMm':0,'vMm':0},{'uMm':15,'vMm':13},{'uMm':25,'vMm':10}]})
    call('sketch.splineAdd',{'points':[{'uMm':25,'vMm':10},{'uMm':35,'vMm':13},{'uMm':50,'vMm':0}]})
    call('sketch.line',{'x1':25,'y1':10,'x2':25,'y2':5,'units':'mm'})
    source=call('app.state')['activeSession']['draftSketch'];sketch_id=source['id']
    source_ids=[entity['id'] for entity in source['parameters']['entities']]
    assert [entity['type'] for entity in source['parameters']['entities']]==['fitSpline','fitSpline','line']
    call('sketch.finish')
    original=call('document.inspect')

    session=call('surface.extrudeDialog',{'sideOneMm':25,'sideTwoMm':30,'name':'Back sheets'})
    assert session['command']=='surface.extrude' and not session['previewed']
    for entity in source_ids:call('surface.extrudeEntity',{'sketchId':sketch_id,'entityId':entity})
    preview=call('command.preview',{'session':session['id']})
    assert preview['geometry']['type']=='surface' and preview['geometry']['topology']['faces']==3
    assert preview['geometry']['topology']['solids']==0 and preview['geometry']['volumeMm3'] is None
    call('capture.dialog',{'path':str(out/'two-sides-controls.png')})
    call('capture.screenshot',{'path':str(out/'three-sheets-preview.png')})
    call('command.cancel',{'session':session['id']})
    assert call('document.inspect')['authored']==original['authored']

    session=call('surface.extrudeDialog',{'sketchId':sketch_id,'entityIds':source_ids,
        'sideOneMm':25,'sideTwoMm':30,'name':'Back sheets'})
    assert session['previewed']
    call('command.commit',{'session':session['id']})
    committed=call('document.inspect')
    feature=committed['authored']['features'][-1];body=surface(committed,feature['bodyId'])
    assert feature['type']=='surfaceExtrude' and feature['parameters']['entityIds']==source_ids
    assert body['type']=='surface' and not body['closed'] and body['valid']
    assert body['topology']['faces']==3 and body['topology']['solids']==0
    assert body['boundsMm']['min'][1]<-29.99 and body['boundsMm']['max'][1]>24.99
    old_ids=[face['faceId'] for face in body['supportFaces']]
    call('sketch.edit',{'sketchId':sketch_id})
    call('sketch.line',{'x1':0,'y1':0,'x2':50,'y2':0,'units':'mm'})
    edited_sketch=call('app.state')['activeSession']['draftSketch'];bottom_id=edited_sketch['parameters']['entities'][-1]['id']
    call('sketch.finish')
    assert surface(call('document.inspect'),feature['bodyId'])['topology']['faces']==3
    edit=call('feature.editDialog',{'featureId':feature['id']})
    assert edit['inputs']['entityIds']==source_ids
    picked=call('surface.extrudeEntity',{'sketchId':sketch_id,'entityId':bottom_id})
    assert picked.get('previewed') and len(picked['entityIds'])==4,picked
    changed=call('command.preview',{'session':edit['id']})
    assert changed['geometry']['topology']['faces']==4
    call('command.commit',{'session':edit['id']})
    final=call('document.inspect');current=surface(final,feature['bodyId'])
    assert [face['faceId'] for face in current['supportFaces']][:3]==old_ids
    assert current['topology']['faces']==4 and current['areaMm2']>body['areaMm2']
    call('view.home')
    call('capture.screenshot',{'path':str(out/'four-sheets-current.png')})

    edit=call('feature.editDialog',{'featureId':feature['id']})
    call('command.update',{'session':edit['id'],'inputs':{'sideOneMm':26}})
    assert call('command.preview',{'session':edit['id']})['geometry']['boundsMm']['max'][1]>25.99
    call('command.cancel',{'session':edit['id']})
    assert call('document.inspect')['authored']==final['authored']
    invalid={'featureId':feature['id'],'sketchId':sketch_id,'entityIds':[str(uuid.uuid4())],
        'sideOneMm':25,'sideTwoMm':30}
    call('surface.extrude',invalid,error='evaluation_failed')
    assert call('document.inspect')['authored']==final['authored']
    call('history.undo');assert surface(call('document.inspect'),feature['bodyId'])['topology']['faces']==3
    call('history.redo');assert call('document.inspect')['authored']==final['authored']
    model=out/'two-sides-surface.alloy';call('document.save',{'path':str(model)})
    call('document.new',{'discard':True});call('document.open',{'path':str(model),'discard':True})
    reopened=call('document.inspect')
    assert reopened['authored']==final['authored']
    assert [face['faceId'] for face in surface(reopened,feature['bodyId'])['supportFaces']][:3]==old_ids
    result={'passed':True,'featureId':feature['id'],'bodyId':feature['bodyId'],
        'initialFaceIds':old_ids,'finalFaces':current['topology']['faces'],
        'finalAreaMm2':current['areaMm2'],'initialAreaMm2':body['areaMm2'],
        'sourceEntityIds':source_ids,'addedBottomLineId':bottom_id,
        'sideOneMm':25,'sideTwoMm':30,'sourceDimensions':'Alloy fixture choices; public source gives none'}
    (out/'result.json').write_text(json.dumps(result,indent=2)+'\n')
    (out/'transcript.json').write_text(json.dumps(ledger,indent=2)+'\n')
