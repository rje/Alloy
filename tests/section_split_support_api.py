#!/usr/bin/env python3
"""Retained section follows a unique named face into a Split descendant."""
import argparse
import json
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--source',required=True);p.add_argument('--output',required=True,type=Path)
a=p.parse_args();out=a.output.resolve()
with application(a.app,out,command_timeout=120) as (call,ledger):
    call('document.open',{'path':str(Path(a.source).resolve())})
    original=call('document.inspect')['authored']
    section=next(x for x in original['analyses'] if x['name']=='Top')
    def top(): return next(x for x in call('analysis.inspect')['analyses'] if x['id']==section['id'])
    def check(z):
        row=top();assert row['availability']=='available',row
        assert row['support']==section['support']
        assert abs(row['resolvedPlane']['origin'][2]-z)<1e-6
        assert row['resolvedPlane']['normal']==[0,0,-1]
    check(24.5)
    call('analysis.visibility',{'analysisId':section['id'],'visible':True})
    assert call('app.state')['viewport']['gpu']['sectionClippedActors']>0
    call('view.home');call('capture.screenshot',{'path':str(out/'retained-top-section.png')})
    before=call('document.inspect')['authored']
    stock=next(f for f in before['features'] if f['id']==section['support']['producerFeatureId'])
    call('component.activate',{'definitionId':stock['ownerId']})
    call('solid.extrude',{'featureId':stock['id'],'sketchId':stock['parameters']['sketchId'],
         'profileId':stock['parameters']['profileId'],'distance':31,'units':'mm'})
    check(25.5)
    call('history.undo');check(24.5)
    assert call('document.inspect')['authored']==before
    call('history.redo');check(25.5)
    saved=call('document.inspect')['authored']
    call('document.save',{'path':str(out/'section-followed.alloy')})
    call('document.open',{'path':str(out/'section-followed.alloy')});check(25.5)
    assert call('document.inspect')['authored']==saved
    # A side face split between two descendants must not silently choose a half.
    call('document.new',{'discard':True})
    call('solid.box',{'width':40,'depth':30,'height':20,'units':'mm'})
    body=call('document.inspect')['evaluation']['bodies'][0]
    def ref(role):
        f=next(f for f in body['supportFaces'] if f['role']==role)
        return {k:f[k] for k in ('producerFeatureId','bodyId','faceId')}
    call('inspect.section',{'plane':'face','support':ref('box.x1'),'name':'Side'})
    plane=call('construction.midplane',{'faces':[ref('box.z0'),ref('box.z1')]})['planeId']
    call('solid.splitBody',{'targetBodyId':body['bodyId'],'planeId':plane})
    assert call('analysis.inspect')['analyses'][0]['availability']=='unavailable'
    assert call('app.state')['viewport']['gpu']['sectionClippedActors']==0
    call('history.undo')
    assert call('analysis.inspect')['analyses'][0]['availability']=='available'
    (out/'report.json').write_text(json.dumps({'passed':True,'sourceSupport':section['support'],'calls':len(ledger)},indent=2))
    (out/'transcript.json').write_text(json.dumps(ledger,indent=2))
print('PASS: unique retained Split section, source edit/history/reopen and ambiguity refusal')
