#!/usr/bin/env python3
"""One mixed actual-app entity-role/status fixture with history and source refresh."""
import argparse
import json
from pathlib import Path
from app_harness import application

p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,ledger):
    def session():return call('app.state')['activeSession']
    def draft():return session()['draftSketch']
    def last():return draft()['parameters']['entities'][-1]
    def roles():return {r['entityId']:r['role'] for r in session()['solverDiagnostics']['entityRoles']}
    call('sketch.begin',{'plane':'XY','name':'Projection source'})
    call('sketch.rectangle',{'x1':5,'y1':35,'x2':35,'y2':45,'units':'mm'})
    source=draft();edges=source['parameters']['entities'];source_id=source['id']
    call('sketch.constraint',{'type':'fixed','pointIds':[edges[0]['startPointId']]})
    call('sketch.dimension',{'kind':'horizontal','entityId':edges[0]['id'],'value':30,'units':'mm'})
    width=draft()['parameters']['dimensions'][-1]['id']
    call('sketch.dimension',{'kind':'vertical','entityId':edges[1]['id'],'value':10,'units':'mm'})
    call('sketch.finish')
    profile=call('document.inspect')['evaluation']['sketches'][0]['profiles'][0]['profileId']
    call('solid.extrude',{'sketchId':source_id,'profileId':profile,'distance':4,'units':'mm'})
    body=call('document.inspect')['evaluation']['bodies'][0]
    face=next(f for f in body['supportFaces'] if f['role']=='extrude.base')
    call('sketch.begin',{'support':{k:face[k] for k in ('producerFeatureId','bodyId','faceId')},'name':'Mixed entity roles'})
    call('sketch.line',{'x1':-35,'y1':20,'x2':-10,'y2':25,'units':'mm'});ordinary=last()
    call('sketch.line',{'x1':-35,'y1':8,'x2':-10,'y2':8,'units':'mm'});construction=last()
    call('sketch.construction',{'entityIds':[construction['id']],'construction':True})
    call('sketch.line',{'x1':-35,'y1':-8,'x2':-10,'y2':-8,'units':'mm'});fixed=last()
    for key in ('startPointId','endPointId'):call('sketch.constraint',{'type':'fixed','pointIds':[fixed[key]]})
    fixed_relation=draft()['parameters']['constraints'][-1]['id']
    call('sketch.addCircle',{'centerX':20,'centerY':10,'diameter':12,'units':'mm'});dimensioned=last()
    # Position from the fixed curve through dimensions; no Fix on this circle.
    call('sketch.dimension',{'kind':'horizontal','pointIds':[fixed['startPointId'],dimensioned['centerPointId']],'value':55,'units':'mm','annotation':{'uMm':-7,'vMm':-20}})
    call('sketch.dimension',{'kind':'vertical','pointIds':[fixed['startPointId'],dimensioned['centerPointId']],'value':18,'units':'mm','annotation':{'uMm':-45,'vMm':1}})
    call('sketch.dimension',{'kind':'diameter','entityId':dimensioned['id'],'value':12,'units':'mm'})
    call('sketch.projectBegin');call('sketch.projectSelect',{'bodyId':body['bodyId'],'edgeId':face['edges'][0]['edgeId']});call('sketch.projectCommit')
    projections=[p['id'] for p in draft()['parameters']['projections']];projection=projections[0];mixed=draft();mixed_id=mixed['id']
    expected={ordinary['id']:'unconstrainedOrdinary',construction['id']:'unconstrainedConstruction',fixed['id']:'fixed',dimensioned['id']:'fullyConstrained',**{p:'projected' for p in projections}}
    assert roles()==expected,(roles(),expected)
    assert session()['solverDiagnostics']['dof']==8
    assert {r['entityId'] for r in session()['solverDiagnostics']['underconstrainedEntities']}=={ordinary['id'],construction['id']}
    before=session();call('sketch.findUnderconstrained');call('sketch.compute')
    call('sketch.underconstrainedSelect',{'entityId':ordinary['id']})
    assert session()['selection']['entityIds']==[ordinary['id']]
    assert session()['draftSketch']==before['draftSketch'] and session()['draftHistory']==before['draftHistory']
    call('sketch.underconstrainedSelect',{'entityId':fixed['id']},error='stale_entity')
    call('sketch.constraintSelect',{'constraintId':fixed_relation});call('sketch.deleteSelection')
    assert roles()[fixed['id']]=='unconstrainedOrdinary' and session()['solverDiagnostics']['dof']==10
    assert fixed['id'] in {r['entityId'] for r in session()['solverDiagnostics']['underconstrainedEntities']}
    call('history.undo');assert draft()==mixed and roles()==expected
    call('history.redo');assert roles()[fixed['id']]=='unconstrainedOrdinary'
    call('sketch.constraint',{'type':'fixed','pointIds':[fixed['endPointId']]});assert roles()==expected
    fixed_relation=draft()['parameters']['constraints'][-1]['id']
    reapplied=draft();call('history.undo');assert roles()[fixed['id']]=='unconstrainedOrdinary'
    call('history.redo');assert draft()==reapplied and roles()==expected
    call('sketch.finish')
    call('document.save',{'path':str(out/'mixed-roles.alloy')})
    baseline=call('document.inspect');native_fixture=baseline['authored']
    # Edit the retained projection's real source and compare coordinates and stable role IDs.
    original=baseline['evaluation']['sketches'][-1]['projectedLines']
    call('sketch.edit',{'sketchId':source_id});call('sketch.dimensionUpdate',{'dimensionId':width,'value':42,'units':'mm'});call('sketch.finish')
    edited=call('document.inspect');updated=edited['evaluation']['sketches'][-1]['projectedLines']
    assert original!=updated and [r['id'] for r in original]==[r['id'] for r in updated]
    call('history.undo');assert call('document.inspect')['authored']==baseline['authored']
    call('history.redo');assert call('document.inspect')['authored']==edited['authored']
    call('document.save',{'path':str(out/'mixed-roles-source-edited.alloy')})
    call('document.open',{'path':str(out/'mixed-roles-source-edited.alloy'),'discard':True})
    assert call('document.inspect')['authored']==edited['authored']
    call('sketch.edit',{'sketchId':mixed_id});assert roles()==expected
    call('sketch.findUnderconstrained');call('sketch.lookAt')
    full_state=call('app.state');assert full_state['viewport']['gpu']['hardwareAccelerated']
    call('capture.screenshot',{'path':str(out/'mixed-roles.png')})
    call('sketch.underconstrainedSelect',{'entityId':ordinary['id']});selected_state=call('app.state')
    call('capture.screenshot',{'path':str(out/'underconstrained-selected.png')})
    call('sketch.finish')
    (out/'receipt.json').write_text(json.dumps({'result':'passed','mixedSketchId':mixed_id,'ids':{'ordinary':ordinary['id'],'construction':construction['id'],'fixed':fixed['id'],'dimensioned':dimensioned['id'],'projection':projection,'fixedRelation':fixed_relation},'expectedRoles':expected,'fullState':full_state,'selectedState':selected_state,'originalProjection':original,'editedProjection':updated,'baselineAuthored':native_fixture,'transcript':ledger},indent=2)+'\n')
print('PASS: mixed per-entity roles, live list/selection, relation history, linked source edit and reopen')
