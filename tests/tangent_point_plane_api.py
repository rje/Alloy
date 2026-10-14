#!/usr/bin/env python3
"""Bounded GEO-004 real editor check on a retained curved Loft endpoint."""
import argparse
import copy
import json
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--source',required=True);p.add_argument('--output',required=True)
a=p.parse_args();out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
with application(a.app,out) as (call,ledger):
    call('window.resize',{'width':1440,'height':900})
    call('document.open',{'path':str(Path(a.source).resolve()),'discard':True})
    initial=call('document.inspect');body=initial['evaluation']['bodies'][0];face=body['supportFaces'][0]
    call('component.activate',{'definitionId':body['ownerId']})
    edge=next(e for e in body['solidEdges'] if e['available'])
    tangent={'face':{k:face[k] for k in ('producerFeatureId','bodyId','faceId')},
             'point':{'bodyId':body['bodyId'],'edgeId':edge['edgeId'],'endpoint':0}}
    session=call('command.begin',{'command':'construction.tangentPlane','inputs':{'tangent':tangent}})
    assert call('app.state')['ui']['activeDialog']['visible']
    preview=call('command.preview',{'session':session['id']});assert preview['previewed']
    call('capture.dialog',{'path':str(out/'tangent-controls.png')})
    call('command.cancel',{'session':session['id']});assert call('document.inspect')['authored']==initial['authored']
    created=call('construction.tangentPlane',{'tangent':tangent,'name':'Endpoint tangent'})['planeId']
    inspected=call('document.inspect');frame=next(f['frame'] for f in inspected['evaluation']['constructionPlanes'] if f['featureId']==created)
    assert max(abs(a-b) for a,b in zip(frame['origin'],edge['startMm']))<1e-7
    call('sketch.create',{'planeId':created,'name':'Attached diameter 5 circle'})
    call('sketch.addCircle',{'centerX':0,'centerY':10,'diameter':5,'units':'mm'})
    call('sketch.finish')
    baseline=call('document.inspect')['authored']
    call('history.undo');call('history.redo');assert call('document.inspect')['authored']==baseline
    bad=copy.deepcopy(tangent);bad['point']['edgeId']='00000000-0000-0000-0000-000000000001'
    call('construction.tangentPlane',{'featureId':created,'tangent':bad},error='evaluation_failed')
    assert call('document.inspect')['authored']==baseline
    call('feature.editDialog',{'featureId':created});edit=call('app.state')['activeSession'];assert edit['inputs']['tangent']==tangent
    call('command.update',{'session':edit['id'],'inputs':{'name':'Edited endpoint tangent'}})
    call('command.commit',{'session':edit['id']})
    saved=call('document.inspect')['authored'];path=out/'tangent-endpoint.alloy';call('document.save',{'path':str(path)})
    call('document.new',{'discard':True});call('document.open',{'path':str(path),'discard':True})
    assert call('document.inspect')['authored']==saved
    call('view.home');call('capture.screenshot',{'path':str(out/'tangent-endpoint.png')})
    (out/'report.json').write_text(json.dumps({'passed':True,'sourceEndpoint':'curved Loft boundary','finalFilletEndpointAccepted':False,'planeId':created,'frame':frame},indent=2))
    (out/'transcript.json').write_text(json.dumps(ledger,indent=2))
