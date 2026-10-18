#!/usr/bin/env python3
"""Projection-backed rectangle Offset owner Delete through the real app."""
import argparse, hashlib, json, math
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
def ref(face):return {k:face[k] for k in ('producerFeatureId','bodyId','faceId')}
with application(a.app,out) as (call,ledger):
    def session():return call('app.state')['activeSession']
    def draft():return session()['draftSketch']
    def params():return draft()['parameters']
    for w,d,x in [(20,10,0),(12,8,30)]:call('solid.box',{'width':w,'depth':d,'height':5,'x':x,'units':'mm'})
    baseline=call('document.inspect');bodies=baseline['evaluation']['bodies'];faces=[next(f for f in b['supportFaces'] if f['role']=='box.z1') for b in bodies]
    call('sketch.begin',{'support':ref(faces[0]),'name':'Rectangle Offset Delete'})
    call('sketch.projectBegin')
    for face in faces:call('sketch.projectFace',ref(face))
    call('sketch.projectCommit');sources=params()['projections'];assert len(sources)==8
    for body in bodies:
        ids=[r['id'] for r in sources if r['source']['bodyId']==body['bodyId']]
        call('sketch.tool',{'tool':'offset'})
        call('sketch.offsetPreview',{'sourceEntityIds':ids,'distance':1,'units':'mm'});call('sketch.offsetAccept')
    before=draft();groups=params()['closedRectOffsets'];g=groups[0];target=g['entityIds'][0]
    assert g['sourceProfiles']==[True]*4
    call('sketch.finish');call('document.save',{'path':str(out/'native-before.alloy')})
    call('sketch.edit',{'sketchId':before['id']});call('sketch.lookAt');call('view.fit')
    for ids in [[g['sourceEntityIds'][0]],[target,g['sourceEntityIds'][0]],[target,'stale']]:
        state=session();call('sketch.deleteSelection',{'entityIds':ids},error='sketch_solve_failed');after=session()
        assert state['draftSketch']==after['draftSketch'] and state['draftHistory']==after['draftHistory']
    # Core checks cover a shared generated corner; a retained measurement
    # exercises surviving-reference refusal through the public command.
    call('sketch.dimension',{'kind':'linear','entityId':target,'driving':False})
    state=session();call('sketch.deleteSelection',{'entityIds':[target]},error='sketch_solve_failed');after=session()
    assert state['draftSketch']==after['draftSketch'] and state['draftHistory']==after['draftHistory']
    call('history.undo')
    before=draft();index=session()['draftHistory']['index']
    call('sketch.selectionSet',{'entityIds':[target]});call('capture.screenshot',{'path':str(out/'selected.png')})
    call('sketch.deleteSelection');changed=draft();pnew=params()
    assert pnew['closedRectOffsets']==[groups[1]]
    assert pnew['entities']==[e for e in before['parameters']['entities'] if e['id'] not in g['entityIds']]
    assert pnew['points']==[e for e in before['parameters']['points'] if e['id'] not in g['pointIds']]
    expected=[dict(r,profile=True) if r['id'] in g['sourceEntityIds'] else r for r in before['parameters']['projections']]
    assert pnew['projections']==expected and session()['draftHistory']['index']==index+1
    call('history.undo');assert draft()==before
    call('history.redo');assert draft()==changed
    call('capture.screenshot',{'path':str(out/'deleted.png')})
    call('sketch.finish');state=call('document.inspect');evaluated=next(s for s in state['evaluation']['sketches'] if s['featureId']==before['id'])
    areas=sorted(p['areaMm2'] for p in evaluated['profiles']);assert len(areas)==2 and all(math.isclose(x,y,abs_tol=1e-7) for x,y in zip(areas,[140,200])),areas
    assert state['evaluation']['bodies']==baseline['evaluation']['bodies']
    authored=state['authored'];path=out/'rectangle-offset-deleted.alloy';call('document.save',{'path':str(path)});call('document.open',{'path':str(path),'discard':True});assert call('document.inspect')['authored']==authored
    call('capture.screenshot',{'path':str(out/'reopened.png')})
    # Existing historical preflight also guards generated rectangle profiles.
    current=call('document.inspect');second_profile=next(p for s in current['evaluation']['sketches'] if s['featureId']==before['id'] for p in s['profiles'] if math.isclose(p['areaMm2'],140,abs_tol=1e-7))
    call('solid.extrude',{'sketchId':before['id'],'profileId':second_profile['profileId'],'distance':3,'units':'mm'})
    committed=call('document.inspect');call('sketch.edit',{'sketchId':before['id']});state=session()
    call('sketch.deleteSelection',{'entityIds':[groups[1]['entityIds'][0]]},error='downstream_profile_in_use')
    after=session();assert state['draftSketch']==after['draftSketch'] and state['draftHistory']==after['draftHistory']
    assert call('document.inspect')==committed
    downstream_refusal=ledger[-3]['response']
    call('sketch.cancel')
    gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated']
    (out/'receipt.json').write_text(json.dumps({'status':'pass','appSha256':hashlib.sha256(Path(a.app).read_bytes()).hexdigest(),'gpu':gpu,'groups':groups,'projections':sources,'profileAreasMm2':areas,'savedPath':str(path),'exactUndoRedoReopen':True,'sameSupportBodies':True,'downstreamRefusal':downstream_refusal},indent=2)+'\n')
    (out/'transcript.json').write_text(json.dumps(ledger,indent=2)+'\n')
print('PASS: rectangle Offset owner Delete, projections/dependencies, exact history/reopen and GPU')
