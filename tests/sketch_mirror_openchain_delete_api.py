#!/usr/bin/env python3
"""Retained line/arc Mirror and open-chain Offset owner Delete through the app."""
import argparse, hashlib, json
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,ledger):
    def session():return call('app.state')['activeSession']
    def draft():return session()['draftSketch']
    def params():return draft()['parameters']
    def line(x1,y1,x2,y2,**kw):
        call('sketch.line',dict(x1=x1,y1=y1,x2=x2,y2=y2,units='mm',**kw));return params()['entities'][-1]
    results=[]
    for family in ['mirrorGroups','openChainOffsets']:
        call('document.new',{'discard':True})
        if family=='openChainOffsets':call('document.parameterAdd',{'name':'clearance','value':0.5,'units':'mm'})
        call('sketch.begin',{'plane':'XY','name':family});call('sketch.lookAt')
        call('sketch.addCircle',{'centerX':-35,'centerY':-25,'diameter':10,'units':'mm'});circle=params()['entities'][-1]
        call('sketch.constraint',{'type':'fixed','entityIds':[circle['id']]})
        if family=='mirrorGroups':
            axis=line(-30,0,30,0);source=line(0,0,12,15)
            call('sketch.addArc',{'mode':'center','centerX':20,'centerY':15,'radius':5,'startAngleDeg':0,'sweepAngleDeg':120,'units':'mm'});arc=params()['entities'][-1]
            call('sketch.mirrorPreview',{'sourceEntityIds':[source['id'],arc['id']],'axisEntityId':axis['id']});call('sketch.mirrorAccept')
            g=params()[family][0];target=g['entityPairs'][1]['resultEntityId'];sources=[source,arc,axis]
        else:
            sources=[line(*xy) for xy in [(-12,10,-8,10),(-8,10,-6,0),(-6,0,6,0),(6,0,8,10),(8,10,12,10),(-8,10,8,10)]]
            call('sketch.tool',{'tool':'offset'})
            call('sketch.offsetPreview',{'sourceEntityIds':[e['id'] for e in sources[:5]],'topEntityId':sources[5]['id'],'parameterName':'clearance','units':'mm'});call('sketch.offsetAccept')
            g=params()[family][0];target=g['generatedEntityIds'][2]
            points={q['id']:q for q in params()['points']};left,right=[points[i] for i in g['topIntersectionPointIds']]
            closure=line(left['uMm'],left['vMm'],right['uMm'],right['vMm'],startSnapTarget={'kind':'point','pointId':left['id']},endSnapTarget={'kind':'point','pointId':right['id']})
            parameter_before=call('document.inspect')['authored']['userParameters']
        baseline=draft();index=session()['draftHistory']['index'];old=baseline['parameters']
        for ids in [[sources[0]['id']],[target,sources[0]['id']],[target,'stale']]:
            before=session();call('sketch.deleteSelection',{'entityIds':ids},error='sketch_solve_failed');after=session()
            assert before['draftSketch']==after['draftSketch'] and before['draftHistory']==after['draftHistory']
        call('sketch.selectionSet',{'entityIds':[target]});call('capture.screenshot',{'path':str(out/(family+'-selected.png'))})
        call('sketch.deleteSelection');changed=draft();new=params()
        assert family not in new and session()['draftHistory']['index']==index+1
        assert circle in new['entities'] and all(e in new['entities'] for e in sources)
        old_points={q['id']:q for q in old['points']};assert all(q==old_points[q['id']] for q in new['points'])
        if family=='openChainOffsets':
            assert closure in new['entities'] and left in new['points'] and right in new['points']
            assert call('document.inspect')['authored']['userParameters']==parameter_before
        call('history.undo');assert draft()==baseline
        call('history.redo');assert draft()==changed
        call('capture.screenshot',{'path':str(out/(family+'-deleted.png'))})
        call('sketch.finish');authored=call('document.inspect')['authored'];path=out/(family+'-deleted.alloy')
        call('document.save',{'path':str(path)});call('document.open',{'path':str(path),'discard':True});assert call('document.inspect')['authored']==authored
        call('capture.screenshot',{'path':str(out/(family+'-reopened.png'))})
        results.append({'family':family,'groupId':g['id'],'selectedMember':target,'savedPath':str(path),'exactUndoRedoReopen':True})
    gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated']
    (out/'receipt.json').write_text(json.dumps({'status':'pass','appSha256':hashlib.sha256(Path(a.app).read_bytes()).hexdigest(),'gpu':gpu,'fixtures':results},indent=2)+'\n')
    (out/'transcript.json').write_text(json.dumps(ledger,indent=2)+'\n')
print('PASS: Mirror/open-chain owner Delete, sources/shared IDs, parameter, history/reopen and GPU')
