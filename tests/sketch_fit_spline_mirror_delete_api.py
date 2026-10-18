#!/usr/bin/env python3
"""Retained mixed fit-spline Mirror whole-owner Delete through the real app."""
import argparse, hashlib, json
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,ledger):
    def session():return call('app.state')['activeSession']
    def draft():return session()['draftSketch']
    def params():return draft()['parameters']
    call('sketch.begin',{'plane':'XY','name':'Mixed spline Mirror Delete'});call('sketch.lookAt')
    call('sketch.addCircle',{'centerX':-30,'centerY':-25,'diameter':8,'units':'mm'});circle=params()['entities'][-1]
    call('sketch.constraint',{'type':'fixed','entityIds':[circle['id']]})
    call('sketch.line',{'x1':-40,'y1':0,'x2':40,'y2':0,'units':'mm'});axis=params()['entities'][-1]
    call('sketch.construction',{'entityIds':[axis['id']],'construction':True})
    call('sketch.line',{'x1':5,'y1':0,'x2':12,'y2':14,'units':'mm'});line=params()['entities'][-1]
    call('sketch.addArc',{'mode':'center','centerX':24,'centerY':15,'radius':4,'startAngleDeg':0,'sweepAngleDeg':120,'units':'mm'});arc=params()['entities'][-1]
    call('sketch.splineAdd',{'points':[{'uMm':-25,'vMm':0},{'uMm':-18,'vMm':20},{'uMm':-4,'vMm':16}]});spline=params()['entities'][-1]
    call('sketch.splineTangent',{'entityId':spline['id'],'pointId':spline['pointIds'][-1],'duMm':20,'dvMm':-10})
    source=params();source_entities=source['entities'];source_points=source['points']
    call('sketch.mirrorPreview',{'sourceEntityIds':[spline['id'],line['id'],arc['id']],'axisEntityId':axis['id']});call('sketch.mirrorAccept')
    g=params()['mirrorGroups'][0];target=g['entityPairs'][0]['resultEntityId'];result=next(e for e in params()['entities'] if e['id']==target)
    assert result['pointIds'][0]==spline['pointIds'][0] and result['tangentHandles']
    baseline=draft();index=session()['draftHistory']['index']
    for ids in [[spline['id']],[axis['id']],[target,spline['id']],[target,'stale']]:
        before=session();call('sketch.deleteSelection',{'entityIds':ids},error='sketch_solve_failed');after=session()
        assert before['draftSketch']==after['draftSketch'] and before['draftHistory']==after['draftHistory']
    call('sketch.selectionSet',{'entityIds':[target]});call('capture.screenshot',{'path':str(out/'selected.png')})
    call('sketch.deleteSelection');changed=draft();new=params()
    assert 'mirrorGroups' not in new and session()['draftHistory']['index']==index+1
    assert new['entities']==source_entities and new['points']==source_points
    call('history.undo');assert draft()==baseline
    call('history.redo');assert draft()==changed
    call('capture.screenshot',{'path':str(out/'deleted.png')})
    call('sketch.finish');authored=call('document.inspect')['authored'];path=out/'fit-spline-mirror-deleted.alloy'
    call('document.save',{'path':str(path)});call('document.open',{'path':str(path),'discard':True});assert call('document.inspect')['authored']==authored
    call('capture.screenshot',{'path':str(out/'reopened.png')})
    gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated']
    (out/'receipt.json').write_text(json.dumps({'status':'pass','appSha256':hashlib.sha256(Path(a.app).read_bytes()).hexdigest(),'gpu':gpu,'group':g,'onAxisPointId':spline['pointIds'][0],'sourceEntityIds':[e['id'] for e in source_entities],'savedPath':str(path),'exactUndoRedoReopen':True},indent=2)+'\n')
    (out/'transcript.json').write_text(json.dumps(ledger,indent=2)+'\n')
print('PASS: mixed fit-spline Mirror Delete, shared/source IDs, tangent, history/reopen and GPU')
