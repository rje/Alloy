#!/usr/bin/env python3
"""DOC-025 real-app priority, retained edge identity and command override."""
import argparse
import json
from pathlib import Path
from app_harness import application

parser=argparse.ArgumentParser()
parser.add_argument('--app',required=True)
parser.add_argument('--output',required=True)
args=parser.parse_args()
out=Path(args.output).resolve()

with application(args.app,out) as (call,ledger):
    call('window.resize',{'width':1440,'height':900})
    assert call('app.state')['ui']['selectionPriority']=='auto'
    call('solid.box',{'width':50,'depth':80,'height':40,'x':0,'y':0,'z':0,'units':'mm','name':'Priority Box'})
    initial=call('document.inspect')
    body=initial['evaluation']['bodies'][0]
    body_id=body['bodyId']
    assert len(body['solidEdges'])==12
    top=next(f for f in body['supportFaces'] if f['role']=='box.z1')
    face_ref={k:top[k] for k in ('bodyId','producerFeatureId','faceId')}
    call('view.home');call('view.orientation',{'name':'top'})
    top_edges=[e for e in body['solidEdges'] if e['available'] and
        all(abs(p[2]-40)<1e-5 for p in (e['startMm'],e['endMm']))]
    assert len(top_edges)==4
    edge=top_edges[0]
    pixel=call('view.projectEdge',{'bodyId':body_id,'edgeId':edge['edgeId']})
    call('selection.priority',{'mode':'edge'})
    assert call('app.state')['ui']['selectionPriority']=='edge'
    picked=call('selection.pick',{'x':pixel['x'],'y':pixel['y']})
    assert picked['kind']=='edge' and picked['bodyId']==body_id and picked['edgeId']==edge['edgeId']
    assert call('app.state')['selection']['edgeId']==edge['edgeId']
    call('capture.screenshot',{'path':str(out/'edge-selected.png')})
    call('selection.pick',{'x':0,'y':0},error='edge_unavailable')
    assert call('app.state')['selection']['edgeId']==edge['edgeId']
    call('selection.edgeSet',{'bodyId':body_id,'edgeId':'00000000-0000-0000-0000-000000000000'},error='edge_unavailable')
    assert call('app.state')['selection']['edgeId']==edge['edgeId']
    call('selection.priority',{'mode':'face'})
    interior=call('view.projectFacePoint',{'support':face_ref,'uMm':25,'vMm':40})
    face=call('selection.pick',{'x':interior['x'],'y':interior['y']})
    assert face['kind']=='face' and face['support']==face_ref
    call('selection.priority',{'mode':'body'})
    selected=call('selection.pick',{'x':interior['x'],'y':interior['y']})
    assert selected['kind']=='body' and selected['bodyId']==body_id
    call('selection.priority',{'mode':'edge'})
    active=call('solid.filletDialog')
    assert call('app.state')['ui']['selectionPriority']=='edge'
    active_pixel=call('view.projectEdge',{'bodyId':body_id,'edgeId':edge['edgeId']})
    explicit=call('selection.pick',{'x':active_pixel['x'],'y':active_pixel['y']})
    assert explicit['kind']=='edge' and explicit['edgeId']==edge['edgeId']
    call('command.cancel',{'session':active['id']})
    assert call('app.state')['ui']['selectionPriority']=='edge'
    assert call('document.inspect')['authored']==initial['authored']
    call('selection.priority',{'mode':'edge'})
    assert call('app.state')['ui']['selectionPriority']=='auto'
    auto=call('selection.pick',{'x':interior['x'],'y':interior['y']})
    assert auto['kind']=='face' and auto['support']==face_ref
    assert call('document.inspect')['authored']==initial['authored']
    (out/'report.json').write_text(json.dumps({'passed':True,'bodyId':body_id,
        'edgeId':edge['edgeId'],'face':face_ref,'priorityAfterCancel':'edge'},indent=2))
    (out/'transcript.json').write_text(json.dumps(ledger,indent=2))
