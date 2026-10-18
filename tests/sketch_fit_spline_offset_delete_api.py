#!/usr/bin/env python3
"""Retained fit-spline Offset result Delete through the real application."""
import argparse, hashlib, json
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,ledger):
    def session():return call('app.state')['activeSession']
    def draft():return session()['draftSketch']
    def params():return draft()['parameters']
    def atomic(ids):
        before=session();call('sketch.deleteSelection',{'entityIds':ids},error='sketch_solve_failed');after=session()
        assert before['draftSketch']==after['draftSketch'] and before['draftHistory']==after['draftHistory']
    def add_offset(distance):
        call('sketch.splineOffset',{'sourceEntityId':source['id'],'distance':distance,'units':'mm'})
        return params()['entities'][-1]
    call('sketch.begin',{'plane':'XY','name':'Fit spline Offset result Delete'});call('sketch.lookAt')
    call('sketch.addCircle',{'centerX':-30,'centerY':-15,'diameter':8,'units':'mm'});circle=params()['entities'][-1]
    call('sketch.constraint',{'type':'fixed','entityIds':[circle['id']]});relation=params()['constraints'][-1]
    call('sketch.splineAdd',{'points':[{'uMm':0,'vMm':-20},{'uMm':8,'vMm':20},{'uMm':-6,'vMm':60},{'uMm':4,'vMm':100}]});source=params()['entities'][-1]
    target=add_offset(-5);sibling=add_offset(-7)
    assert target['id']!=sibling['id']
    # Independent annotation Delete never expands to the spline/result owner.
    annotation_before=draft();call('sketch.deleteSelection',{'constraintIds':[relation['id']]})
    assert params()['entities']==annotation_before['parameters']['entities'] and params()['points']==annotation_before['parameters']['points']
    call('history.undo');assert draft()==annotation_before
    # Close the selected source/result strip with authored dependent Lines.
    points={r['id']:r for r in params()['points']}
    for source_id,result_id in [(source['startPointId'],target['startPointId']),(source['endPointId'],target['endPointId'])]:
        x,y=points[source_id],points[result_id]
        call('sketch.line',{'x1':x['uMm'],'y1':x['vMm'],'x2':y['uMm'],'y2':y['vMm'],'units':'mm','startSnapTarget':{'kind':'point','pointId':source_id},'endSnapTarget':{'kind':'point','pointId':result_id}})
    connectors=[r['id'] for r in params()['entities'] if r['type']=='line']
    atomic([target['id']]);atomic([source['id']]);atomic([source['id'],target['id']]);atomic([target['id'],source['id']]);atomic([target['id'],'stale'])
    call('sketch.deleteSelection',{'entityIds':connectors})
    baseline=draft();index=session()['draftHistory']['index'];expected=json.loads(json.dumps(params()))
    expected['entities']=[r for r in expected['entities'] if r['id']!=target['id']]
    expected['points']=[r for r in expected['points'] if r['id'] not in [target['startPointId'],target['endPointId']]]
    call('view.fit');call('sketch.selectionSet',{'entityIds':[target['id']]});call('capture.screenshot',{'path':str(out/'selected.png')})
    call('sketch.deleteSelection');changed=draft()
    assert params()==expected and session()['draftHistory']['index']==index+1
    call('history.undo');assert draft()==baseline
    call('history.redo');assert draft()==changed
    call('capture.screenshot',{'path':str(out/'deleted.png')})
    call('sketch.finish');authored=call('document.inspect')['authored'];path=out/'fit-spline-offset-deleted.alloy'
    call('document.save',{'path':str(path)});call('document.open',{'path':str(path),'discard':True});assert call('document.inspect')['authored']==authored
    call('capture.screenshot',{'path':str(out/'reopened.png')})
    gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated']
    (out/'receipt.json').write_text(json.dumps({'status':'pass','appSha256':hashlib.sha256(Path(a.app).read_bytes()).hexdigest(),'gpu':gpu,'source':source,'target':target,'sibling':sibling,'savedPath':str(path),'exactUndoRedoReopen':True,'dependentEndpointRefusal':True},indent=2)+'\n')
    (out/'transcript.json').write_text(json.dumps(ledger,indent=2)+'\n')
print('PASS: fit-spline Offset result Delete, sibling/source preservation, dependent refusal, history/reopen and GPU')
