#!/usr/bin/env python3
"""One actual three-port Cut edit through explicit retained Offset-loop intent."""
import argparse
import json
import math
import uuid
from pathlib import Path
from app_harness import application

p=argparse.ArgumentParser()
p.add_argument('--app',required=True)
p.add_argument('--design',required=True)
p.add_argument('--output',required=True)
a=p.parse_args()
out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)

with application(a.app,out,command_timeout=120) as (call,ledger):
    call('document.open',{'path':str(Path(a.design).resolve()),'discard':True})
    initial=call('document.inspect')
    sketch=next(f for f in initial['authored']['features'] if f['type']=='sketch' and len(f['parameters'].get('closedRectOffsets',[]))==3)
    cut=next(f for f in initial['authored']['features'] if f['type']=='extrude' and f['parameters'].get('sketchId')==sketch['id'] and f['parameters'].get('operation')=='cut')
    group_ids=[g['id'] for g in sketch['parameters']['closedRectOffsets']]
    selection={'kind':'closedRectOffsetUnion','groupIds':group_ids}
    args={'featureId':cut['id'],'sketchId':sketch['id'],'profileSelection':selection,
          'extent':'toObject','endSupport':cut['parameters']['endSupport'],
          'operation':'cut','targetBodyIds':cut['parameters']['targetBodyIds']}
    call('solid.extrude',args)
    bound=call('document.inspect')
    authored_cut=next(f for f in bound['authored']['features'] if f['id']==cut['id'])
    assert authored_cut['parameters']['profileSelection']['groupIds']==sorted(group_ids)
    assert 'profileIds' not in authored_cut['parameters']
    source_body=cut['parameters']['targetBodyIds'][0]
    def area_and_volume(state):
        evaluated=state['evaluation']
        current=next(s for s in evaluated['sketches'] if s['featureId']==sketch['id'])
        body=next(b for b in evaluated['bodies'] if b['bodyId']==source_body)
        return sum(p['areaMm2'] for p in current['profiles']),body['volumeMm3'],[p['profileId'] for p in current['profiles']]
    area0,volume0,cells0=area_and_volume(bound)
    assert math.isclose(area0,973.5,abs_tol=1e-5)
    call('sketch.edit',{'sketchId':sketch['id']})
    call('sketch.closedRectOffsetUpdate',{'offsetId':group_ids[0],'distance':1.1,'units':'mm'})
    call('sketch.finish')
    changed=call('document.inspect')
    area1,volume1,cells1=area_and_volume(changed)
    assert math.isclose(area1,979.43,abs_tol=1e-5),(area1,cells1)
    assert cells0!=cells1 and volume1<volume0
    assert next(f for f in changed['authored']['features'] if f['id']==cut['id'])['parameters']['profileSelection']==authored_cut['parameters']['profileSelection']
    call('capture.screenshot',{'path':str(out/'edited-three-port-cut.png')})
    call('history.undo')
    assert math.isclose(area_and_volume(call('document.inspect'))[0],area0,abs_tol=1e-5)
    call('history.redo')
    assert math.isclose(area_and_volume(call('document.inspect'))[0],area1,abs_tol=1e-5)
    saved=out/'edited-three-port-cut.alloy'
    call('document.save',{'path':str(saved)})
    call('document.open',{'path':str(saved),'discard':True})
    reopened=call('document.inspect')
    assert reopened['authored']==changed['authored']
    assert math.isclose(area_and_volume(reopened)[0],area1,abs_tol=1e-5)
    missing=dict(args);missing['profileSelection']={'kind':'closedRectOffsetUnion',
        'groupIds':[str(uuid.uuid4()),*group_ids[1:]]}
    call('solid.extrude',missing,error='evaluation_failed')
    mixed=dict(args);mixed['profileIds']=cells0
    call('solid.extrude',mixed,error='invalid_arguments')
    assert call('document.inspect')['authored']==reopened['authored']
    (out/'result.json').write_text(json.dumps({'areaBeforeMm2':area0,'areaAfterMm2':area1,
        'volumeBeforeMm3':volume0,'volumeAfterMm3':volume1,'cellsBefore':cells0,'cellsAfter':cells1,
        'groupIds':group_ids,'cutFeatureId':cut['id'],'saved':str(saved)},indent=2))
    (out/'ledger.json').write_text(json.dumps(ledger,indent=2))
