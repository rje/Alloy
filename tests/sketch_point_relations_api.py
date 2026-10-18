#!/usr/bin/env python3
"""Bounded real-entrypoint control check using the retained native mixed fixture."""
import argparse
import json
from pathlib import Path
from app_harness import application

p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--fixture',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,ledger):
    call('document.open',{'path':str(Path(a.fixture).resolve())})
    authored=call('document.inspect')['authored'];sketch=next(f for f in authored['features'] if f['name']=='Contextual point relations')
    call('sketch.edit',{'sketchId':sketch['id']})
    def session():return call('app.state')['activeSession']
    def draft():return session()['draftSketch']
    def glyphs():return call('app.state')['viewport']['gpu']['sketchConstraintGlyphs']
    def ids():return {g['constraintId'] for g in glyphs()}
    base=draft();relations=base['parameters']['constraints'];pp=next(c for c in relations if c['type']=='coincident');pc=next(c for c in relations if c['type']=='pointOnCurve')
    assert pp['id'] not in ids() and pc['id'] in ids()
    call('sketch.displayOptions',{'showConstraints':False});assert not ids()
    for relation in (pp,pc):
        call('sketch.selectionSet',{'pointIds':[relation['pointIds'][0]]});assert relation['id'] in ids()
        assert next(g for g in glyphs() if g['constraintId']==relation['id'])['contextual']
        call('sketch.deleteCoincident',{'constraintId':relation['id']});removed=draft();assert relation not in removed['parameters']['constraints']
        call('sketch.deleteCoincident',{'constraintId':relation['id']},error='stale_constraint');assert draft()==removed
        call('history.undo');assert draft()==base
        call('history.redo');assert draft()==removed
        call('history.undo');assert draft()==base
    call('sketch.selectionSet');assert not ids()
    call('sketch.constraintSelect',{'constraintId':pc['id']},error='annotation_hidden');assert draft()==base
    call('sketch.finish');assert call('document.inspect')['authored']==authored
    call('document.save',{'path':str(out/'point-relations-api.alloy')});call('document.open',{'path':str(out/'point-relations-api.alloy')});assert call('document.inspect')['authored']==authored
    call('sketch.edit',{'sketchId':sketch['id']});assert pp['id'] not in ids() and pc['id'] in ids()
    call('sketch.selectionSet',{'pointIds':pc['pointIds']});call('sketch.deleteCoincident',{'constraintId':pc['id']})
    call('sketch.finish');deleted=call('document.inspect')['authored'];assert deleted!=authored
    call('document.save',{'path':str(out/'point-relations-deleted.alloy')});call('document.open',{'path':str(out/'point-relations-deleted.alloy')});assert call('document.inspect')['authored']==deleted
    call('sketch.edit',{'sketchId':sketch['id']});assert pc not in draft()['parameters']['constraints'] and pp in draft()['parameters']['constraints']
    call('sketch.displayOptions',{'showConstraints':False});call('sketch.selectionSet',{'pointIds':[pp['pointIds'][0]]});assert ids()=={pp['id']}
    call('capture.screenshot',{'path':str(out/'point-relation-api.png')});state=call('app.state');call('sketch.cancel')
    (out/'receipt.json').write_text(json.dumps({'result':'passed','state':state,'transcript':ledger},indent=2)+'\n')
print('PASS: contextual reveal, stable-ID refusal/deletion, Undo/Redo, authored save/reopen, local API and GPU capture')
