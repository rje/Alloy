#!/usr/bin/env python3
"""Exact planar concave attachment finish through the shared application commands."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application

parser=argparse.ArgumentParser()
parser.add_argument('--app',required=True)
parser.add_argument('--source',required=True)
parser.add_argument('--output',required=True)
args=parser.parse_args()
out=Path(args.output).resolve()
with application(args.app,out,command_timeout=180) as (call,transcript):
    assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
    call('document.open',{'path':str(Path(args.source).resolve()),'discard':True})
    before=call('document.inspect')
    join=next(f for f in reversed(before['authored']['features']) if f['type']=='extrude' and f['parameters']['operation']=='join')
    call('component.activate',{'definitionId':join['ownerId']})
    before=call('document.inspect')
    target=join['parameters']['targetBodyIds'][0]
    body=lambda state: next(b for b in state['evaluation']['bodies'] if b['bodyId']==target)
    original=body(before)
    roots=[e for e in original['solidEdges'] if e['available'] and e['provenance'].get('role')=='join.root']
    assert len(roots)==1
    inputs={'targetBodyId':target,'edgeIds':[roots[0]['edgeId']],'distanceMm':2}
    call('view.home')
    session=call('solid.chamferDialog',inputs)
    assert session['previewed'],session
    assert call('document.inspect')['authored']==before['authored']
    call('capture.dialog',{'path':str(out/'root-chamfer-dialog.png')})
    call('capture.screenshot',{'path':str(out/'root-chamfer-preview.png')})
    call('command.cancel',{'session':session['id']})
    assert call('document.inspect')['authored']==before['authored']
    session=call('solid.chamferDialog',inputs)
    call('command.commit',{'session':session['id']})
    accepted=call('document.inspect')
    assert body(accepted)['chamferMethod']=='planarConcave'
    assert math.isclose(body(accepted)['volumeMm3']-original['volumeMm3'],11,abs_tol=1e-6)
    assert accepted['authored']['features'][-1]['parameters']['distanceMm']==2
    call('history.undo')
    assert call('document.inspect')['authored']==before['authored']
    call('history.redo')
    path=out/'exact-root-chamfer.alloy'
    call('document.save',{'path':str(path)})
    call('document.open',{'path':str(path),'discard':True})
    reopened=call('document.inspect')
    assert reopened['authored']==accepted['authored']
    assert math.isclose(body(reopened)['volumeMm3'],body(accepted)['volumeMm3'],abs_tol=1e-6)
    call('view.home')
    call('capture.screenshot',{'path':str(out/'root-chamfer-reopened.png')})
    (out/'receipt.json').write_text(json.dumps({'before':before,'accepted':accepted,'reopened':reopened,'transcript':transcript},indent=2)+'\n')
print('PASS exact 2 mm root preview/Cancel/commit/history/reopen through real app')
