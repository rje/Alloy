#!/usr/bin/env python3
"""Retained local-body names on actual accepted Split halves."""
import argparse
import json
from pathlib import Path
from app_harness import application

p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--source',required=True);p.add_argument('--output',required=True)
a=p.parse_args();out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
with application(a.app,out,command_timeout=90) as (call,ledger):
    call('document.open',{'path':str(Path(a.source).resolve()),'discard':True})
    before=call('document.inspect')
    split=next(f for f in before['authored']['features'] if f['type']=='splitBody')
    top=split['bodyId']; bottom=split['parameters']['targetBodyId']
    names={b['bodyId']:b['name'] for b in before['evaluation']['bodies']}
    original={b['bodyId']:(b['volumeMm3'],b['boundsMm']) for b in before['evaluation']['bodies'] if b['bodyId'] in (top,bottom)}
    assert len(original)==2
    call('body.rename',{'bodyId':bottom,'name':'Bottom'})
    call('body.rename',{'bodyId':top,'name':'Top'})
    named=call('document.inspect')
    assert {b['bodyId']:b['name'] for b in named['evaluation']['bodies'] if b['bodyId'] in original}=={bottom:'Bottom',top:'Top'}
    assert {b['bodyId']:(b['volumeMm3'],b['boundsMm']) for b in named['evaluation']['bodies'] if b['bodyId'] in original}==original
    assert named['authored']['features']==before['authored']['features']
    call('body.rename',{'bodyId':'00000000-0000-0000-0000-000000000001','name':'Wrong'},error='invalid_body_rename')
    assert call('document.inspect')['authored']==named['authored']
    call('history.undo');assert call('document.inspect')['evaluation']['bodies'][-1]['name']!= 'Top'
    call('history.redo');assert call('document.inspect')['authored']==named['authored']
    path=out/'named-split.alloy';call('document.save',{'path':str(path)})
    call('document.new',{'discard':True});call('document.open',{'path':str(path),'discard':True})
    reopened=call('document.inspect');assert reopened['authored']==named['authored']
    assert {b['bodyId']:b['name'] for b in reopened['evaluation']['bodies'] if b['bodyId'] in original}=={bottom:'Bottom',top:'Top'}
    call('capture.screenshot',{'path':str(out/'named-split-browser.png')})
    (out/'receipt.json').write_text(json.dumps({'bodyIds':{'bottom':bottom,'top':top},'originalNames':names,
        'originalGeometry':original,'named':named,'reopened':reopened,'ledger':ledger},indent=2)+'\n')
print('PASS retained Split body names, geometry/feature preservation, refusal, undo/reopen')
