#!/usr/bin/env python3
"""Temporary real-app continuation probe; final acceptance runs from blank."""
import argparse
import json
from pathlib import Path
from app_harness import application
from enclosure_snap_continuation import continue_snaps
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--source',required=True);p.add_argument('--output',required=True)
a=p.parse_args();out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
with application(a.app,out,command_timeout=120) as (call,ledger):
    call('window.resize',{'width':1400,'height':900})
    call('document.open',{'path':str(Path(a.source).resolve()),'discard':True})
    def inspect():return call('document.inspect')
    base=inspect()
    case=next(c for c in base['authored']['components'] if c['name']=='Case')
    owner=case['id'];call('component.activate',{'definitionId':owner})
    body=next(b for b in base['evaluation']['bodies'] if b['ownerId']==owner)
    reference=next(f for f in base['authored']['features'] if f['type']=='stepAssemblyImport')
    def stage(text,capture=None):
        print(text,flush=True)
        if capture:call('capture.screenshot',{'path':str(out/capture)})
    result=continue_snaps(call=call,stage=stage,inspect=inspect,out=out,
        case_owner=owner,case_body_id=body['bodyId'],
        reference_id=reference['id'],analyses=call('analysis.inspect')['analyses'])
    (out/'probe-result.json').write_text(json.dumps({'result':result,'ledger':ledger},indent=2)+'\n')
print('PASS real-app continuation probe through source-order retained snaps')
