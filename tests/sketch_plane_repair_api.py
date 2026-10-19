#!/usr/bin/env python3
"""Explicit ordinary plane repair through the real saved-model application path."""
import argparse,json,hashlib
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--fixtures',required=True);p.add_argument('--output',required=True);a=p.parse_args();folder=Path(a.fixtures).resolve();out=Path(a.output).resolve();ids=json.loads((folder/'repair-fixtures.json').read_text())
with application(a.app,out) as (call,ledger):
    inspect=lambda:call('document.inspect')
    session=lambda:call('app.state')['activeSession']
    call('document.open',{'path':str(folder/'ordinary-plane-repair.alloy')});call('selection.set',{'bodyId':ids['targetBodyId']})
    base=inspect();original=next(f for f in base['authored']['features'] if f['id']==ids['sketchId']);assert all(k not in original['parameters'] for k in ('projections','loopOffsets'))
    args={'featureId':ids['combineFeatureId'],'targetBodyId':ids['targetBodyId'],'toolBodyIds':[ids['deleteBodyId']],'operation':'cut','keepTools':True}
    call('solid.combine',args,error='evaluation_failed');repair=call('history.failedEditInspect');cached=session();camera=call('app.state')['viewport']['navigation'];assert inspect()==base
    call('sketch.redefinePlaneBegin',{'sketchId':ids['referenceFeatureId']},error='repair_unavailable');assert session()==cached and inspect()==base
    call('sketch.redefinePlaneBegin',{'sketchId':ids['sketchId']});choice={'support':ids['replacementSupport']}
    preview=call('sketch.redefinePlanePreview',{'replacement':choice});assert preview['frame']['origin']==[0,40,8] and preview['frame']['normal']==[0,0,1]
    downstream=next(b for b in preview['geometry'] if b['featureId']==ids['downstreamFeatureId']);assert downstream['geometry']['boundsMm']['min']==[3.5,43.5,8]
    current=session();missing={**ids['replacementSupport'],'faceId':'00000000-0000-0000-0000-000000000001'}
    for replacement,code in [({'support':missing},'support_unavailable'),({'support':ids['curvedSupport']},'nonplanar_support'),({'support':ids['laterSupport']},'later_support'),({'support':ids['support']},'support_unavailable'),({'plane':'XZ'},'unsupported_plane_orientation')]:
        call('sketch.redefinePlanePreview',{'replacement':replacement},error=code);assert session()==current and inspect()==base
    call('sketch.redefinePlaneCancel');assert session()==cached and inspect()==base
    state=call('app.state');assert state['viewport']['navigation']==camera
    assert state['viewport']['gpu']['draftSketchVisible'] and state['viewport']['gpu']['draftSketchId']==ids['sketchId'] and state['viewport']['gpu']['bodyStageKey']==''
    # A split also leaves the old exact face non-unique; it must not be guessed.
    call('command.cancel',{'session':cached['id']});args['toolBodyIds']=[ids['splitBodyId']];call('solid.combine',args,error='evaluation_failed')
    split_cached=session();call('sketch.redefinePlaneBegin',{'sketchId':ids['sketchId']});before_invalid=session()
    call('sketch.redefinePlanePreview',{'replacement':{'support':ids['support']}},error='support_unavailable');assert session()==before_invalid and inspect()==base
    call('sketch.redefinePlaneCancel');assert session()==split_cached;call('command.cancel',{'session':split_cached['id']})
    args['toolBodyIds']=[ids['deleteBodyId']];call('solid.combine',args,error='evaluation_failed');call('sketch.redefinePlaneBegin',{'sketchId':ids['sketchId']})
    for replacement,origin in [({'plane':'XY'},[0,0,0]),({'planeId':ids['constructionPlaneId']},[0,0,8]),(choice,[0,40,8])]:
        result=call('sketch.redefinePlanePreview',{'replacement':replacement});assert result['frame']['origin']==origin and inspect()==base
    call('view.orientation',{'name':'isometric'});call('view.fit');call('capture.screenshot',{'path':str(out/'candidate-preview.png')})
    call('sketch.redefinePlaneCommit');after=inspect();assert session()=={} and call('history.failedEditInspect')=={}
    call('sketch.redefinePlaneCommit',error='replacement_required');assert inspect()==after and session()=={}
    repaired=next(f for f in after['authored']['features'] if f['id']==ids['sketchId']);expected={**original,'parameters':{**original['parameters'],'support':ids['replacementSupport']}};assert repaired==expected
    assert next(f for f in after['authored']['features'] if f['id']==ids['combineFeatureId'])['parameters']['toolBodyIds']==[ids['deleteBodyId']]
    assert after['undo']['count']==base['undo']['count']+1 and after['undo']['index']==base['undo']['index']+1
    target=next(b for b in after['evaluation']['bodies'] if b['featureId']==ids['downstreamFeatureId']);assert target['boundsMm']['min']==[3.5,43.5,8] and target['boundsMm']['max']==[6.5,46.5,10]
    call('history.undo');undone=inspect();assert undone['authored']==base['authored'] and undone['evaluation']['bodies']==base['evaluation']['bodies']
    call('history.redo');assert inspect()['authored']==after['authored'] and inspect()['evaluation']['bodies']==after['evaluation']['bodies']
    call('document.save',{'path':str(out/'repaired.alloy')});call('document.open',{'path':str(out/'repaired.alloy')});reopened=inspect();assert reopened['authored']==after['authored'] and reopened['evaluation']['bodies']==after['evaluation']['bodies']
    call('capture.screenshot',{'path':str(out/'repaired.png')});gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated']
    # A new committed edit invalidates a prepared repair, including its dock and candidate.
    call('document.open',{'path':str(folder/'ordinary-plane-repair.alloy')});call('solid.combine',args,error='evaluation_failed');call('sketch.redefinePlaneBegin',{'sketchId':ids['sketchId']});call('sketch.redefinePlanePreview',{'replacement':choice})
    call('feature.edit',{'featureId':ids['referenceFeatureId'],'parameters':{'heightMm':9}});changed=inspect();assert changed['revision']!=base['revision'] and session()=={} and call('history.failedEditInspect')=={}
    call('sketch.redefinePlaneCommit',error='replacement_required');assert inspect()==changed
    (out/'receipt.json').write_text(json.dumps({'status':'pass','appSha256':hashlib.sha256(Path(a.app).read_bytes()).hexdigest(),'input':'local API; physical native unqualified','framePolicy':'preserve local coordinates, require parallel matching axes','repair':repair,'preview':preview,'before':base,'after':after,'gpu':gpu},indent=2)+'\n');(out/'transcript.json').write_text(json.dumps(ledger,indent=2)+'\n')
print('PASS: explicit face/plane preview, exact rejection/Cancel, aggregate atomic repair, downstream geometry/history/reopen')
