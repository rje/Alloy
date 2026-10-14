#!/usr/bin/env python3
"""Real-app Overall Slot, cut, retained edits, history and reopen."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application

p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True)
a=p.parse_args();out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
with application(a.app,out) as (call,ledger):
    call('solid.box',{'width':40,'depth':20,'height':5,'x':-20,'y':-10,'z':0,'units':'mm','name':'Stock'})
    stock=call('document.inspect');body=stock['evaluation']['bodies'][0]['bodyId']
    call('sketch.begin',{'plane':'XY'})
    call('sketch.overallSlot',{'firstX':-15,'firstY':0,'lastX':15,'lastY':0,'width':6.5,'units':'mm'})
    draft=call('app.state')['activeSession']['draftSketch'];params=draft['parameters'];group=params['slotGroups'][0]
    assert group['mode']=='overall' and len(params['entities'])==5 and len(params['dimensions'])==3
    length_id,width_id,angle_id=[d['id'] for d in params['dimensions']]
    assert [d['valueMm'] for d in params['dimensions'][:2]]==[30,6.5]
    sketch_id=draft['id'];call('sketch.finish')
    info=call('document.inspect')['evaluation']['sketches'][0]
    assert len(info['profiles'])==1
    assert math.isclose(info['profiles'][0]['areaMm2'],23.5*6.5+math.pi*3.25**2,abs_tol=1e-5)
    call('solid.extrude',{'sketchId':sketch_id,'profileId':info['profiles'][0]['profileId'],
        'distance':5,'units':'mm','operation':'cut','targetBodyIds':[body]})
    cut=call('document.inspect');assert len(cut['evaluation']['bodies'])==1
    volume=40*20*5-5*(23.5*6.5+math.pi*3.25**2)
    assert math.isclose(cut['evaluation']['bodies'][0]['volumeMm3'],volume,abs_tol=1e-3)
    call('view.orientation',{'name':'top'});call('view.home')
    call('capture.screenshot',{'path':str(out/'overall-slot-cut.png')})
    before=cut['authored']
    call('sketch.edit',{'sketchId':sketch_id})
    call('sketch.dimensionUpdate',{'dimensionId':length_id,'value':32,'units':'mm'})
    call('sketch.dimensionUpdate',{'dimensionId':width_id,'value':7,'units':'mm'})
    call('sketch.dimensionUpdate',{'dimensionId':angle_id,'valueDeg':10})
    changed=call('app.state')['activeSession']['draftSketch']
    assert changed['parameters']['slotGroups'][0]['id']==group['id']
    call('sketch.dimensionUpdate',{'dimensionId':length_id,'value':6,'units':'mm'},error='sketch_solve_failed')
    assert call('app.state')['activeSession']['draftSketch']==changed
    call('sketch.finish');after=call('document.inspect')
    assert after['authored']!=before and after['evaluation']['bodies'][0]['valid']
    call('history.undo');assert call('document.inspect')['authored']==before
    call('history.redo');assert call('document.inspect')['authored']==after['authored']
    path=out/'overall-slot-cut.alloy';call('document.save',{'path':str(path)})
    call('document.new',{'discard':True});call('document.open',{'path':str(path),'discard':True})
    assert call('document.inspect')['authored']==after['authored']
    (out/'overall-slot-api.json').write_text(json.dumps({'calls':len(ledger),'initialCutVolumeMm3':volume,
        'slotGroupId':group['id'],'lengthDimensionId':length_id,'widthDimensionId':width_id,'angleDimensionId':angle_id},indent=2)+'\n')
print('Overall Slot real-app cut and history passed')
