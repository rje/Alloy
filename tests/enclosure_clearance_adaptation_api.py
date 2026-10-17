#!/usr/bin/env python3
"""Apply the approved nominal-connector clearance adaptation through real Alloy."""
import argparse
import json
from pathlib import Path
from app_harness import application

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--app',required=True)
p.add_argument('--source',required=True,type=Path)
p.add_argument('--output',required=True,type=Path)
p.add_argument('--end-offset',type=float,default=-1.5,
               help='Signed To Object depth offset, not the resulting measured clearance')
a=p.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
with application(a.app,out,command_timeout=180) as(call,ledger):
    try:
        call('document.open',{'path':str(a.source.resolve()),'discard':True})
        before=call('document.inspect');authored=before['authored']
        sketch=next(f for f in authored['features'] if f['type']=='sketch' and f['name']=='Three linked port clearances')
        cut=next(f for f in authored['features'] if f['type']=='extrude' and f['parameters'].get('sketchId')==sketch['id'])
        assert cut['parameters']['extent']=='toObject' and cut['parameters']['operation']=='cut'
        session=call('feature.editDialog',{'featureId':cut['id']})
        call('command.update',{'session':session['id'],'inputs':{'endOffset':a.end_offset}})
        call('command.preview',{'session':session['id']})
        call('command.commit',{'session':session['id']})
        after=call('document.inspect')
        expected=json.loads(json.dumps(authored))
        next(f for f in expected['features'] if f['id']==cut['id'])['parameters']['endOffsetMm']=a.end_offset
        assert after['authored']==expected,'Only the retained port Cut depth may change'
        assert len(after['evaluation']['bodies'])==len(before['evaluation']['bodies'])
        call('history.undo');assert call('document.inspect')['authored']==authored
        call('history.redo');assert call('document.inspect')['authored']==expected
        target=out/'enclosure-nominal-clearance.alloy'
        call('document.save',{'path':str(target)})
        call('document.open',{'path':str(target),'discard':True})
        reopened=call('document.inspect');assert reopened['authored']==expected
        call('view.orientation',{'name':'rearIsometric'});call('view.fit')
        call('capture.screenshot',{'path':str(out/'adapted-enclosure.png')})
        split=next(f for f in expected['features'] if f['type']=='splitBody')
        for name,identity in [('Top',split['bodyId']),('Bottom',split['parameters']['targetBodyId'])]:
            body=next(b for b in reopened['evaluation']['bodies'] if b['bodyId']==identity)
            assert body['valid'] and body['topology']['solids']==1
        # The pre-rim checkpoint is an intermediate model. Export and inspect
        # each finished half after the four rim-tail operations are recreated.
        (out/'result.json').write_text(json.dumps({'requestedMinimumClearanceMm':0.5,
            'actualMinimumClearance':'requires independent BRep measurement',
            'adaptation':'User-approved clearance against unchanged nominal connector; physical fit unverified',
            'endOffsetMm':a.end_offset,'source':str(a.source.resolve()),'featureId':cut['id'],
            'before':before,'after':reopened,'next':'Recreate rim tail, then export and measure finished halves'},indent=2)+'\n')
    finally:
        (out/'transcript.json').write_text(json.dumps(ledger,indent=2)+'\n')
print('PASS authored adaptation and exact undo/redo/reopen; recreate rim tail next')
