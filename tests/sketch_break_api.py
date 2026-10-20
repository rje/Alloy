#!/usr/bin/env python3
"""Bounded analytic Break through the real application's local control server."""
import argparse,json,math
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,log):
    assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
    session=call('sketch.begin',{'plane':'XY','name':'Circle Break secant'})
    def draft():return call('command.preview',{'session':session['id']})['authoredDraft']
    call('sketch.addCircle',{'centerX':0,'centerY':0,'diameter':20,'units':'mm'})
    call('sketch.line',{'x1':-15,'y1':0,'x2':15,'y2':0,'units':'mm'})
    circle=draft()['parameters']['entities'][0]
    call('sketch.dimension',{'kind':'diameter','entityId':circle['id'],'value':20,'units':'mm'})
    before=draft();history=call('app.state')['activeSession']['draftHistory']
    inputs={'entityId':circle['id'],'uMm':0,'vMm':10}
    preview=call('sketch.breakPreview',inputs);assert preview['authoredChange'] is False
    assert draft()==before and call('app.state')['activeSession']['draftHistory']==history
    assert len(preview['breakPreview']['diagnostics']['breakPoints'])==2
    call('sketch.lookAt');call('capture.screenshot',{'path':str(out/'break-preview.png')})
    result=call('sketch.break',inputs);after=draft();arcs=[e for e in after['parameters']['entities'] if e['type']=='arc']
    assert len(arcs)==2 and math.isclose(sum(e['sweepAngleDeg'] for e in arcs),360,abs_tol=1e-8)
    assert {e['radiusMm'] for e in arcs}=={10} and circle['id'] in {e['id'] for e in arcs}
    assert result['breakDiagnostics']['removedDimensionIds']==[before['parameters']['dimensions'][0]['id']]
    assert after['parameters']['entities'][-1]==before['parameters']['entities'][-1]
    refused_state=call('app.state')['activeSession']
    call('sketch.break',inputs,error='sketch_break_unavailable')
    assert draft()==after and call('app.state')['activeSession']['draftHistory']==refused_state['draftHistory']
    call('history.undo');assert draft()==before
    call('history.redo');assert draft()==after
    boundary=before['parameters']['entities'][-1]
    call('sketch.movePoint',{'pointId':boundary['startPointId'],'x':-15,'y':2,'units':'mm'})
    assert [e for e in draft()['parameters']['entities'] if e['type']=='arc']==arcs
    assert draft()['parameters']['constraints']==after['parameters']['constraints']
    call('history.undo');assert draft()==after
    call('sketch.finish');committed=call('document.inspect');profiles=committed['evaluation']['sketches'][0]['profiles']
    assert math.isclose(sum(p['areaMm2'] for p in profiles),math.pi*100,abs_tol=1e-5),profiles
    call('history.undo');assert call('document.inspect')['features']==0
    call('history.redo');assert call('document.inspect')['authored']==committed['authored']
    saved=out/'circle-break.alloy';call('document.save',{'path':str(saved)});call('document.open',{'path':str(saved)})
    assert call('document.inspect')['authored']==committed['authored']
    call('capture.screenshot',{'path':str(out/'break-result.png')})
    (out/'report.json').write_text(json.dumps({'passed':True,'before':before,'after':after,'diagnostics':result['breakDiagnostics'],'profiles':profiles},indent=2));(out/'transcript.json').write_text(json.dumps(log,indent=2))
