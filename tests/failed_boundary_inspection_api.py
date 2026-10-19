#!/usr/bin/env python3
"""Changed planar-inset boundary inspection through the real Alloy endpoint."""
import argparse, hashlib, json
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--fixture',required=True);p.add_argument('--output',required=True)
a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,ledger):
    call('document.open',{'path':str(Path(a.fixture).resolve())})
    base=call('document.inspect');features=base['authored']['features']
    sketch=next(f for f in features if 'loopOffsets' in f.get('parameters',{}));support=sketch['parameters']['support']
    combine=next(f for f in features if f['type']=='boolean');tool=next(f for f in features if f['name']=='Split tool')
    assert tool['parameters']['xMm']==49 and sketch['parameters']['loopOffsets'][0]['kind']=='planarInsetV1'
    profile=next(s for s in base['evaluation']['sketches'] if s['sketchId']==sketch['id'])['profiles'][0]['profileId']
    call('selection.set',{'bodyId':support['bodyId']});initial=call('app.state')
    args={'featureId':combine['id'],'targetBodyId':support['bodyId'],'toolBodyIds':[tool['bodyId']],'operation':'cut','keepTools':True}
    call('solid.combine',args,error='evaluation_failed');edit_failure=ledger[-1]['response']
    repair=call('history.failedEditInspect');diagnostic=repair['diagnostics']
    assert diagnostic['code']=='ambiguous_inset_lineage' and diagnostic['message']=='Inset source boundary does not have complete retained edges'
    assert diagnostic==edit_failure['error']['diagnostics']
    assert diagnostic['featureId']==sketch['id'] and diagnostic['featureType']=='sketch'
    assert all(diagnostic[k]==support[k] for k in ('bodyId','producerFeatureId','faceId'))
    assert repair['failedCommand']=={'command':'solid.combine','arguments':args} and repair['baseDocumentRevision']==base['revision']
    assert repair['cachedOwnerIds']==[sketch['parameters']['loopOffsets'][0]['id']] and repair['cachedProfileIds']==[profile]
    assert repair['candidateAuthored']['features'][-1]==sketch and sketch['id'] not in repair['validPrefix']['featureIds']
    assert call('command.status',{'session':repair['id']})['failedEditRepair']==repair
    assert call('document.inspect')==base
    call('sketch.selectionSet',{'profileIds':[profile]});selected=call('app.state')['activeSession']
    call('sketch.deleteSelection',error='ambiguous_inset_lineage');refusal=ledger[-1]['response']
    recovery='Cancel Failed Edit, reopen the sketch, then reselect its current loop Offset result and retry Delete.'
    assert refusal['error']['recovery']==recovery
    assert refusal['error']['message']==diagnostic['message']+'\n'+recovery
    assert refusal['error']['diagnostics']=={'code':'ambiguous_inset_lineage','message':diagnostic['message']}
    assert 'recovery' not in diagnostic and repair['diagnostics']==edit_failure['error']['diagnostics']
    assert call('app.state')['activeSession']==selected and call('document.inspect')==base
    call('command.commit',{'session':repair['id']},error='repair_read_only')
    assert call('app.state')['activeSession']==selected and call('document.inspect')==base
    call('capture.screenshot',{'path':str(out/'boundary-selected.png')})
    call('command.cancel',{'session':repair['id']});assert call('document.inspect')==base
    state=call('app.state');assert state['activeSession']=={} and state['selection']==initial['selection']
    assert state['viewport']['navigation']==initial['viewport']['navigation']
    # A second committed edit invalidates the old candidate ID.
    call('solid.combine',args,error='evaluation_failed');old=call('history.failedEditInspect')
    call('feature.visibility',{'featureId':sketch['id'],'visible':False})
    assert call('history.failedEditInspect')=={} and call('document.inspect')['revision']>base['revision']
    call('command.commit',{'session':old['id']},error='invalid_session');call('history.undo')
    restored=call('document.inspect');assert restored['authored']==base['authored'] and restored['evaluation']['bodies']==base['evaluation']['bodies']
    # Save/reopen only the committed valid model; ordinary Delete still succeeds.
    call('document.save',{'path':str(out/'accepted-only.alloy')});assert json.loads((out/'accepted-only.alloy').read_text())==base['authored']
    call('document.open',{'path':str(out/'accepted-only.alloy')});reopened=call('document.inspect')
    assert reopened['authored']==base['authored'] and reopened['evaluation']['bodies']==base['evaluation']['bodies']
    call('sketch.edit',{'sketchId':sketch['id']});call('sketch.selectionSet',{'profileIds':[profile]})
    call('sketch.deleteSelection');assert 'loopOffsets' not in call('app.state')['activeSession']['draftSketch']['parameters']
    call('sketch.cancel');assert call('document.inspect')==reopened
    gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated']
    (out/'receipt.json').write_text(json.dumps({'status':'pass','appSha256':hashlib.sha256(Path(a.app).read_bytes()).hexdigest(),'input':'Local API; physical input unqualified','repair':repair,'refusal':refusal,'base':base,'gpu':gpu},indent=2)+'\n')
    (out/'transcript.json').write_text(json.dumps(ledger,indent=2)+'\n')
print('PASS: contextual boundary inspection, exact cached Delete refusal/Cancel, invalidation, accepted-only save/reopen and valid Delete')
