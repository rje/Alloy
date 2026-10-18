#!/usr/bin/env python3
"""Break Offset association into an authored editable spline through the real app."""
import argparse, hashlib, json
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,ledger):
    def session():return call('app.state')['activeSession']
    def draft():return session()['draftSketch']
    def params():return draft()['parameters']
    def control():return next(e for e in params()['entities'] if e['id']==target['id'])
    def points():return {e['id']:e for e in params()['points']}
    def details(result):return next(e for e in result['sketchGeometry']['entities'] if e['id']==target['id'])
    call('sketch.begin',{'plane':'XY','name':'Independent Offset result'});call('sketch.lookAt')
    call('sketch.splineAdd',{'points':[{'uMm':0,'vMm':-20},{'uMm':8,'vMm':20},{'uMm':-6,'vMm':60},{'uMm':4,'vMm':100}]});source=params()['entities'][-1]
    call('sketch.splineOffset',{'sourceEntityId':source['id'],'distance':-5,'units':'mm'});target=params()['entities'][-1]
    # Legal connectors retain exact endpoint IDs across the conversion.
    for start,end in [(source['startPointId'],target['startPointId']),(source['endPointId'],target['endPointId'])]:
        x,y=points()[start],points()[end]
        call('sketch.line',{'x1':x['uMm'],'y1':x['vMm'],'x2':y['uMm'],'y2':y['vMm'],'units':'mm','startSnapTarget':{'kind':'point','pointId':start},'endSnapTarget':{'kind':'point','pointId':end}})
    before=draft();index=session()['draftHistory']['index'];endpoint_points={k:points()[k] for k in [target['startPointId'],target['endPointId']]}
    for invalid in [source['id'],'stale']:
        call('sketch.splineOffsetBreakLink',{'offsetEntityId':invalid},error='sketch_solve_failed')
        assert draft()==before and session()['draftHistory']['index']==index
    call('view.fit');call('view.zoom',{'factor':1.25});call('capture.screenshot',{'path':str(out/'associated.png')})
    call('sketch.selectionSet',{'entityIds':[target['id']]})
    conversion=call('sketch.splineOffsetBreakLink');converted=draft()
    assert session()['draftHistory']['index']==index+1
    assert control()['type']=='controlSpline' and control()['knotPolicy']=='clampedExplicit' and 'sourceEntityId' not in control()
    assert control()['startPointId']==target['startPointId'] and control()['endPointId']==target['endPointId']
    assert all(points()[k]==v for k,v in endpoint_points.items())
    assert next(e for e in params()['entities'] if e['id']==source['id'])==source
    assert [e for e in params()['entities'] if e['type']=='line']==[e for e in before['parameters']['entities'] if e['type']=='line']
    assert len(conversion['sketchGeometry']['profiles'])==1
    samples=details(conversion)['curveSamples'];assert conversion['conversion']['sampledMaximumDeviationMm']<=.001
    call('history.undo');assert draft()==before
    call('history.redo');assert draft()==converted
    call('capture.screenshot',{'path':str(out/'independent-control-cage.png')})
    # Independent authored geometry Delete removes the curve while preserving shared connector points.
    delete_before=draft();call('sketch.deleteSelection',{'entityIds':[target['id']]});assert not any(e['id']==target['id'] for e in params()['entities']);assert all(points()[k]==v for k,v in endpoint_points.items());call('history.undo');assert draft()==delete_before
    result=call('sketch.movePoint',{'pointId':source['pointIds'][1],'x':9,'y':20,'units':'mm'})
    assert details(result)['curveSamples']==samples
    sketch_id=draft()['id'];call('sketch.finish');authored=call('document.inspect')['authored'];path=out/'independent-offset.alloy'
    call('document.save',{'path':str(path)});call('document.open',{'path':str(path),'discard':True});assert call('document.inspect')['authored']==authored
    call('sketch.edit',{'sketchId':sketch_id});pole_id=control()['pointIds'][1];pole=points()[pole_id]
    result=call('sketch.movePoint',{'pointId':pole_id,'x':pole['uMm']+2,'y':pole['vMm'],'units':'mm'})
    assert details(result)['curveSamples']!=samples and control()['id']==target['id']
    assert all(points()[k]==v for k,v in endpoint_points.items())
    call('sketch.selectionSet',{'entityIds':[target['id']]});call('capture.screenshot',{'path':str(out/'reopened-edited-control.png')})
    gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated']
    (out/'receipt.json').write_text(json.dumps({'status':'pass','appSha256':hashlib.sha256(Path(a.app).read_bytes()).hexdigest(),'gpu':gpu,'source':source,'target':target,'conversion':conversion['conversion'],'conversionCallElapsedMs':next(r['elapsedMs'] for r in ledger if r['command']=='sketch.splineOffsetBreakLink' and r['response']['ok']),'savedPath':str(path),'sourceIndependent':True,'reopenedControlPoleEdited':True,'exactUndoRedoReopen':True},indent=2)+'\n')
    (out/'transcript.json').write_text(json.dumps(ledger,indent=2)+'\n')
print('PASS: Offset Break Link, retained endpoints/connectors, independent source/pole editing, history/reopen and GPU')
