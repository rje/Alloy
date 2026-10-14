#!/usr/bin/env python3
"""Retained construction-plane Project from a prior authored enclosure and imported sibling face."""
import argparse
import json
from pathlib import Path
from app_harness import application

parser=argparse.ArgumentParser()
parser.add_argument('--app',required=True)
parser.add_argument('--design',required=True)
parser.add_argument('--output',required=True)
a=parser.parse_args()
out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)

def ref(face):return {key:face[key] for key in ('producerFeatureId','bodyId','faceId')}

with application(a.app,out,command_timeout=120) as (call,ledger):
    call('document.open',{'path':str(Path(a.design).resolve()),'discard':True})
    state=call('document.inspect')
    owner=next(c['id'] for c in state['authored']['components'] if c['name']=='Case')
    call('component.activate',{'definitionId':owner})
    bodies=call('document.inspect')['evaluation']['bodies']
    case=next(f for b in bodies if b['ownerId']==owner for f in b['supportFaces']
        if abs(f['frame']['origin'][1]-46.5)<1e-6 and f['frame']['normal'][1]>.99)
    eth=next(f for b in bodies if b['name']=='SIDE-ETH-001' for f in b['supportFaces']
        if abs(f['frame']['origin'][1]-45.5)<1e-6 and f['frame']['normal'][1]>.99 and len(f['edges'])==4)
    source={'support':ref(case)}
    plane=call('construction.plane',{'source':source,'offset':0,'units':'mm','name':'Port reference plane'})['planeId']
    call('sketch.begin',{'planeId':plane,'name':'Linked connector edges'})
    original=call('app.state')['activeSession']['draftSketch']
    call('sketch.projectBegin')
    picked=call('sketch.projectFace',ref(eth))
    assert picked['added']==4 and picked['selectedCount']==4
    call('sketch.projectCancel')
    assert call('app.state')['activeSession']['draftSketch']==original
    call('sketch.projectBegin')
    picked=call('sketch.projectFace',ref(eth))
    assert picked['added']==4 and picked['selectedCount']==4
    call('sketch.projectCommit')
    draft=call('app.state')['activeSession']['draftSketch']
    projections=draft['parameters']['projections']
    assert len(projections)==4 and all(p['source']['bodyId']!=case['bodyId'] for p in projections)
    call('capture.screenshot',{'path':str(out/'construction-project.png')})
    call('sketch.finish')
    current=call('document.inspect')
    assert len(next(s for s in current['evaluation']['sketches'] if s['featureId']==draft['id'])['profiles'])==1
    stable=projections[0]['id']
    call('construction.plane',{'featureId':plane,'source':source,'offset':.25,'units':'mm'})
    moved=call('document.inspect')
    assert abs(next(p for p in moved['evaluation']['constructionPlanes'] if p['featureId']==plane)['frame']['origin'][1]-46.75)<1e-6
    assert next(f for f in moved['authored']['features'] if f['id']==draft['id'])['parameters']['projections'][0]['id']==stable
    call('history.undo')
    undone=call('document.inspect')
    assert abs(next(p for p in undone['evaluation']['constructionPlanes'] if p['featureId']==plane)['frame']['origin'][1]-46.5)<1e-6
    call('history.redo')
    design=out/'construction-project.alloy'
    call('document.save',{'path':str(design)})
    call('document.open',{'path':str(design),'discard':True})
    reopened=call('document.inspect')
    assert next(f for f in reopened['authored']['features'] if f['id']==draft['id'])['parameters']['projections'][0]['id']==stable
    assert len(next(s for s in reopened['evaluation']['sketches'] if s['featureId']==draft['id'])['profiles'])==1
    (out/'ledger.json').write_text(json.dumps(ledger,indent=2))
    (out/'receipt.json').write_text(json.dumps({'planeId':plane,'sketchId':draft['id'],
        'importedFace':ref(eth),'sourceIds':[p['source'] for p in projections],
        'stableProjectionIds':[p['id'] for p in projections],'savedDesign':str(design)},indent=2))
