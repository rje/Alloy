#!/usr/bin/env python3
"""Existing-dimension conversion through the real application entrypoint."""
import argparse,json,math
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,log):
    def session():return call('app.state')['activeSession']
    def draft():return session()['draftSketch']
    call('sketch.begin',{'plane':'XY'});call('sketch.rectangle',{'x1':0,'y1':0,'x2':40,'y2':25,'units':'mm'})
    edge=draft()['parameters']['entities'][0]
    call('sketch.dimension',{'kind':'horizontal','entityId':edge['id'],'value':40,'annotation':{'uMm':20,'vMm':-8}})
    call('sketch.tool',{'tool':'select'});before=draft();dimension=before['parameters']['dimensions'][0];did=dimension['id']
    call('sketch.dimensionDriving',{'dimensionId':did,'driving':False});driven=draft()
    expected={k:v for k,v in dimension.items() if k!='valueMm'};expected['driving']=False
    assert driven['parameters']['dimensions'][0]==expected
    assert driven['parameters']['points']==before['parameters']['points'] and driven['parameters']['entities']==before['parameters']['entities']
    call('history.undo');assert draft()==before
    call('history.redo');assert draft()==driven
    call('sketch.dimensionDriving',{'dimensionId':did,'driving':True});assert draft()==before
    call('sketch.dimensionDriving',{'dimensionId':did,'driving':False})
    call('sketch.dimension',{'kind':'horizontal','entityId':edge['id'],'value':50});assert len(draft()['parameters']['dimensions'])==2
    values=session()['solverDiagnostics']['dimensionValues'];assert math.isclose(next(d['valueMm'] for d in values if d['id']==did),50,abs_tol=1e-6)
    retained=draft();journal=session()['draftHistory']
    call('sketch.dimensionDriving',{'dimensionId':did,'driving':True},error='sketch_solve_failed');assert draft()==retained and session()['draftHistory']==journal
    call('sketch.dimensionUpdate',{'dimensionId':did,'value':70},error='sketch_solve_failed');assert draft()==retained
    call('sketch.dimensionEdit',{'dimensionId':did});assert session()['pendingDimension']['driving']==False
    call('sketch.dimensionCancel');call('view.home');call('view.orientation',{'name':'top'});call('view.zoom',{'factor':4});call('capture.screenshot',{'path':str(out/'measured-50.png')})
    sketch_id=retained['id'];driver_id=retained['parameters']['dimensions'][1]['id']
    call('sketch.finish');sketched=call('document.inspect');profile=sketched['evaluation']['sketches'][0]['profiles'][0]['profileId']
    call('solid.extrude',{'sketchId':sketch_id,'profileId':profile,'distance':5,'units':'mm'})
    extruded=call('document.inspect');assert math.isclose(extruded['evaluation']['bodies'][0]['volumeMm3'],6250,abs_tol=1e-5)
    assert len(extruded['evaluation']['bodies'])==1
    body=extruded['evaluation']['bodies'][0];assert body['valid'] and body['closed'] and body['topology']['solids']==1
    assert all(math.isclose(a,b,abs_tol=1e-6) for a,b in zip(body['boundsMm']['size'],[50,25,5]))
    call('history.undo');assert call('document.inspect')['authored']==sketched['authored']
    call('history.redo');assert call('document.inspect')['authored']==extruded['authored']
    call('sketch.edit',{'sketchId':sketch_id});call('sketch.dimensionUpdate',{'dimensionId':driver_id,'value':60})
    values=session()['solverDiagnostics']['dimensionValues'];assert math.isclose(next(d['valueMm'] for d in values if d['id']==did),60,abs_tol=1e-6)
    assert draft()['parameters']['dimensions'][0]==expected
    call('sketch.finish');edited=call('document.inspect');assert math.isclose(edited['evaluation']['bodies'][0]['volumeMm3'],7500,abs_tol=1e-5)
    assert edited['evaluation']['sketches'][0]['profiles'][0]['profileId']==profile
    body=edited['evaluation']['bodies'][0];assert body['valid'] and body['closed'] and body['topology']['solids']==1
    assert all(math.isclose(a,b,abs_tol=1e-6) for a,b in zip(body['boundsMm']['size'],[60,25,5]))
    call('history.undo');assert call('document.inspect')['authored']==extruded['authored']
    call('history.redo');assert call('document.inspect')['authored']==edited['authored']
    call('document.save',{'path':str(out/'converted.alloy')});call('document.open',{'path':str(out/'converted.alloy')});reopened=call('document.inspect');assert reopened['authored']==edited['authored']
    assert math.isclose(reopened['evaluation']['bodies'][0]['volumeMm3'],7500,abs_tol=1e-5)
    (out/'geometry.json').write_text(json.dumps({'extruded':extruded,'edited':edited,'reopened':reopened},indent=2))
    (out/'state.json').write_text(json.dumps(call('app.state'),indent=2));(out/'transcript.json').write_text(json.dumps(log,indent=2));(out/'report.json').write_text(json.dumps({'passed':True,'calls':len(log),'dimensionId':did},indent=2))
