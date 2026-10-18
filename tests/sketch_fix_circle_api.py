#!/usr/bin/env python3
"""Real-entrypoint authored circle Fix/UnFix: center, radius, history and persistence."""
import argparse
import json
import time
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,ledger):
    def session(): return call('app.state')['activeSession']
    def draft(): return session()['draftSketch']
    def role(e): return next(r['role'] for r in session()['solverDiagnostics']['entityRoles'] if r['entityId']==e['id'])
    call('sketch.begin',{'plane':'XY','units':'mm','name':'Circle Fix UnFix'})
    circles=[]
    for x,y,d in [(-20,10,20),(20,-10,14)]:
        call('sketch.addCircle',{'centerX':x,'centerY':y,'diameter':d,'units':'mm'});circles.append(draft()['parameters']['entities'][-1])
    c,u=circles
    call('sketch.dimension',{'kind':'radius','entityId':c['id'],'value':10,'units':'mm'});dim=draft()['parameters']['dimensions'][-1]['id']
    call('sketch.tool',{'tool':'select'});call('sketch.selectionSet');baseline=draft()
    relations=[]
    for e in circles:
        call('sketch.fixToggle',{'entityIds':[e['id']]});relations.append(draft()['parameters']['constraints'][-1]);assert role(e)=='fixed'
    fixed=draft()
    assert all(r['type']=='fixedCircle' and r['pointIds']==[e['centerPointId']] and r['radiusMm']==e['radiusMm'] for r,e in zip(relations,circles))
    call('sketch.dimensionUpdate',{'dimensionId':dim,'value':12,'units':'mm'},error='sketch_solve_failed');assert draft()==fixed
    call('sketch.dimension',{'kind':'radius','entityId':u['id'],'value':9,'units':'mm'},error='sketch_solve_failed');assert draft()==fixed
    # Center edits and stale/mixed targets do not change authored geometry/history.
    for e in circles:
        call('sketch.movePoint',{'pointId':e['centerPointId'],'x':50,'y':40,'units':'mm'});assert draft()==fixed
    for args in [{'entityIds':['00000000-0000-0000-0000-000000000000']},{'entityIds':[c['id']],'pointIds':[u['centerPointId']]}]:
        before=session()['draftHistory'];call('sketch.fixToggle',args,error='sketch_solve_failed');assert draft()==fixed and session()['draftHistory']==before
    call('sketch.constraint',{'type':'fixed','entityIds':[c['id']]});unfixed=draft();assert role(c)!='fixed' and relations[1] in unfixed['parameters']['constraints']
    call('history.undo');assert draft()==fixed
    call('history.redo');assert draft()==unfixed
    call('sketch.dimensionUpdate',{'dimensionId':dim,'value':12,'units':'mm'});assert draft()['parameters']['entities'][0]['radiusMm']==12
    assert draft()['parameters']['dimensions'][0]['id']==dim
    call('sketch.fixToggle',{'entityIds':[c['id']]});final=draft();call('sketch.lookAt');time.sleep(0.25);call('capture.screenshot',{'path':str(out/'fixed.png')});state=call('app.state')
    assert not {'fixed','fixedLine','fixedCircle'} & {g['kind'] for g in state['viewport']['gpu']['sketchConstraintGlyphs']}
    # Exact stable-ID deletion removes only one circle's ownership.
    final_rel=final['parameters']['constraints'][-1]
    call('sketch.deleteSelection',{'constraintIds':[final_rel['id']]});assert final_rel not in draft()['parameters']['constraints'] and relations[1] in draft()['parameters']['constraints']
    call('history.undo');assert draft()==final
    call('sketch.finish');authored=call('document.inspect')['authored'];call('document.save',{'path':str(out/'fixed-circles.alloy')});call('document.open',{'path':str(out/'fixed-circles.alloy')});assert call('document.inspect')['authored']==authored
    call('sketch.edit',{'sketchId':final['id']});assert draft()==final and all(role(e)=='fixed' for e in circles)
    time.sleep(0.25);call('capture.screenshot',{'path':str(out/'reopened.png')});reopened=call('app.state');call('sketch.cancel')
    (out/'receipt.json').write_text(json.dumps({'result':'passed','fixedState':state,'reopenedState':reopened,'transcript':ledger},indent=2)+'\n')
print('PASS: fixed circle center/radius, dimension conflict/recovery, shared toggle, atomic refusal, history/reopen and GPU captures')
