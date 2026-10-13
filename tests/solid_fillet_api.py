#!/usr/bin/env python3
"""Bounded source-video fillet using a saved source-order joined support fixture."""
import argparse,json,math
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--fixture',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,log):
    call('document.open',{'path':str(Path(a.fixture).resolve())});source=call('document.inspect');body=source['evaluation']['bodies'][0];target=body['bodyId'];seeds=[]
    for e in body['solidEdges']:
        if not e['available']:continue
        x,y=e['startMm'],e['endMm'];role=e['provenance']['role']
        if abs(x[0]-32)>1e-6 or abs(y[0]-32)>1e-6:continue
        if (role=='sideCorner' and abs(x[1]+10)<1e-6 and max(x[2],y[2])>23.9) or (role=='endRim' and abs(x[2]-24)<1e-6 and abs(x[1]-y[1])>9):seeds.append(e['edgeId'])
    assert len(seeds)==2
    session=call('solid.filletDialog',{'targetBodyId':target});assert session['inputs']['radiusMm']==0
    # Actual edge hit testing from projected geometry, separate from physical native input.
    for edge in seeds:
        point=call('view.projectEdge',{'bodyId':target,'edgeId':edge});hit=call('selection.pick',{'x':point['x'],'y':point['y']});assert hit['edgeId']==edge
    call('command.update',{'session':session['id'],'inputs':{'radiusMm':1}});preview=call('command.preview',{'session':session['id']});assert preview['geometry']['valid'];assert call('document.inspect')['authored']==source['authored']
    call('capture.screenshot',{'path':str(out/'fillet-preview.png')});call('command.cancel',{'session':session['id']});assert call('document.inspect')['authored']==source['authored']
    session=call('solid.filletDialog',{'targetBodyId':target,'edgeIds':seeds,'radiusMm':1});call('command.commit',{'session':session['id']});result=call('document.inspect');assert result['evaluation']['bodies'][0]['bodyId']==target
    original=next(f for f in body['supportFaces'] if abs(f['frame']['origin'][2]-20)<1e-6);assert any(f['faceId']==original['faceId'] for f in result['evaluation']['bodies'][0]['supportFaces'])
    call('history.undo');assert call('document.inspect')['authored']==source['authored'];call('history.redo');assert call('document.inspect')['authored']==result['authored']
    feature=result['authored']['features'][-1]['id'];edit=call('feature.editDialog',{'featureId':feature});call('command.update',{'session':edit['id'],'inputs':{'radiusMm':.5}});call('command.commit',{'session':edit['id']});assert call('document.inspect')['evaluation']['bodies'][0]['radiusMm']==.5;call('history.undo')
    path=out/'support-filleted.alloy';call('document.save',{'path':str(path)});call('document.open',{'path':str(path)});assert call('document.inspect')['authored']==result['authored'];call('capture.screenshot',{'path':str(out/'fillet-committed.png')})
    (out/'report.json').write_text(json.dumps({'passed':True,'sourceReferenceId':'support-original-video-20261012','selectedEdgeIds':seeds,'source':source,'result':result},indent=2));(out/'transcript.json').write_text(json.dumps(log,indent=2))
