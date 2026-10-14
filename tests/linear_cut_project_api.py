#!/usr/bin/env python3
"""Continue the generated sliding-lid prefix through retained Cut-boundary Project."""
import argparse
import json
import copy
import math
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--source',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
def ref(f):return {k:f[k] for k in ('producerFeatureId','bodyId','faceId')}
def same_authored_after_solve(left,right):
    # Re-solving retained mirror coordinates may differ at machine precision.
    # Authored identities, constraints, parameters and all other data stay exact.
    adjusted=copy.deepcopy(right)
    assert len(left['features'])==len(adjusted['features'])
    for before,after in zip(left['features'],adjusted['features']):
        old=before.get('parameters',{}).get('points',[])
        new=after.get('parameters',{}).get('points',[])
        assert len(old)==len(new)
        for a,b in zip(old,new):
            assert a['id']==b['id']
            for key in ('uMm','vMm'):
                if key in a:
                    assert math.isclose(a[key],b[key],rel_tol=0,abs_tol=1e-9)
                    b[key]=a[key]
    assert left==adjusted

with application(a.app,out) as (call,ledger):
    call('document.open',{'path':str(Path(a.source).resolve()),'discard':True})
    def inspect():return call('document.inspect')
    def front(state):return next(f for f in state['evaluation']['bodies'][0]['supportFaces'] if f['role']=='box.y0')
    source=inspect();face=front(source);assert len(face['edges'])==8
    ids={e['edgeId'] for e in face['edges']};assert len(ids)==8
    named=[e for e in face['edges'] if e.get('lineage')=='exactLinearCutHistory'];assert len(named)==5
    bottom=[e for e in named if len(e.get('contributors',[]))==2];assert len(bottom)==1
    lid=call('component.create',{'name':'Lid'})['definitionId'];call('component.activate',{'definitionId':lid})
    call('sketch.begin',{'support':ref(face),'name':'Projected dovetail lid'})
    call('sketch.projectBegin');call('sketch.projectFace',ref(face));project=call('sketch.projectCommit')
    assert len(project['sketchGeometry']['projectedLines'])==8
    call('sketch.finish');baseline=inspect();project_feature=baseline['authored']['features'][-1]
    def edit(feature,inputs):
        session=call('feature.editDialog',{'featureId':feature['id']})
        call('command.update',{'session':session['id'],'inputs':inputs});call('command.commit',{'session':session['id']})
    box=next(f for f in baseline['authored']['features'] if f['type']=='box')
    shell=next(f for f in baseline['authored']['features'] if f['type']=='shell')
    edit(box,{'width':74});edit(shell,{'thicknessMm':3.5})
    updated=inspect();assert {e['edgeId'] for e in front(updated)['edges']}==ids
    by_id={e['edgeId']:e for e in front(updated)['edges']}
    assert by_id[bottom[0]['edgeId']]['contributors']==bottom[0]['contributors']
    shoulders=[e for e in named if any(c.startswith('split/') for c in e['contributors'])]
    assert len(shoulders)==2
    for previous in shoulders:
        current=by_id[previous['edgeId']]
        assert current['contributors']==previous['contributors']
        old_x=[previous['startMm'][0],previous['endMm'][0]]
        new_x=[current['startMm'][0],current['endMm'][0]]
        if max(old_x)<25: assert abs(min(new_x))<1e-6 and abs(max(new_x)-3.5)<1e-6
        else: assert abs(min(new_x)-70.5)<1e-6 and abs(max(new_x)-74)<1e-6
    projected_lines=updated['evaluation']['sketches'][-1]['projectedLines']
    assert abs(max(max(l['u1Mm'],l['u2Mm']) for l in projected_lines)-74)<1e-6
    assert projected_lines!=baseline['evaluation']['sketches'][-1]['projectedLines']
    assert updated['authored']['features'][-1]==project_feature
    assert updated['evaluation']['bodies'][0]['valid']
    assert len(updated['evaluation']['sketches'][-1]['projectedLines'])==8
    call('history.undo');call('history.redo');assert inspect()['authored']==updated['authored']
    saved=out/'linked-cut-lid.alloy';call('document.save',{'path':str(saved)});call('document.open',{'path':str(saved),'discard':True})
    reopened=inspect()
    same_authored_after_solve(updated['authored'],reopened['authored']);assert {e['edgeId'] for e in front(reopened)['edges']}==ids
    call('sketch.edit',{'sketchId':project_feature['id']});call('sketch.lookAt')
    call('capture.screenshot',{'path':str(out/'projected-cut-after-edits.png')});call('sketch.finish')
    (out/'receipt.json').write_text(json.dumps({'source':source,'baseline':baseline,'updated':updated,'transcript':ledger},indent=2)+'\n')
print('PASS: actual Cut eight boundaries, merged bottom, sibling Project, width/thickness edits and reopen')
