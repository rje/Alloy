#!/usr/bin/env python3
"""Transient support-loss inspection through the real Alloy local endpoint."""
import argparse, hashlib, json
from pathlib import Path
from app_harness import application

p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--fixtures',required=True);p.add_argument('--output',required=True)
a=p.parse_args();out=Path(a.output).resolve();fixtures=Path(a.fixtures).resolve();ids=json.loads((fixtures/'fixtures.json').read_text())['failure']
with application(a.app,out) as (call,ledger):
    call('document.open',{'path':str(fixtures/'support-failure.alloy')})
    call('selection.set',{'bodyId':ids['targetBodyId']})
    base=call('document.inspect');initial=call('app.state');profile=base['evaluation']['sketches'][-1]['profiles'][0]['profileId']
    sessions=[]
    for tool in ('splitBodyId','deleteBodyId'):
        args={'featureId':ids['combineFeatureId'],'targetBodyId':ids['targetBodyId'],'toolBodyIds':[ids[tool]],'operation':'cut','keepTools':True}
        call('solid.combine',args,error='evaluation_failed')
        repair=call('history.failedEditInspect');sessions.append(repair)
        assert repair['status']=='active' and repair['baseDocumentRevision']==base['revision']
        assert repair['failedCommand']=={'command':'solid.combine','arguments':args}
        assert repair['diagnostics']['code']=='support_unavailable' and repair['diagnostics']['featureId']==ids['sketchId']
        assert repair['diagnostics']['faceId']==ids['support']['faceId']
        assert repair['cachedOwnerIds']==[base['authored']['features'][-1]['parameters']['loopOffsets'][0]['id']]
        assert profile in repair['cachedProfileIds'] and ids['sketchId'] not in repair['validPrefix']['featureIds']
        assert repair['presentation']=='last_accepted_sketch_cache' and not repair['candidateGeometryEvaluated']
        assert call('document.inspect')==base
        call('sketch.edit',{'sketchId':ids['sketchId']})
        call('sketch.selectionSet',{'profileIds':[profile]})
        selected=call('app.state')['activeSession'];assert selected['cachedGeometry'] and selected['mode']=='failedEditRepair'
        call('sketch.deleteSelection',error='support_unavailable')
        delete_error=ledger[-1]['response']['error']
        recovery='Cancel Failed Edit, reopen the sketch, then reselect its current loop Offset result and retry Delete.'
        assert delete_error['recovery']==recovery
        assert delete_error['message']==repair['diagnostics']['message']+'\n'+recovery
        assert delete_error['diagnostics']['code']=='support_unavailable' and delete_error['diagnostics']['message']==repair['diagnostics']['message']
        assert 'recovery' not in repair['diagnostics']
        assert call('app.state')['activeSession']==selected and call('document.inspect')==base
        call('sketch.finish',error='repair_read_only');call('command.commit',{'session':repair['id']},error='repair_read_only')
        assert call('app.state')['activeSession']==selected and call('document.inspect')==base
        call('capture.screenshot',{'path':str(out/(tool+'-cached.png'))})
        call('command.cancel',{'session':repair['id']})
        assert call('document.inspect')==base and call('history.failedEditInspect')=={}
        state=call('app.state');assert state['activeSession']=={} and state['selection']==initial['selection']
        assert state['viewport']['navigation']==initial['viewport']['navigation']
    # Revision changes discard the session; its opaque ID cannot be committed.
    call('solid.combine',args,error='evaluation_failed');repair=call('history.failedEditInspect')
    call('feature.visibility',{'featureId':ids['sketchId'],'visible':False})
    changed=call('document.inspect');assert changed['revision']>base['revision'] and call('history.failedEditInspect')=={}
    call('command.commit',{'session':repair['id']},error='invalid_session')
    call('history.undo');restored=call('document.inspect');assert restored['authored']==base['authored'] and restored['evaluation']['bodies']==base['evaluation']['bodies']
    call('document.save',{'path':str(out/'accepted-only.alloy')});saved=json.loads((out/'accepted-only.alloy').read_text())
    assert saved==base['authored']
    call('document.open',{'path':str(out/'accepted-only.alloy')});reopened=call('document.inspect');assert reopened['authored']==base['authored']
    assert reopened['evaluation']['bodies']==base['evaluation']['bodies'] and call('history.failedEditInspect')=={}
    gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated']
    (out/'receipt.json').write_text(json.dumps({'status':'pass','appSha256':hashlib.sha256(Path(a.app).read_bytes()).hexdigest(),'input':'Local API; physical native input unqualified','sessions':sessions,'base':base,'gpu':gpu},indent=2)+'\n')
    (out/'transcript.json').write_text(json.dumps(ledger,indent=2)+'\n')
print('PASS: split/delete failed sessions, exact cache/refusal/cancel, revision invalidation, accepted-only save/reopen')
