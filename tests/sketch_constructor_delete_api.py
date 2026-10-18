#!/usr/bin/env python3
"""Real-app owner-aware constructor Delete; history, saved data and GPU evidence."""
import argparse, hashlib, json, math
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,ledger):
    def session():return call('app.state')['activeSession']
    def draft():return session()['draftSketch']
    def params():return draft()['parameters']
    def last():return params()['entities'][-1]
    results=[]
    for family in ['slotGroups','arcSlotGroups','polygonGroups','chamferGroups']:
        call('document.new',{'discard':True});call('sketch.begin',{'plane':'XY','name':family})
        call('sketch.addCircle',{'centerX':-45,'centerY':-30,'diameter':12,'units':'mm'});circle=last()
        call('sketch.constraint',{'type':'fixed','entityIds':[circle['id']]})
        call('sketch.addPoint',{'x':0,'y':0,'units':'mm'});source=last()
        snap={'kind':'point','pointId':source['pointId']}
        if family=='slotGroups':
            call('sketch.centerPointSlot',{'centerX':0,'centerY':0,'endX':20,'endY':0,'width':10,'units':'mm','centerSnapTarget':snap})
            g=params()[family][0];target=g['sideIds'][0]
            call('sketch.dimension',{'kind':'linear','entityId':g['centerlineId'],'value':40,'units':'mm'})
        elif family=='arcSlotGroups':
            call('sketch.centerPointArcSlot',{'centerX':0,'centerY':0,'firstX':20,'firstY':0,'lastX':0,'lastY':20,'width':6,'units':'mm','centerSnapTarget':snap})
            g=params()[family][0];target=g['outerArcId']
            call('sketch.dimension',{'kind':'radius','entityId':g['pathArcId'],'value':20,'units':'mm'})
        elif family=='polygonGroups':
            # Both retained placement drivers follow external authored points.
            call('sketch.addPoint',{'x':15,'y':0,'units':'mm'});driver=last()
            call('sketch.polygon',{'mode':'inscribed','x':0,'y':0,'size':15,'count':6,'units':'mm','anchorSnapTarget':snap,'driverSnapTarget':{'kind':'point','pointId':driver['pointId']}})
            g=params()[family][0];target=g['entityIds'][0]
            # An external snapped line survives; its retired-target Coincident is removed.
            shared=next(q for q in params()['points'] if q['id']==g['pointIds'][0])
            call('sketch.line',{'x1':shared['uMm'],'y1':shared['vMm'],'x2':40,'y2':35,'units':'mm','startSnapTarget':{'kind':'point','pointId':shared['id']}});survivor=last()
        else:
            call('sketch.centerRectangle',{'centerX':0,'centerY':0,'width':60,'height':40,'units':'mm'})
            es=params()['entities'];lines=[e for e in es if e['type']=='line'];original_source=draft()
            call('sketch.chamfer',{'entityIds':[lines[1]['id'],lines[2]['id']],'value':5,'units':'mm'})
            call('sketch.chamfer',{'entityIds':[lines[2]['id'],lines[3]['id']],'value':7,'units':'mm'})
            g=params()[family][0];target=g['lineId'];adjacent=params()[family][1]
            # Source selection does not implicitly delete the owning Chamfer.
            before=session();call('sketch.deleteSelection',{'entityIds':[lines[2]['id']]},error='sketch_solve_failed');after=session()
            assert before['draftSketch']==after['draftSketch'] and before['draftHistory']==after['draftHistory']
        baseline=draft();index=session()['draftHistory']['index'];old=baseline['parameters']
        call('sketch.selectionSet',{'entityIds':[target]});call('capture.screenshot',{'path':str(out/(family+'-selected.png'))})
        call('sketch.deleteSelection');changed=draft();new=params()
        assert session()['draftHistory']['index']==index+1
        if family=='chamferGroups':
            assert new[family]==[adjacent] and new['entities']==old['entities'] and new['points']==old['points']
        else:assert family not in new
        assert circle in new['entities'] and source in new['entities']
        old_points={q['id']:q for q in old['points']};assert all(q==old_points[q['id']] for q in new['points'])
        if family=='polygonGroups':
            assert survivor in new['entities'] and driver in new['entities']
            owned=set(g['constraintIds']+[g['anchorConstraintId'],g['driverConstraintId']]);assert not any(c['id'] in owned for c in new['constraints'])
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
print('PASS: four constructor owners, source/shared identities, history, reopen and hardware GPU')
