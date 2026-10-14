#!/usr/bin/env python3
"""Bounded real-app Cut-feature Mirror; the finished snap group is separate."""
import argparse
import json
from pathlib import Path
from app_harness import application

parser = argparse.ArgumentParser()
parser.add_argument('--app', required=True)
parser.add_argument('--output', required=True)
args = parser.parse_args()
out = Path(args.output).resolve()

def body(state):
    solids=state['evaluation']['bodies']
    assert len(solids)==1 and solids[0]['valid'] and solids[0]['closed'], solids
    return solids[0]

with application(args.app,out) as (call, ledger):
    call('window.resize', {'width':1400,'height':900})
    assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
    call('sketch.begin', {'plane':'XY','name':'Symmetric case stock'})
    call('sketch.centerRectangle', {'centerX':0,'centerY':0,
        'width':50,'height':80,'units':'mm','centerSnapTarget':{'kind':'origin'}})
    stock_sketch=call('app.state')['activeSession']['draftSketch']['id']
    call('sketch.finish')
    stock_profile=next(s for s in call('document.inspect')['evaluation']['sketches']
        if s['sketchId']==stock_sketch)['profiles'][0]['profileId']
    call('solid.extrude', {'sketchId':stock_sketch,'profileId':stock_profile,'distance':20,'units':'mm'})
    stock=body(call('document.inspect'))
    target=stock['bodyId']
    call('sketch.begin', {'plane':'XY','name':'Undercut seed'})
    call('sketch.centerRectangle', {'centerX':13,'centerY':23,
        'width':6,'height':6,'units':'mm'})
    cut_sketch=call('app.state')['activeSession']['draftSketch']['id']
    call('sketch.finish')
    cut_profile=next(s for s in call('document.inspect')['evaluation']['sketches']
        if s['sketchId']==cut_sketch)['profiles'][0]['profileId']
    call('solid.extrude', {'sketchId':cut_sketch,'profileId':cut_profile,
        'distance':5,'units':'mm','operation':'cut','targetBodyIds':[target]})
    seed=call('document.inspect')
    cut_id=seed['authored']['features'][-1]['id']
    assert abs(body(seed)['volumeMm3']-79820)<1e-5
    call('document.save', {'path':str(out/'mirror-seed.alloy')})
    inputs={'targetBodyId':target,'sourceFeatureIds':[cut_id],
        'mirrorPlane':{'kind':'origin','plane':'XZ'}}
    session=call('solid.featureMirrorDialog', inputs)
    assert session['previewed'] and session['geometryEvaluated']
    call('capture.dialog', {'path':str(out/'mirror-preview-panel.png')})
    call('capture.screenshot', {'path':str(out/'mirror-preview.png')})
    call('command.cancel', {'session':session['id']})
    assert call('document.inspect')['authored']==seed['authored']
    committed=call('solid.featureMirror', inputs)
    done=call('document.inspect')
    mirror_id=done['authored']['features'][-1]['id']
    assert done['authored']['features'][-1]['type']=='featureMirror'
    assert abs(body(done)['volumeMm3']-79640)<1e-5
    assert body(done)['bodyId']==target
    mirrored=[e for e in body(done)['solidEdges'] if e['available']
        and e['provenance'].get('generatorFeatureId')==mirror_id]
    assert mirrored, body(done)['solidEdges']
    call('solid.featureMirror', {**inputs,'sourceFeatureIds':[target]}, error='evaluation_failed')
    assert call('document.inspect')['authored']==done['authored']
    edit=call('feature.editDialog', {'featureId':mirror_id})
    assert edit['previewed']
    call('command.cancel', {'session':edit['id']})
    call('document.save', {'path':str(out/'mirrored-cut.alloy')})
    call('history.undo')
    assert abs(body(call('document.inspect'))['volumeMm3']-79820)<1e-5
    call('history.redo')
    assert call('document.inspect')['authored']==done['authored']
    call('document.open', {'path':str(out/'mirrored-cut.alloy')})
    reopened=call('document.inspect')
    assert reopened['authored']==done['authored']
    assert abs(body(reopened)['volumeMm3']-79640)<1e-5
    call('capture.screenshot', {'path':str(out/'mirrored-cut.png')})
    (out/'report.json').write_text(json.dumps({'passed':True,'scope':'Cut-feature Mirror ingredient only',
        'mirrorFeatureId':mirror_id,'mirroredEdgeCount':len(mirrored),
        'seedVolumeMm3':body(seed)['volumeMm3'],'resultVolumeMm3':body(done)['volumeMm3']},indent=2))
    (out/'transcript.json').write_text(json.dumps(ledger,indent=2))
