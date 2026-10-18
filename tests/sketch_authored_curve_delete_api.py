#!/usr/bin/env python3
"""Real-entrypoint authored ellipse/conic/spline Delete and ownership gate."""
import argparse
import hashlib
import json
import math
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,ledger):
    def session():return call('app.state')['activeSession']
    def draft():return session()['draftSketch']
    def params():return draft()['parameters']
    def last():return params()['entities'][-1]
    def delete_and_restore(target,removed_entities,removed_points):
        before=session();baseline=before['draftSketch'];index=before['draftHistory']['index']
        call('sketch.selectionSet',{'entityIds':[target['id']]});call('capture.screenshot',{'path':str(out/(target['type']+'-selected.png'))})
        result=call('sketch.deleteSelection');after=session()
        assert after['draftHistory']['index']==index+1
        assert {e['id'] for e in params()['entities']}=={e['id'] for e in baseline['parameters']['entities']}-set(removed_entities)
        assert {q['id'] for q in params()['points']}=={q['id'] for q in baseline['parameters']['points']}-set(removed_points)
        retired=set(removed_entities+removed_points)
        for row in params()['constraints']+params()['dimensions']:
            assert not any(i in json.dumps(row) for i in retired)
        old_points={q['id']:q for q in baseline['parameters']['points']}
        assert all(q==old_points[q['id']] for q in params()['points'])
        changed=draft();call('history.undo');assert draft()==baseline
        call('history.redo');assert draft()==changed
        call('capture.screenshot',{'path':str(out/(target['type']+'-deleted.png'))})
        call('history.undo');assert draft()==baseline
        return result
    call('sketch.begin',{'plane':'XY','name':'Authored curve deletion'})
    call('sketch.addCircle',{'centerX':0,'centerY':0,'diameter':30,'units':'mm'});circle=last()
    call('sketch.splineAdd',{'points':[{'uMm':-35,'vMm':25},{'uMm':-10,'vMm':35},{'uMm':15,'vMm':25},{'uMm':35,'vMm':30}]});fit=last()
    # Retain a line's distinct endpoint and coincident source relation; the
    # conic below also reuses this fit point's exact authored identity.
    shared=fit['pointIds'][0]
    call('sketch.line',{'x1':-35,'y1':25,'x2':-40,'y2':15,'units':'mm','startSnapTarget':{'kind':'point','pointId':shared}});line=last()
    # Selected fit-point Delete remains a point-removal edit, not whole-curve Delete.
    baseline=draft();call('sketch.selectionSet',{'pointIds':[fit['pointIds'][1]]});call('sketch.deleteSelection')
    reduced=next(e for e in params()['entities'] if e['id']==fit['id']);assert len(reduced['pointIds'])==3
    call('history.undo');assert draft()==baseline
    call('sketch.controlSplineAdd',{'points':[{'uMm':-35,'vMm':-20},{'uMm':-10,'vMm':-35},{'uMm':15,'vMm':-20}]});control=last()
    call('sketch.conicAdd',{'start':{'uMm':-35,'vMm':25,'pointId':shared},'end':{'uMm':55,'vMm':0},'vertex':{'uMm':45,'vMm':45},'rho':.5});conic=last()
    call('sketch.centerPointEllipse',{'centerX':-50,'centerY':0,'firstX':-40,'firstY':0,'contourX':-50,'contourY':6,'units':'mm'});ellipse=last()
    call('sketch.dimension',{'kind':'linear','entityId':ellipse['firstAxisId'],'value':20,'units':'mm'})
    baseline=draft();ids={e['id'] for e in params()['entities']}
    # An independently selected ellipse axis refuses while its ellipse survives.
    call('sketch.deleteSelection',{'entityIds':[ellipse['firstAxisId']]},error='sketch_solve_failed');assert draft()==baseline
    delete_and_restore(fit,[fit['id']],fit['pointIds'][1:])
    assert next(e for e in params()['entities'] if e['id']==line['id'])==line
    delete_and_restore(control,[control['id']],control['pointIds'])
    delete_and_restore(conic,[conic['id']],conic['pointIds'][1:])
    axes=[e for e in params()['entities'] if e['id'] in [ellipse['firstAxisId'],ellipse['secondAxisId']]]
    ellipse_points=[ellipse['centerPointId']]+[e[k] for e in axes for k in ['startPointId','endPointId']]
    delete_and_restore(ellipse,[ellipse['id']]+[e['id'] for e in axes],ellipse_points)
    # Commit deletion, retain and extrude the unaffected circle profile.
    call('sketch.deleteSelection',{'entityIds':[fit['id'],control['id'],conic['id'],ellipse['id'],line['id']]})
    assert params()['entities']==[circle]
    final=draft();call('sketch.finish');doc=call('document.inspect');sketch_id=final['id']
    profile=doc['evaluation']['sketches'][0]['profiles'][0]['profileId']
    call('solid.extrude',{'sketchId':sketch_id,'profileId':profile,'distance':8,'units':'mm'})
    solid=call('document.inspect');body=solid['evaluation']['bodies'][0]
    assert body['valid'] and body['closed'] and math.isclose(body['volumeMm3'],math.pi*15**2*8,rel_tol=1e-6)
    path=out/'authored-curves-deleted-solid.alloy';call('document.save',{'path':str(path)})
    call('document.open',{'path':str(path),'discard':True});reopened=call('document.inspect');assert reopened['authored']==solid['authored']
    assert reopened['evaluation']['bodies'][0]['bodyId']==body['bodyId']
    call('view.home');call('view.fit')
    call('capture.screenshot',{'path':str(out/'reopened-solid.png')})
    # Retained constructor ownership blocks deletion atomically.
    call('sketch.begin',{'plane':'XY','name':'Owner refusal'});call('sketch.centerPointSlot',{'centerX':55,'centerY':0,'endX':75,'endY':0,'width':10,'units':'mm'})
    side=last();call('sketch.selectionSet',{'entityIds':[side['id']]});before=session()
    call('sketch.deleteSelection',error='sketch_solve_failed');after=session()
    for key in ['draftSketch','draftHistory','selection']:assert before[key]==after[key]
    call('capture.screenshot',{'path':str(out/'owner-refusal.png')});call('sketch.cancel')
    gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated']
    (out/'receipt.json').write_text(json.dumps({'status':'pass','appSha256':hashlib.sha256(Path(a.app).read_bytes()).hexdigest(),'gpu':gpu,'body':body,'sketchId':sketch_id,'fitSplineSharedPoint':shared,'refusedSlotMemberId':side['id']},indent=2)+'\n')
    (out/'transcript.json').write_text(json.dumps(ledger,indent=2)+'\n')
print('PASS: authored curves, shared point, fit-point route, owner refusal, history/reopen and solid')
