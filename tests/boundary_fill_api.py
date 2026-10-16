#!/usr/bin/env python3
"""Retained Boundary Fill through the authoritative real editor commands."""
import argparse,copy,json,uuid
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--source',required=True);p.add_argument('--output',required=True)
a=p.parse_args();out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
with application(a.app,out,command_timeout=180) as (call,ledger):
 try:
    call('window.resize',{'width':1500,'height':950});call('document.open',{'path':str(Path(a.source).resolve()),'discard':True})
    before=call('document.inspect');tools=before['evaluation']['bodies'];assert len(tools)==5 and all(b['type']=='surface' for b in tools)
    owner=tools[0]['ownerId'];assert all(b['ownerId']==owner for b in tools);call('component.activate',{'definitionId':owner});call('ui.designTab',{'tab':'surface'});call('view.home')
    def state():return call('app.state')['activeSession']
    def choose():
        session=call('surface.boundaryFillDialog');assert session['selectionRole']=='tools' and not session['inputs']['removeTools'] and session['inputs']['operation']=='newBody'
        for b in tools:
            selected=call('surface.boundaryFillTool',{'bodyId':b['bodyId']});assert selected.get('partitionBuildCount',0)==0
        cells=call('surface.boundaryFillSelectCells');assert len(cells['cells'])==1 and cells['partitionBuildCount']==1 and not cells['previewed']
        cell=cells['cells'][0];assert cell['selectionPointMm'] and cell['geometry']['valid'] and cell['geometry']['volumeMm3']>0
        return session['id'],cell
    sid,cell=choose();call('surface.boundaryFillCell',{'cellId':cell['cellId']});s=state();assert s['previewed'] and s['partitionReused'] and s['partitionBuildCount']==1
    call('command.update',{'session':sid,'inputs':{'removeTools':True,'name':'Iron material cell'}});s=state();assert s['previewed'] and s['partitionBuildCount']==1 and s['partitionReused']
    call('capture.dialog',{'path':str(out/'1-cell-controls.png')});call('capture.screenshot',{'path':str(out/'1-selected-cell.png')})
    assert call('document.inspect')['authored']==before['authored'];call('command.cancel',{'session':sid});assert call('document.inspect')['authored']==before['authored']
    # Exterior and handle sheets alone remain open. Zero-cell enumeration must
    # keep acceptance disabled, without inventing a bottom or choosing a hull.
    empty=call('surface.boundaryFillDialog',{'tools':[{'bodyId':b['bodyId'],'producerFeatureId':b['featureId']} for b in tools[:2]]})
    zero=call('surface.boundaryFillSelectCells');assert not zero.get('cells') and not zero['previewed']
    call('command.commit',{'session':empty['id']},error='boundary_fill_cell_required');call('command.cancel',{'session':empty['id']})
    assert call('document.inspect')['authored']==before['authored']
    sid,cell=choose();call('surface.boundaryFillCell',{'cellId':cell['cellId']});call('command.update',{'session':sid,'inputs':{'removeTools':True}})
    call('command.commit',{'session':sid});committed=call('document.inspect');solid=committed['evaluation']['bodies'];assert len(solid)==1 and solid[0]['valid'] and solid[0]['topology']['solids']==1
    feature=committed['authored']['features'][-1];assert feature['type']=='boundaryFill' and feature['ownerId']==owner and feature['parameters']['removeTools']
    assert len(feature['parameters']['tools'])==5 and feature['parameters']['cell']==cell['cell']
    call('capture.screenshot',{'path':str(out/'2-solid.png')})
    edit=call('feature.editDialog',{'featureId':feature['id']});assert edit['previewed'] and edit['partitionBuildCount']==1
    call('command.update',{'session':edit['id'],'inputs':{'removeTools':False}});s=state();assert s['previewed'] and s['partitionReused'] and s['partitionBuildCount']==1
    call('command.cancel',{'session':edit['id']});assert call('document.inspect')['authored']==committed['authored']
    call('history.undo');assert call('document.inspect')['authored']==before['authored'];call('history.redo');assert call('document.inspect')['authored']==committed['authored']
    path=out/'boundary-filled-iron.alloy';call('document.save',{'path':str(path)});call('document.open',{'path':str(path)});reopened=call('document.inspect');assert reopened['authored']==committed['authored']
    body=reopened['evaluation']['bodies'][0];assert body['topology']==solid[0]['topology'] and abs(body['volumeMm3']-solid[0]['volumeMm3'])<1e-6
    call('view.home');call('capture.screenshot',{'path':str(out/'3-reopened-solid.png')})
    # Exact stale revision cleanup and unavailable selection are checked through
    # a replacement edit, without another expensive full modeling sequence.
    edit=call('feature.editDialog',{'featureId':feature['id']});call('surface.boundaryFillCell',{'cellId':str(uuid.uuid4())},error='boundary_fill_cell_unavailable')
    call('feature.visibility',{'featureId':feature['id'],'visible':False});changed=call('document.inspect')['authored']
    call('command.preview',{'session':edit['id']},error='stale_session');s=state();assert not s['previewed'] and not s.get('cells') and not s.get('geometry')
    call('command.cancel',{'session':edit['id']});assert call('document.inspect')['authored']==changed
    gpu=call('app.state')['viewport']['gpu'];(out/'result.json').write_text(json.dumps({'passed':True,'calls':len(ledger),'cell':cell,'solid':solid[0],'gpu':gpu,'scope':'Accepted deterministic five-sheet fixture. Actual selected cell and explicit Remove Tools; no tutorial movie or complete closure-source parity claim.'},indent=2))
 finally:(out/'transcript.json').write_text(json.dumps(ledger,indent=2))
