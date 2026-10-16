#!/usr/bin/env python3
"""Actual separate sheet tools → retained bridge portions → Remove and revert."""
import argparse, copy, json, uuid
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--source',required=True);p.add_argument('--output',required=True)
a=p.parse_args();out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
with application(a.app,out,command_timeout=180) as (call,ledger):
 try:
    call('window.resize',{'width':1500,'height':950});call('document.open',{'path':str(Path(a.source).resolve()),'discard':True})
    before=call('document.inspect')
    def bodies(s):return s['evaluation']['bodies']
    def body(s,bid):return next(b for b in bodies(s) if b['bodyId']==bid)
    target=next(b for b in bodies(before) if b['supportFaces'][0]['role']=='loft.closedSection')
    tools=[b for b in bodies(before) if b['supportFaces'][0]['role']=='surfaceExtrude.sheet']
    upper=next(b for b in tools if b['topology']['faces']==2);lower=next(b for b in tools if b['topology']['faces']==1)
    rear=next(e['edgeId'] for e in target['solidEdges'] if e['provenance']['role']=='seam.end')
    front=next(e['edgeId'] for e in target['solidEdges'] if e['provenance']['role']=='seam.start')
    call('component.activate',{'definitionId':target['ownerId']})
    call('view.home')
    timings=[]
    for index,tool in enumerate((upper,lower)):
        session=call('surface.trimDialog',{'name':('Upper' if index==0 else 'Lower')+' sheet trial'})
        assert not session['inputs'].get('targetBodyId') and session['selectionRole']=='tool'
        sid=session['id'];call('surface.trimBody',{'bodyId':tool['bodyId']});s=call('app.state')['activeSession'];assert s['selectionRole']=='target'
        call('surface.trimTarget',{'bodyId':target['bodyId']});s=call('app.state')['activeSession']
        assert s['partitionBuildCount']==1 and len(s['regions'])==(3 if index==0 else 4)
        removed=[r for r in s['regions'] if r['removable'] and rear in r['signature'] and front not in r['signature']]
        assert len(removed)==(1 if index==0 else 2)
        for r in removed:
            call('surface.trimRegion',{'regionId':r['regionId']});timings.append({'command':'region','elapsedMs':ledger[-1]['elapsedMs']})
            s=call('app.state')['activeSession'];assert s['partitionBuildCount']==1 and s['partitionReused']
        assert s['previewed'] and s['geometry']['result']['valid']
        call('capture.dialog',{'path':str(out/f'{index+1}-controls.png')});call('capture.screenshot',{'path':str(out/f'{index+1}-red-preview.png')})
        if index==0:
            unchanged=call('document.inspect')['authored'];call('command.cancel',{'session':sid});assert call('document.inspect')['authored']==unchanged
            session=call('surface.trimDialog',{'targetBodyId':target['bodyId'],'toolBodyId':tool['bodyId'],'name':'Upper sheet trial'});sid=session['id']
            s=call('app.state')['activeSession']
            for r in s['regions']:
                if r['removable'] and rear in r['signature'] and front not in r['signature']:call('surface.trimRegion',{'regionId':r['regionId']})
        call('command.commit',{'session':sid});current=call('document.inspect');b=body(current,target['bodyId'])
        assert b['valid'] and b['topology']['faces']==2 and b['topology']['solids']==0 and len(bodies(current))==4
        for t in tools:
            assert body(current,t['bodyId'])['topology']==t['topology'] and abs(body(current,t['bodyId'])['areaMm2']-t['areaMm2'])<1e-8
    trimmed=current
    def identity(b):return {f['faceId'] for f in b['supportFaces']},{e['edgeId'] for e in b['solidEdges']}
    stable=identity(body(trimmed,target['bodyId']));feature=trimmed['authored']['features'][-1]
    edit=call('feature.editDialog',{'featureId':feature['id']});assert edit['previewed'];call('command.cancel',{'session':edit['id']})
    source=next(f for f in trimmed['authored']['features'] if f['type']=='surfaceExtrude');sketch=next(f for f in trimmed['authored']['features'] if f['id']==source['parameters']['sketchId'])
    params=copy.deepcopy(sketch['parameters']);mid=params['entities'][0]['pointIds'][1]
    for point in params['points']:
        if point['id']==mid:point['vMm']+=.5
    call('feature.edit',{'featureId':sketch['id'],'parameters':params});edited=call('document.inspect');assert identity(body(edited,target['bodyId']))==stable
    assert abs(body(edited,target['bodyId'])['areaMm2']-body(trimmed,target['bodyId'])['areaMm2'])>.01
    call('surface.trim',{'featureId':feature['id'],'targetBodyId':target['bodyId'],'toolBodyId':lower['bodyId'],'removedRegionIds':[str(uuid.uuid4())]},error='evaluation_failed')
    assert call('document.inspect')['authored']==edited['authored']
    for t in tools:call('body.remove',{'bodyId':t['bodyId']})
    assert len(bodies(call('document.inspect')))==2
    call('history.undo');call('history.undo');assert call('document.inspect')['authored']==edited['authored']
    call('history.undo');assert call('document.inspect')['authored']==trimmed['authored']
    call('history.undo');call('history.undo');assert call('document.inspect')['authored']==before['authored']
    for _ in range(3):call('history.redo')
    assert call('document.inspect')['authored']==edited['authored']
    saved=out/'edited-sheet-trials.alloy';call('document.save',{'path':str(saved)});call('document.open',{'path':str(saved)});reopened=call('document.inspect');assert reopened['authored']==edited['authored'] and identity(body(reopened,target['bodyId']))==stable
    call('view.home');call('capture.screenshot',{'path':str(out/'3-reopened-trials.png')})
    gpu=call('app.state')['viewport']['gpu']
    (out/'result.json').write_text(json.dumps({'passed':True,'calls':len(ledger),'targetBodyId':target['bodyId'],'upperToolId':upper['bodyId'],'lowerToolId':lower['bodyId'],'warmRegionTimings':timings,'gpu':gpu,'scope':'Unchanged accepted four-body source; declared rear bridge portions, upper then lower. No closure or bottom correction claim.'},indent=2))
 finally:(out/'transcript.json').write_text(json.dumps(ledger,indent=2))
