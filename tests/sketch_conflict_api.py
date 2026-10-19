#!/usr/bin/env python3
"""Certified same-line orientation conflict and shared-command repair lifecycle."""
import argparse
import json
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,ledger):
    def session(): return call('app.state')['activeSession']
    def draft(): return session()['draftSketch']
    call('sketch.begin',{'plane':'XY','units':'mm','name':'Orientation conflict repair'})
    call('sketch.line',{'x1':0,'y1':0,'x2':40,'y2':0,'units':'mm'})
    line=draft()['parameters']['entities'][-1];sid=draft()['id']
    call('sketch.constraint',{'type':'fixed','pointIds':[line['startPointId']]})
    call('sketch.dimension',{'kind':'linear','entityId':line['id'],'value':40,'units':'mm'})
    dimension=draft()['parameters']['dimensions'][-1]
    call('sketch.constraint',{'type':'horizontal','entityIds':[line['id']]})
    existing=draft()['parameters']['constraints'][-1]
    call('sketch.tool',{'tool':'select'});call('sketch.selectionSet',{'entityIds':[line['id']]});call('sketch.lookAt')
    baseline=session();state=call('app.state');before_revision=state['document']['revision']
    call('sketch.constraint',{'type':'vertical','entityIds':[line['id']]},error='constraint_conflict')
    failed=session();d=failed['lastSolveFailure']
    assert failed['draftSketch']==baseline['draftSketch'] and failed['draftHistory']==baseline['draftHistory']
    assert failed['solverDiagnostics']==baseline['solverDiagnostics']
    assert call('app.state')['document']['revision']==before_revision
    assert d['accepted'] is False and d['attemptedRelation']['type']=='vertical' and d['attemptedRelationAuthored'] is False
    assert d['conflictingRelations']==[existing] and d['conflictingEntityIds']==[line['id']]
    assert d['supportingDimensions']==[dimension] and d['postFailureSolver']==baseline['solverDiagnostics']
    assert d['attemptedRelation']['id'] not in [c['id'] for c in draft()['parameters']['constraints']]
    call('capture.screenshot',{'path':str(out/'conflict.png')})
    call('sketch.conflictSelect');assert session()['selection']['constraintIds']==[existing['id']]
    call('capture.screenshot',{'path':str(out/'selected-existing.png')})
    call('sketch.deleteSelection');deleted=draft()
    assert existing not in deleted['parameters']['constraints']
    call('history.undo');assert draft()==baseline['draftSketch']
    call('history.redo');assert draft()==deleted
    call('sketch.constraint',{'type':'vertical','entityIds':[line['id']]});vertical=draft();relation=vertical['parameters']['constraints'][-1]
    def check_length(value):
        points={v['id']:v for v in draft()['parameters']['points']};start=points[line['startPointId']];end=points[line['endPointId']]
        assert abs(start['uMm'])<1e-6 and abs(start['vMm'])<1e-6
        assert abs(end['uMm'])<1e-6 and abs(end['vMm']-value)<1e-6,(end,value)
    check_length(40)
    call('history.undo');assert draft()==deleted
    call('history.redo');assert draft()==vertical
    call('sketch.dimensionUpdate',{'dimensionId':dimension['id'],'value':50,'units':'mm'});check_length(50);fifty=draft()
    call('history.undo');assert draft()==vertical
    call('history.redo');assert draft()==fifty
    call('sketch.lookAt');call('capture.screenshot',{'path':str(out/'repaired-50mm.png')})
    call('sketch.finish');authored=call('document.inspect')['authored'];path=out/'orientation-repair.alloy'
    call('document.save',{'path':str(path)});call('document.open',{'path':str(path)});assert call('document.inspect')['authored']==authored
    call('sketch.edit',{'sketchId':sid});assert draft()==fifty
    assert relation in draft()['parameters']['constraints'] and existing not in draft()['parameters']['constraints']
    call('sketch.dimensionUpdate',{'dimensionId':dimension['id'],'value':60,'units':'mm'});check_length(60)
    call('sketch.lookAt');call('capture.screenshot',{'path':str(out/'reopened-source-edit-60mm.png')})
    state=call('app.state');call('sketch.finish');call('document.save',{'path':str(out/'orientation-repair-edited.alloy')})
    (out/'receipt.json').write_text(json.dumps({'diagnostic':d,'line':line,'existing':existing,'dimension':dimension,'finalState':state,'ledger':ledger},indent=2))
print('PASS: certified API conflict, unchanged state/history, selection/Delete repair, length edit, undo/redo and exact authored reopen')
