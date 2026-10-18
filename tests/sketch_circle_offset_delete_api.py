#!/usr/bin/env python3
"""Circle Offset geometry versus association Delete through the real app."""
import argparse, hashlib, json
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,ledger):
    def session():return call('app.state')['activeSession']
    def draft():return session()['draftSketch']
    def params():return draft()['parameters']
    call('sketch.begin',{'plane':'XY','name':'Circle Offset Delete'});call('sketch.lookAt')
    call('sketch.addCircle',{'centerX':-8,'centerY':0,'diameter':30,'units':'mm'});source=params()['entities'][-1]
    call('sketch.dimension',{'kind':'diameter','entityId':source['id'],'value':30,'units':'mm'})
    call('sketch.constraint',{'type':'fixed','pointIds':[source['centerPointId']]})
    call('sketch.addCircle',{'centerX':28,'centerY':0,'diameter':8,'units':'mm'});unrelated=params()['entities'][-1]
    call('sketch.constraint',{'type':'fixed','entityIds':[unrelated['id']]})
    source_params=params()
    call('sketch.circleOffset',{'sourceEntityId':source['id'],'distance':-4,'units':'mm'})
    result=params()['entities'][-1];relation=params()['constraints'][-1];coincidence=params()['constraints'][-2]
    baseline=draft();index=session()['draftHistory']['index']
    for ids in [[source['id']],[result['id'],source['id']],[result['id'],'stale']]:
        before=session();call('sketch.deleteSelection',{'entityIds':ids},error='sketch_solve_failed');after=session()
        assert before['draftSketch']==after['draftSketch'] and before['draftHistory']==after['draftHistory']
    # Publicly documented glyph Delete removes the association alone.
    call('sketch.deleteSelection',{'constraintIds':[relation['id']]})
    association_removed=draft()
    assert params()['entities']==baseline['parameters']['entities'] and params()['points']==baseline['parameters']['points']
    assert params()['constraints']==[r for r in baseline['parameters']['constraints'] if r['id']!=relation['id']]
    assert coincidence in params()['constraints']
    call('sketch.dimension',{'kind':'radius','entityId':result['id'],'value':9,'units':'mm'})
    assert next(e for e in params()['entities'] if e['id']==result['id'])['radiusMm']==9
    assert next(e for e in params()['entities'] if e['id']==source['id'])['radiusMm']==15
    call('history.undo');assert draft()==association_removed
    call('history.undo');assert draft()==baseline
    call('sketch.selectionSet',{'entityIds':[result['id']]});call('capture.screenshot',{'path':str(out/'selected.png')})
    call('sketch.deleteSelection');changed=draft()
    assert params()==source_params and session()['draftHistory']['index']==index+1
    call('history.undo');assert draft()==baseline
    call('history.redo');assert draft()==changed
    call('capture.screenshot',{'path':str(out/'deleted.png')})
    call('sketch.finish');authored=call('document.inspect')['authored'];path=out/'circle-offset-deleted.alloy'
    call('document.save',{'path':str(path)});call('document.open',{'path':str(path),'discard':True});assert call('document.inspect')['authored']==authored
    call('capture.screenshot',{'path':str(out/'reopened.png')})
    gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated']
    (out/'receipt.json').write_text(json.dumps({'status':'pass','appSha256':hashlib.sha256(Path(a.app).read_bytes()).hexdigest(),'gpu':gpu,'source':source,'result':result,'offsetRelation':relation,'centerCoincidence':coincidence,'savedPath':str(path),'exactUndoRedoReopen':True},indent=2)+'\n')
    (out/'transcript.json').write_text(json.dumps(ledger,indent=2)+'\n')
print('PASS: Circle Offset result/source/association Delete, exact history/reopen and GPU')
