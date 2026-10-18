#!/usr/bin/env python3
"""Historical Delete preflight: committed profile lineage and atomic refusal."""
import argparse, hashlib, json
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);p.add_argument('--fixture',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,ledger):
    def session():return call('app.state')['activeSession']
    def draft():return session()['draftSketch']
    def inspect():return call('document.inspect')
    call('document.open',{'path':str(Path(a.fixture).resolve()),'discard':True})
    base=inspect();sketch=next(f for f in base['authored']['features'] if f['type']=='sketch');extrude=next(f for f in base['authored']['features'] if f['type']=='extrude')
    call('sketch.edit',{'sketchId':sketch['id']})
    call('sketch.addCircle',{'centerX':35,'centerY':0,'diameter':6,'units':'mm'});unrelated=draft()['parameters']['entities'][-1]
    call('sketch.finish');call('document.save',{'path':str(out/'native-before.alloy')})
    baseline=inspect();call('sketch.edit',{'sketchId':sketch['id']});call('sketch.lookAt');call('view.fit')
    target=sketch['parameters']['entities'][-1]['id']
    call('sketch.selectionSet',{'entityIds':[target]});before=session()
    call('sketch.deleteSelection',error='downstream_profile_in_use');response=ledger[-1]['response'];after=session()
    assert before==after,(before,after)
    assert inspect()==baseline
    d=response['error']['diagnostics'];assert d['featureId']==extrude['id'] and d['profileId']==extrude['parameters']['profileId'] and d['featureName']==extrude['name']
    # Association removal is safe; the same stable profile then tests authored curves.
    relation=next(r for r in draft()['parameters']['constraints'] if r['type']=='circleOffset')
    call('sketch.deleteSelection',{'constraintIds':[relation['id']]})
    authored_before=session()
    call('sketch.deleteSelection',{'entityIds':[sketch['parameters']['entities'][0]['id'],unrelated['id']]},error='downstream_profile_in_use')
    assert session()==authored_before and inspect()==baseline
    call('history.undo');assert draft()==before['draftSketch']
    call('sketch.selectionSet',{'entityIds':[unrelated['id']]});before=session();call('sketch.deleteSelection');safe=ledger[-1];changed=draft()
    assert session()['draftHistory']['index']==before['draftHistory']['index']+1
    call('history.undo');assert draft()==before['draftSketch']
    call('history.redo');assert draft()==changed
    call('sketch.finish');final=inspect();assert final['evaluation']['bodies']==baseline['evaluation']['bodies']
    path=out/'safe-deleted.alloy';call('document.save',{'path':str(path)});call('document.open',{'path':str(path),'discard':True});assert inspect()['authored']==final['authored'];assert inspect()['evaluation']['bodies']==final['evaluation']['bodies']
    gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated'];call('capture.screenshot',{'path':str(out/'reopened.png')})
    (out/'receipt.json').write_text(json.dumps({'status':'pass','appSha256':hashlib.sha256(Path(a.app).read_bytes()).hexdigest(),'gpu':gpu,'refusal':response,'dependentElapsedMs':ledger[next(i for i,x in enumerate(ledger) if x.get('response')==response)]['elapsedMs'],'safeElapsedMs':safe['elapsedMs'],'unchangedSessionOnRefusal':True,'unchangedCommittedOnRefusal':True,'exactUndoRedoReopen':True,'sameSolid':True,'nativeFixture':str(out/'native-before.alloy'),'target':target,'unrelated':unrelated['id'],'sketchId':sketch['id']},indent=2)+'\n')
    (out/'transcript.json').write_text(json.dumps(ledger,indent=2)+'\n')
print('PASS: targeted downstream refusal and safe unrelated Delete/history/reopen')
