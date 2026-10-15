#!/usr/bin/env python3
"""Actual retained Trim loops -> middle ellipse -> no-guide New Body bridge."""
import argparse
import copy
import json
from pathlib import Path
from app_harness import application

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--app',required=True);p.add_argument('--source',required=True);p.add_argument('--output',required=True,type=Path)
a=p.parse_args();out=a.output.resolve()
with application(a.app,out,command_timeout=120) as (call,ledger):
    call('window.resize',{'width':1500,'height':950});call('document.open',{'path':str(Path(a.source).resolve()),'discard':True})
    before=call('document.inspect');authored=before['authored'];source=before['evaluation']['bodies'][0]
    middle=next(f for f in authored['features'] if f['name']=='Handle middle ellipse')
    ellipse=next(e for e in middle['parameters']['entities'] if e['type']=='ellipse')
    selected=[]
    for face in source['supportFaces']:
        candidates=[e for e in source['solidEdges'] if e['provenance'].get('role')=='trimBoundary'
                    and e['provenance']['sourceFaceId']==face['provenance']['sourceFaceId']]
        assert len(candidates)==2
        selected.append(sorted(candidates,key=lambda e:e['provenance']['sourceEntityId'])[0]['edgeId'])
    inputs={'operation':'newBody','guideType':'none','name':'Retained handle bridge'}
    def select():
        session=call('surface.loftDialog',inputs)
        first=call('solid.loftBoundary',{'role':'section1','bodyId':source['bodyId'],'edgeId':selected[0]})
        call('solid.loftEntity',{'role':'section2','sketchId':middle['id'],'entityId':ellipse['id']})
        last=call('solid.loftBoundary',{'role':'section3','bodyId':source['bodyId'],'edgeId':selected[1]})
        active=call('app.state')['activeSession'];assert active['previewed']
        sections=active['inputs']['sections'];assert all(len(sections[i]['edgeIds'])==2 for i in [0,2])
        assert sections[0]['faceId']!=sections[2]['faceId'] and sections[0]['producerFeatureId']==sections[2]['producerFeatureId']
        assert active['geometry']['type']=='surface' and active['geometry']['topology']['solids']==0
        return session,sections
    call('view.home');session,sections=select()
    call('capture.dialog',{'path':str(out/'bridge-controls.png')});call('capture.screenshot',{'path':str(out/'bridge-preview.png')})
    call('command.cancel',{'session':session['id']});assert call('document.inspect')['authored']==authored
    session,sections=select();call('command.commit',{'session':session['id']})
    initial=call('document.inspect');bridge=initial['evaluation']['bodies'][-1]
    assert len(initial['evaluation']['bodies'])==2 and bridge['type']=='surface' and bridge['valid']
    assert bridge['topology']['faces']==2 and bridge['topology']['solids']==0 and len(bridge['solidEdges'])==6
    assert initial['authored']['features'][-1]['parameters']=={'operation':'newBody','guideType':'none','sections':sections}
    assert initial['authored']['components']==authored['components']
    face_ids=[f['faceId'] for f in bridge['supportFaces']];edge_ids=[e['edgeId'] for e in bridge['solidEdges']]
    call('view.home');call('capture.screenshot',{'path':str(out/'bridge-committed.png')})
    params=copy.deepcopy(middle['parameters']);axis=next(e for e in params['entities'] if e['id']==ellipse['secondAxisId'])
    for point in params['points']:
        if point['id']==axis['startPointId']:point['vMm']=1
        if point['id']==axis['endPointId']:point['vMm']=29
    call('feature.edit',{'featureId':middle['id'],'parameters':params})
    edited=call('document.inspect');current=edited['evaluation']['bodies'][-1]
    assert [f['faceId'] for f in current['supportFaces']]==face_ids and [e['edgeId'] for e in current['solidEdges']]==edge_ids
    assert abs(current['areaMm2']-bridge['areaMm2'])>1
    call('history.undo');assert call('document.inspect')['authored']==initial['authored']
    call('history.redo');assert call('document.inspect')['authored']==edited['authored']
    path=out/'retained-handle-bridge.alloy';call('document.save',{'path':str(path)})
    call('document.new',{'discard':True});call('document.open',{'path':str(path)})
    assert call('document.inspect')['authored']==edited['authored']
    edit=call('feature.editDialog',{'featureId':initial['authored']['features'][-1]['id']})
    assert edit['previewed'] and edit['inputs']['sections']==sections and edit['inputs']['operation']=='newBody'
    call('capture.dialog',{'path':str(out/'bridge-edit-controls.png')})
    call('command.cancel',{'session':edit['id']});assert call('document.inspect')['authored']==edited['authored']
    call('view.home');call('capture.screenshot',{'path':str(out/'bridge-reopened.png')})
    call('view.orientation',{'name':'rearIsometric'});call('view.fit');call('capture.screenshot',{'path':str(out/'bridge-rear.png')})
    state=call('app.state')
    (out/'result.json').write_text(json.dumps({'passed':True,'calls':len(ledger),'sections':sections,'originalBridge':bridge,'editedBridge':current,
        'gpu':state['viewport']['gpu'],'scope':'Shared API chain picks/preview/Cancel/New Body commit, ellipse edit/history/reopen; actual supports and source edits separately verified by core.'},indent=2))
    (out/'transcript.json').write_text(json.dumps(ledger,indent=2))
print('PASS retained handle bridge API')
