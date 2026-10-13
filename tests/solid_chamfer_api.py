#!/usr/bin/env python3
"""Bounded equal-distance Chamfer; actual app evidence, separate native review."""
import argparse,json,math
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,log):
    call('sketch.begin',{'plane':'XY','name':'Chamfer cube base'})
    call('sketch.rectangle',{'x1':-10,'y1':-10,'x2':10,'y2':10,'units':'mm'})
    call('sketch.finish');state=call('document.inspect');sketch=state['evaluation']['sketches'][-1]
    call('solid.extrude',{'sketchId':sketch['sketchId'],'profileId':sketch['profiles'][0]['profileId'],'distance':20,'units':'mm'})
    source=call('document.inspect');body=source['evaluation']['bodies'][0];target=body['bodyId'];edges=[e['edgeId'] for e in body['solidEdges'] if e['available']];assert len(edges)==12
    call('view.home');session=call('solid.chamferDialog',{'targetBodyId':target,'edgeIds':edges,'distanceMm':1})
    preview=call('command.preview',{'session':session['id']});assert preview['geometry']['valid'] and preview['geometry']['closed'];assert 7800<preview['geometry']['volumeMm3']<8000
    call('capture.screenshot',{'path':str(out/'chamfer-preview.png')});call('command.cancel',{'session':session['id']});assert call('document.inspect')['authored']==source['authored']
    session=call('solid.chamferDialog',{'targetBodyId':target,'edgeIds':edges,'distanceMm':1});call('command.commit',{'session':session['id']});result=call('document.inspect');assert result['evaluation']['bodies'][0]['bodyId']==target
    call('history.undo');assert call('document.inspect')['authored']==source['authored'];call('history.redo');assert call('document.inspect')['authored']==result['authored']
    feature=result['authored']['features'][-1]['id'];edit=call('feature.editDialog',{'featureId':feature});call('command.update',{'session':edit['id'],'inputs':{'distanceMm':.5}});call('command.preview',{'session':edit['id']});call('command.commit',{'session':edit['id']});assert call('document.inspect')['evaluation']['bodies'][0]['distanceMm']==.5;call('history.undo')
    extrude=result['authored']['features'][-2];call('solid.extrude',{'featureId':extrude['id'],'sketchId':sketch['sketchId'],'profileId':sketch['profiles'][0]['profileId'],'distance':24,'units':'mm'});assert call('document.inspect')['evaluation']['bodies'][0]['valid'];call('history.undo')
    bad=call('solid.chamferDialog',{'targetBodyId':target,'edgeIds':edges,'distanceMm':30});assert not bad['previewed'];call('command.cancel',{'session':bad['id']});assert call('document.inspect')['authored']==result['authored']
    path=out/'cube-chamfered.alloy';call('document.save',{'path':str(path)});call('document.open',{'path':str(path)});assert call('document.inspect')['authored']==result['authored'];call('capture.screenshot',{'path':str(out/'chamfer-committed.png')})
    (out/'report.json').write_text(json.dumps({'passed':True,'scope':'shared API/geometry/history/storage; physical input and visual acceptance separate','source':source,'result':result},indent=2));(out/'transcript.json').write_text(json.dumps(log,indent=2))
