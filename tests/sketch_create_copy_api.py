#!/usr/bin/env python3
"""Move/Copy Create Copy controller shares independent geometry without clipboard writes."""
import argparse,json
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,log):
    def state():return call('app.state')['activeSession']
    def draft():return state()['draftSketch']
    call('sketch.begin',{'plane':'XY'});call('sketch.rectangle',{'x1':-20,'y1':0,'x2':-10,'y2':5,'units':'mm'});seed=[r['id'] for r in draft()['parameters']['entities']];call('sketch.copy',{'entityIds':seed});call('sketch.cancel');clipboard=call('sketch.clipboardInspect')['clipboard']
    session=call('sketch.begin',{'plane':'XY'});call('sketch.rectangle',{'x1':10,'y1':8,'x2':30,'y2':20,'units':'mm'});ids=[r['id'] for r in draft()['parameters']['entities']];source=draft();history=state()['draftHistory'];authored=call('document.inspect')['authored'];call('solid.moveDialog');assert state()['moveInteraction']['active']
    args={'entityIds':ids,'createCopy':True,'x':35,'y':0,'units':'mm'}
    first=call('sketch.movePreview',args);assert first['createCopy'] and not first['retained'] and len(first['draft']['parameters']['entities'])==8 and 'moveGroups' not in first['draft']['parameters'];identity=first['diagnostics']['identityMap'];candidate=first['draft']
    for x in (40,35):assert call('sketch.movePreview',dict(args,x=x))['diagnostics']['identityMap']==identity
    assert draft()==source and state()['draftHistory']==history and call('sketch.clipboardInspect')['clipboard']==clipboard
    pending=state()['pendingSketchMove'];call('sketch.lookAt');assert state()['pendingSketchMove']==pending
    for command,params in [('sketch.finish',{}),('command.commit',{'session':session['id']})]:call(command,params,error='pending_move')
    atomic=call('sketch.movePreview',dict(args,createCopy=False));assert not atomic['createCopy'] and len(atomic['draft']['parameters']['entities'])==4 and not state().get('moveCopyCapture')
    for bad in ({'retain':True},{'groupId':''}):call('sketch.movePreview',dict(args,**bad),error='incompatible_move_modes');assert 'pendingSketchMove' not in state() and not state().get('moveCopyCapture')
    for bad in ({'entityIds':[]},{'entityIds':ids[:3]}):call('sketch.movePreview',dict(args,**bad),error='sketch_move_failed');assert draft()==source and state()['draftHistory']==history
    call('sketch.movePreview',{'createCopy':True,'x':35,'y':0},error='sketch_move_failed')
    call('sketch.movePreview',dict(args,x=0),error='sketch_move_failed');assert 'pendingSketchMove' not in state();capture=state()['moveCopyCapture']['copyId'];valid=call('sketch.movePreview',args);assert state()['moveCopyCapture']['copyId']==capture and valid['diagnostics']['identityMap']!=identity
    call('sketch.moveCancel');assert draft()==source and state()['draftHistory']==history and not state().get('moveCopyCapture');call('solid.moveDialog');assert state()['moveInteraction']['active'] and not state()['moveInteraction']['createCopy']
    call('sketch.movePreview',args);call('sketch.moveAccept');accepted=draft();assert len(accepted['parameters']['entities'])==8 and 'moveGroups' not in accepted['parameters'];assert call('sketch.clipboardInspect')['clipboard']==clipboard
    call('history.undo');assert draft()==source;call('history.redo');assert draft()==accepted;call('solid.moveDialog');assert state()['moveInteraction']['active'] and not state()['moveInteraction']['createCopy'];call('sketch.moveCancel')
    # Retained-owned outputs cannot become an independent copied source.
    call('sketch.movePreview',{'entityIds':ids,'retain':True,'x':5,'y':-3});call('sketch.moveAccept');owned=draft();h=state()['draftHistory'];call('sketch.movePreview',args,error='sketch_move_failed');assert draft()==owned and state()['draftHistory']==h;call('sketch.moveCancel');call('command.cancel',{'session':session['id']});assert not call('app.state')['activeSession'] and call('document.inspect')['authored']==authored
    # External constraints are refused, never silently discarded.
    session=call('sketch.begin',{'plane':'XY'});call('sketch.rectangle',{'x1':10,'y1':8,'x2':30,'y2':20,'units':'mm'});s=draft();ids=[r['id'] for r in s['parameters']['entities']];call('sketch.addPoint',{'x':10,'y':8,'units':'mm'});anchor=draft()['parameters']['entities'][-1]['pointId'];call('sketch.constraint',{'type':'coincident','pointIds':[s['parameters']['points'][0]['id'],anchor]});linked=draft();h=state()['draftHistory'];call('sketch.movePreview',dict(args,entityIds=ids),error='sketch_move_failed');assert draft()==linked and state()['draftHistory']==h;call('sketch.cancel');assert call('sketch.clipboardInspect')['clipboard']==clipboard
    (out/'report.json').write_text(json.dumps({'passed':True,'clipboardUnchanged':clipboard,'firstPreview':candidate,'accepted':accepted,'identityMap':identity},indent=2));(out/'transcript.json').write_text(json.dumps(log,indent=2))
