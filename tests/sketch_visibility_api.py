#!/usr/bin/env python3
"""Real-entrypoint session presentation, authored/history and reopen checks."""
import argparse
import json
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--fixture',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,ledger):
    call('document.open',{'path':str(Path(a.fixture).resolve())})
    authored=call('document.inspect')['authored'];mixed=next(f for f in authored['features'] if f['name']=='Mixed entity roles')
    call('sketch.edit',{'sketchId':mixed['id']});call('sketch.lookAt')
    def session():return call('app.state')['activeSession']
    before=session();defaults=before['displayOptions']
    assert all(defaults[k] for k in defaults if k!='hideFullyConstrained') and not defaults['hideFullyConstrained']
    for key in defaults:
        call('sketch.displayOptions',{key:not defaults[key]});current=session()
        assert current['displayOptions']=={**defaults,key:not defaults[key]}
        assert current['draftSketch']==before['draftSketch'] and current['draftHistory']==before['draftHistory'] and current['solverDiagnostics']==before['solverDiagnostics']
        assert call('app.state')['viewport']['gpu']['sketchDisplayOptions']==current['displayOptions']
        call('sketch.displayOptions',{key:defaults[key]})
    # Visible boundary location, captured from evaluated projection rather than a guessed face frame.
    line=call('app.state')['viewport']['gpu']['projectedLines'][0]
    # Pointer behavior is covered by native controls fixture; stable ID API selection
    # also refuses hidden geometry while inspection preserves the entity records.
    call('sketch.selectionSet',{'entityIds':[line['id']]})
    call('sketch.displayOptions',{'showProjectedGeometries':False});assert not session()['selection'].get('entityIds')
    call('sketch.selectionSet',{'entityIds':[line['id']]},error='geometry_hidden')
    call('sketch.displayOptions',{'showProjectedGeometries':True,'showPoints':False})
    call('sketch.finish');assert call('document.inspect')['authored']==authored
    call('document.save',{'path':str(out/'visibility.alloy')});call('document.open',{'path':str(out/'visibility.alloy')})
    assert call('document.inspect')['authored']==authored
    call('sketch.edit',{'sketchId':mixed['id']});assert session()['displayOptions']==defaults
    call('sketch.lookAt');state=call('app.state');assert state['viewport']['gpu']['hardwareAccelerated']
    call('capture.screenshot',{'path':str(out/'all-visible.png')})
    call('sketch.displayOptions',{'hideFullyConstrained':True});call('capture.screenshot',{'path':str(out/'focus.png')})
    (out/'receipt.json').write_text(json.dumps({'result':'passed','state':state,'defaults':defaults,'transcript':ledger},indent=2)+'\n')
print('PASS: independent session presentation, shared API/native options, authored/history invariance, hidden selection refusal, reopen reset and GPU capture')
