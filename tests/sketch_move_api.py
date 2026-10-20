#!/usr/bin/env python3
"""Read the native moved profile, exercise local transactions, and inspect STL."""
import argparse,json,math
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--native-evidence',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve();native=Path(a.native_evidence).resolve();receipt=json.loads((native/'native-report.json').read_text())
with application(a.app,out) as (call,log):
    gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated']
    call('document.open',{'path':str(native/'moved-profile.alloy')});initial=call('document.inspect');assert initial['authored']==receipt['authored'];sketch=next(f for f in initial['authored']['features'] if f['id']==receipt['sketchId']);assert sketch==receipt['final']
    points=sketch['parameters']['points'];assert [min(p[k] for p in points) for k in ('uMm','vMm')]==[17,6];assert [max(p[k] for p in points) for k in ('uMm','vMm')]==[37,18]
    for key in ('entities','constraints','dimensions'):assert sketch['parameters'][key]==receipt['baseline']['parameters'][key]
    body=initial['evaluation']['bodies'][0];assert math.isclose(body['volumeMm3'],1920,abs_tol=1e-6);assert initial['evaluation']['sketches'][0]['profiles'][0]['profileId']==receipt['profileId']
    mesh=inspect_stl(native/'moved-profile.stl',True);assert len(mesh['components'])==1 and mesh['eulerCharacteristic']==2 and math.isclose(mesh['signedVolume'],1920,abs_tol=1e-5);assert mesh['bounds']==[[17.,6.,0.],[37.,18.,8.]]
    session=call('sketch.edit',{'sketchId':receipt['sketchId']})
    def draft():return call('command.preview',{'session':session['id']})['authoredDraft']
    baseline=draft();history=call('app.state')['activeSession']['draftHistory'];args={'entityIds':receipt['entityIds'],'x':-2,'y':-1,'units':'mm'}
    preview=call('sketch.movePreview',args);call('sketch.finish',error='pending_move');pending=call('app.state')['activeSession']['pendingSketchMove'];call('sketch.lookAt');assert call('app.state')['activeSession']['pendingSketchMove']==pending;assert not preview['authoredChange'] and not preview['retained'];assert draft()==baseline and call('app.state')['activeSession']['draftHistory']==history;call('sketch.moveCancel');assert draft()==baseline
    call('sketch.movePreview',args);call('sketch.moveAccept');first=draft();assert first==receipt['first'];call('history.undo');assert draft()==baseline;call('history.redo');assert draft()==first;call('sketch.cancel');assert call('document.inspect')['authored']==initial['authored']
    session=call('sketch.edit',{'sketchId':receipt['sketchId']});baseline=draft();history=call('app.state')['activeSession']['draftHistory']
    for bad in ({'x':0,'y':0},{'entityIds':receipt['entityIds'][:3]},{'entityIds':receipt['entityIds'][:3]+['foreign-projection'] } ):
        call('sketch.movePreview',dict(args,**bad),error='sketch_move_failed');assert draft()==baseline and call('app.state')['activeSession']['draftHistory']==history
    # A free unselected anchor may not follow the moved profile through Coincident.
    call('sketch.addPoint',{'x':17,'y':6,'units':'mm'});anchor=draft()['parameters']['entities'][-1]['pointId'];vertex=sketch['parameters']['entities'][0]['startPointId'];call('sketch.constraint',{'type':'coincident','pointIds':[vertex,anchor]});anchored=draft();history=call('app.state')['activeSession']['draftHistory']
    call('sketch.movePreview',dict(args,x=1,y=1),error='sketch_move_failed');assert draft()==anchored and call('app.state')['activeSession']['draftHistory']==history;call('sketch.cancel');assert call('document.inspect')['authored']==initial['authored']
    call('document.save',{'path':str(out/'reopened-move.alloy')});call('document.open',{'path':str(out/'reopened-move.alloy')});assert call('document.inspect')['authored']==initial['authored'];call('view.home');call('capture.screenshot',{'path':str(out/'moved-profile-solid.png')})
    (out/'report.json').write_text(json.dumps({'passed':True,'authored':initial['authored'],'gpu':gpu,'volumeMm3':body['volumeMm3'],'mesh':mesh,'boundsMm':mesh['bounds'],'profileId':receipt['profileId']},indent=2));(out/'transcript.json').write_text(json.dumps(log,indent=2))
