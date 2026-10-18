#!/usr/bin/env python3
"""Check actual shared-boundary membership from two planar face actions."""
import argparse, hashlib, json
from pathlib import Path
from app_harness import application

p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
def ref(face):return {k:face[k] for k in ('producerFeatureId','bodyId','faceId')}
with application(a.app,out) as (call,ledger):
    def session():return call('app.state')['activeSession']
    def draft():return session()['draftSketch']
    call('solid.box',{'width':20,'depth':15,'height':5,'units':'mm','name':'Shared source'})
    baseline=call('document.inspect');body=baseline['evaluation']['bodies'][0]
    top=next(f for f in body['supportFaces'] if f['role']=='box.z1')
    side=next(f for f in body['supportFaces'] if f['role']=='box.x0')
    call('sketch.begin',{'support':ref(top),'name':'Shared face membership'})
    call('sketch.projectBegin');first=call('sketch.projectFace',ref(top));second=call('sketch.projectFace',ref(side));call('sketch.projectCommit')
    shared=set(first['projectionIds']) & set(second['projectionIds']);assert len(shared)==1
    target=next(iter(shared));original=draft();groups=original['parameters']['faceProjectionGroups'];assert len(groups)==2
    call('sketch.projectBegin');before=call('sketch.projectPreview');again=call('sketch.projectFace',ref(top));assert again['projectionIds']==first['projectionIds'];assert call('sketch.projectPreview')['previewDraft']==before['previewDraft'];call('sketch.projectCancel');assert draft()==original
    call('sketch.lookAt');call('view.fit');call('sketch.selectionSet',{'entityIds':[target]});call('capture.screenshot',{'path':str(out/'shared-selected.png')})
    history=session()['draftHistory']['index'];call('sketch.deleteSelection');changed=draft()
    expected=json.loads(json.dumps(original['parameters']))
    expected['projections']=[r for r in expected['projections'] if r['id']!=target]
    expected['faceProjectionGroups']=[{**g,'projectionIds':[i for i in g['projectionIds'] if i!=target]} for g in groups]
    assert changed['parameters']==expected;assert session()['draftHistory']['index']==history+1
    call('capture.screenshot',{'path':str(out/'shared-deleted.png')});call('history.undo');assert draft()==original;call('history.redo');assert draft()==changed
    call('sketch.finish');final=call('document.inspect');assert final['evaluation']['bodies']==baseline['evaluation']['bodies']
    path=out/'shared-deleted.alloy';call('document.save',{'path':str(path)});call('document.open',{'path':str(path),'discard':True});assert call('document.inspect')['authored']==final['authored'];assert call('document.inspect')['evaluation']['bodies']==baseline['evaluation']['bodies']
    call('capture.screenshot',{'path':str(out/'shared-reopened.png')});gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated']
    (out/'receipt.json').write_text(json.dumps({'status':'pass','appSha256':hashlib.sha256(Path(a.app).read_bytes()).hexdigest(),'groups':groups,'sharedProjectionId':target,'expectedAfter':expected,'duplicateStable':True,'exactUndoRedoReopen':True,'sourceBodiesUnchanged':True,'gpu':gpu},indent=2)+'\n')
    (out/'transcript.json').write_text(json.dumps(ledger,indent=2)+'\n')
print('PASS: two real face actions share one stable result; Delete updates both memberships and preserves source/history/reopen')
