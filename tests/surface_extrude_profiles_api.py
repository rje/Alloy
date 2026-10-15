#!/usr/bin/env python3
"""Real-app grouped Two Sides surface sheets from the accepted iron continuation."""
import argparse, copy, json
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--app',required=True);p.add_argument('--source',required=True);p.add_argument('--output',required=True,type=Path)
a=p.parse_args();out=a.output.resolve()
with application(a.app,out,command_timeout=120) as (call,ledger):
    call('window.resize',{'width':1500,'height':950});call('document.open',{'path':str(Path(a.source).resolve()),'discard':True})
    before=call('document.inspect');sketch=before['authored']['features'][-1];assert sketch['name']=='Back and notch sheets'
    call('component.activate',{'definitionId':sketch['ownerId']});es=sketch['parameters']['entities']
    def start():
        s=call('surface.extrudeDialog',{'sideOneMm':60,'sideTwoMm':65,'name':'Closing trial sheets'})
        assert s['inputs']['resultMode']=='perProfile' and s['chaining']
        call('surface.extrudeEntity',{'sketchId':sketch['id'],'entityId':es[0]['id']})
        call('surface.extrudeEntity',{'sketchId':sketch['id'],'entityId':es[2]['id']})
        active=call('app.state')['activeSession'];assert active['previewed'] and len(active['resultBodyIds'])==2
        assert len(active['inputs']['profiles'][0]['entityIds'])==2 and len(active['inputs']['profiles'][1]['entityIds'])==1
        assert 'bodyId' not in active and 'geometry' not in active
        assert [r['geometry']['topology']['faces'] for r in active['resultBodies']]==[2,1]
        return active
    session=start();call('view.home');call('capture.dialog',{'path':str(out/'two-profile-controls.png')});call('capture.screenshot',{'path':str(out/'two-profile-preview.png')})
    call('command.cancel',{'session':session['id']});assert call('document.inspect')['authored']==before['authored']
    session=start();call('command.commit',{'session':session['id']});initial=call('document.inspect');f=initial['authored']['features'][-1];assert 'bodyId' not in f
    results={b['bodyId']:b for b in initial['evaluation']['bodies'] if b['featureId']==f['id']};assert len(results)==2 and len(initial['evaluation']['bodies'])==4
    assert all(b['ownerId']==sketch['ownerId'] and b['valid'] for b in results.values())
    call('view.home');call('capture.screenshot',{'path':str(out/'two-profile-committed.png')})
    edit=call('feature.editDialog',{'featureId':f['id']});assert edit['previewed'] and set(edit['resultBodyIds'])==set(results)
    call('command.cancel',{'session':edit['id']});assert call('document.inspect')['authored']==initial['authored']
    # The bottom line is an upstream edit to the same sketch; extend independently past both ends.
    call('sketch.edit',{'sketchId':sketch['id']})
    call('sketch.line',{'x1':-15,'y1':-4,'x2':145,'y2':-4,'units':'mm'})
    call('sketch.finish');afterSketch=call('document.inspect');changed=next(x for x in afterSketch['authored']['features'] if x['id']==sketch['id']);bottom=changed['parameters']['entities'][-1]['id']
    edit=call('feature.editDialog',{'featureId':f['id']});call('surface.extrudeProfile',{'sketchId':sketch['id'],'entityIds':[bottom]})
    active=call('app.state')['activeSession'];assert len(active['resultBodyIds'])==3 and set(results)<=set(active['resultBodyIds'])
    call('capture.dialog',{'path':str(out/'three-profile-edit.png')});call('command.commit',{'session':edit['id']});final=call('document.inspect');assert len(final['evaluation']['bodies'])==5
    for b in final['evaluation']['bodies']:
        if b['bodyId'] in results:
            assert [e['edgeId'] for e in b['solidEdges']]==[e['edgeId'] for e in results[b['bodyId']]['solidEdges']]
            assert [x['faceId'] for x in b['supportFaces']]==[x['faceId'] for x in results[b['bodyId']]['supportFaces']]
    call('history.undo');assert call('document.inspect')['authored']==afterSketch['authored'];call('history.redo');assert call('document.inspect')['authored']==final['authored']
    path=out/'five-sheet-precursor.alloy';call('document.save',{'path':str(path)});call('document.new',{'discard':True});call('document.open',{'path':str(path)});assert call('document.inspect')['authored']==final['authored']
    call('view.home');call('capture.screenshot',{'path':str(out/'five-sheet-reopened.png')})
    state=call('app.state');(out/'result.json').write_text(json.dumps({'passed':True,'calls':len(ledger),'resultBodies':list(results),'featureId':f['id'],'gpu':state['viewport']['gpu'],'scope':'Actual retained iron bridge + independently declared two open profiles. Third bottom profile added to same sketch/Extrude; old identities retained. No later iron Trim or closure claim.'},indent=2));(out/'transcript.json').write_text(json.dumps(ledger,indent=2))
print('PASS per-profile Surface Extrude API')
