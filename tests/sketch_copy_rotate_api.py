#!/usr/bin/env python3
"""Copy/Paste and Create Copy of a rectangle beside a retained Rotate."""
import json, math, uuid
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl
import argparse
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
xy=[(12,6),(32,10),(27,22),(9,18)]
with application(a.app,out) as (call,log):
    def state():return call('app.state')['activeSession']
    def draft():return state()['draftSketch']
    call('sketch.begin',{'plane':'XY','name':'Copy beside retained Rotate'})
    for i,(u,v) in enumerate(xy):call('sketch.line',{'x1':u,'y1':v,'x2':xy[(i+1)%4][0],'y2':xy[(i+1)%4][1],'units':'mm'})
    p0=draft()['parameters']['points']
    for i in range(5):
        x=i if i<4 else 0;y=(i+1)%4 if i<4 else 2
        call('sketch.dimension',{'kind':'linear','pointIds':[p0[x]['id'],p0[y]['id']],'value':math.dist(xy[x],xy[y]),'units':'mm','driving':True})
    sourceEntities=[e['id'] for e in draft()['parameters']['entities']]
    call('sketch.3dMode',{'enabled':True});axis=call('sketch.axis3d',{'start':{'u':4,'v':3,'w':0},'end':{'u':4,'v':3,'w':10},'units':'mm'})['axis']
    call('sketch.rotatePreview',{'entityIds':sourceEntities,'axisLineId':axis['id'],'angleDeg':30});call('sketch.rotateAccept');rotated=draft();rotateGroup=rotated['parameters']['rotateGroups'][0]
    call('sketch.rectangle',{'x1':80,'y1':8,'x2':100,'y2':20,'units':'mm'});rectIds=[e['id'] for e in draft()['parameters']['entities'][4:]]
    original=draft();h=state()['draftHistory'];authored=call('document.inspect')['authored'];
    call('sketch.copy',{'entityIds':rectIds});call('sketch.paste');call('sketch.pastePreview',{'x':35,'y':0,'units':'mm'});pastePending=state()['pendingSketchPaste'];assert len(pastePending['draft']['parameters']['entities'])==12;call('sketch.pasteAccept');pasted=draft();assert len(pasted['parameters']['entities'])==12 and pasted['parameters']['rotateGroups']==original['parameters']['rotateGroups'];assert pasted['parameters']['points'][:8]==original['parameters']['points']
    pastedIds=[e['id'] for e in pasted['parameters']['entities'][8:12]];assert not set(pastedIds)&set(rectIds)
    call('history.undo');assert draft()==original;call('history.redo');assert draft()==pasted
    # Create Copy uses the shared Move/Copy command path and leaves the clipboard untouched.
    clipboard=call('sketch.clipboardInspect')['clipboard'];call('solid.moveDialog');preview=call('sketch.movePreview',{'entityIds':rectIds,'createCopy':True,'x':65,'y':0,'units':'mm'});candidate=preview['draft'];assert preview['createCopy'] and candidate['parameters']['rotateGroups']==original['parameters']['rotateGroups'];assert candidate['parameters']['points'][:12]==pasted['parameters']['points']
    createdIds=[e['id'] for e in candidate['parameters']['entities'][12:16]];assert not set(createdIds)&(set(rectIds)|set(pastedIds));assert call('sketch.clipboardInspect')['clipboard']==clipboard;call('sketch.moveAccept');created=draft();assert len(created['parameters']['entities'])==16
    call('history.undo');assert draft()==pasted;call('history.redo');assert draft()==created
    # Edit each independent definition and prove exact preservation in the other.
    rotate=call('sketch.rotatePreview',{'groupId':rotateGroup['id'],'axisLineId':axis['id'],'angleDeg':-15});call('sketch.rotateAccept');angleEdited=draft();assert angleEdited['parameters']['points'][4:]==created['parameters']['points'][4:]
    call('solid.moveDialog');call('sketch.movePreview',{'entityIds':pastedIds,'x':3,'y':2,'units':'mm'});call('sketch.moveAccept');copyEdited=draft();assert copyEdited['parameters']['rotateGroups']==angleEdited['parameters']['rotateGroups'];assert copyEdited['parameters']['points'][:8]==angleEdited['parameters']['points'][:8]
    call('history.undo');assert draft()==angleEdited;call('history.redo');assert draft()==copyEdited
    sid=draft()['id'];call('sketch.finish');inspection=call('document.inspect');profiles=inspection['evaluation']['sketches'][0]['profiles'];assert len(profiles)==4
    for profile in profiles:call('solid.extrude',{'sketchId':sid,'profileId':profile['profileId'],'distance':8,'units':'mm'})
    inspection=call('document.inspect')
    call('document.save',{'path':str(out/'coexistence.alloy')});call('document.open',{'path':str(out/'coexistence.alloy')});reopened=call('document.inspect');assert reopened['authored']==inspection['authored'];assert [p['profileId'] for p in reopened['evaluation']['sketches'][0]['profiles']]==[p['profileId'] for p in inspection['evaluation']['sketches'][0]['profiles']];assert [(b['bodyId'],b['volumeMm3']) for b in reopened['evaluation']['bodies']]==[(b['bodyId'],b['volumeMm3']) for b in inspection['evaluation']['bodies']]
    bodies=reopened['evaluation']['bodies'];assert len(bodies)==4;bodyIds=[b['bodyId'] for b in bodies];area=abs(sum(xy[i][0]*xy[(i+1)%4][1]-xy[(i+1)%4][0]*xy[i][1] for i in range(4)))/2;assert math.isclose(sum(b['volumeMm3'] for b in bodies),8*(area+3*240),abs_tol=1e-5),(area,[b['volumeMm3'] for b in bodies])
    call('document.exportStl',{'path':str(out/'coexistence.stl'),'bodyIds':bodyIds,'units':'mm','format':'binary','surfaceDeviationMm':.01,'normalDeviationDeg':5});mesh=inspect_stl(out/'coexistence.stl',True);assert len(mesh['components'])==4 and mesh['eulerCharacteristic']==8 and math.isclose(mesh['signedVolume'],8*(area+3*240),abs_tol=.002)
    call('capture.screenshot',{'path':str(out/'coexistence.png')});gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated']
    (out/'report.json').write_text(json.dumps({'passed':True,'initial':authored,'original':original,'pasted':pasted,'created':created,'angleEdited':angleEdited,'copyEdited':copyEdited,'final':inspection,'mesh':mesh,'gpu':gpu,'transcript':log},indent=2))
