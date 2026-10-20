#!/usr/bin/env python3
"""Real Copy/Paste independence, refusal, history and two-solid STL readback."""
import argparse,json,math
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--native-evidence',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve();native=Path(a.native_evidence).resolve();receipt=json.loads((native/'native-report.json').read_text())
with application(a.app,out) as (call,log):
    call('document.open',{'path':str(native/'copied-profiles.alloy')});initial=call('document.inspect');assert initial['authored']==receipt['authored'];sketch=receipt['final'];source_ids=receipt['sourceEntityIds'];copy_ids=[e['id'] for e in sketch['parameters']['entities'][4:]];sid=receipt['sketchId']
    def bounds(points):return [[min(p[k] for p in points) for k in ('uMm','vMm')],[max(p[k] for p in points) for k in ('uMm','vMm')]]
    assert bounds(sketch['parameters']['points'][:4])==[[10,8],[30,20]] and bounds(sketch['parameters']['points'][4:])==[[45,8],[65,20]]
    for key in ('points','entities','constraints','dimensions'):
        before=receipt['source']['parameters'][key];now=sketch['parameters'][key];assert now[:len(before)]==before
        assert len({r['id'] for r in now})==len(now) and len(now)==2*len(before)
    copied_targets={r['id'] for key in ('points','entities') for r in sketch['parameters'][key][4:]}
    for c in sketch['parameters']['constraints'][4:]:assert set(c.get('pointIds',[])+c.get('entityIds',[]))<=copied_targets
    for d in sketch['parameters']['dimensions'][2:]:assert set(d['pointIds'])<=copied_targets
    mesh=inspect_stl(native/'copied-profiles.stl',True);assert len(mesh['components'])==2 and mesh['eulerCharacteristic']==4 and math.isclose(mesh['signedVolume'],3840,abs_tol=1e-5) and mesh['bounds']==[[10.,8.,0.],[65.,20.,8.]]
    session=call('sketch.edit',{'sketchId':sid})
    def state():return call('app.state')['activeSession']
    def draft():return state()['draftSketch']
    baseline=draft();width=baseline['parameters']['dimensions'][0]['id'];call('sketch.dimensionUpdate',{'dimensionId':width,'value':24,'units':'mm'});sized=draft()
    for key,n in [('points',4),('entities',4),('constraints',4),('dimensions',2)]:assert sized['parameters'][key][n:]==baseline['parameters'][key][n:]
    sb=bounds(sized['parameters']['points'][:4]);assert math.isclose(sb[1][0]-sb[0][0],24,abs_tol=1e-6) and math.isclose(sb[1][1]-sb[0][1],12,abs_tol=1e-6)
    call('history.undo');assert draft()==baseline;call('history.redo');assert draft()==sized
    call('sketch.movePreview',{'entityIds':copy_ids,'x':7,'y':-2,'units':'mm'});call('sketch.moveAccept');moved=draft();assert bounds(moved['parameters']['points'][4:])==[[52,6],[72,18]];assert moved['parameters']['points'][:4]==sized['parameters']['points'][:4]
    call('history.undo');assert draft()==sized;call('history.redo');assert draft()==moved;call('sketch.finish');final=call('document.inspect');assert [p['profileId'] for p in final['evaluation']['sketches'][0]['profiles']]==[p['profileId'] for p in initial['evaluation']['sketches'][0]['profiles']];assert [b['bodyId'] for b in final['evaluation']['bodies']]==[b['bodyId'] for b in initial['evaluation']['bodies']];assert math.isclose(sum(b['volumeMm3'] for b in final['evaluation']['bodies']),4224,abs_tol=1e-6)
    call('history.undo');assert call('document.inspect')['authored']==initial['authored'];call('history.redo');assert call('document.inspect')['authored']==final['authored']
    call('document.save',{'path':str(out/'edited-independent-copy.alloy')});call('document.open',{'path':str(out/'edited-independent-copy.alloy')});assert call('document.inspect')['authored']==final['authored'];body_ids=[b['bodyId'] for b in final['evaluation']['bodies']];call('document.exportStl',{'path':str(out/'edited-independent-copy.stl'),'bodyIds':body_ids,'units':'mm','format':'binary','surfaceDeviationMm':.01,'normalDeviationDeg':5});edited_mesh=inspect_stl(out/'edited-independent-copy.stl',True);assert len(edited_mesh['components'])==2 and edited_mesh['eulerCharacteristic']==4 and math.isclose(edited_mesh['signedVolume'],4224,abs_tol=1e-5)
    call('view.home');call('capture.screenshot',{'path':str(out/'edited-independent-solids.png')});gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated']
    # Fresh source-only sketch: shared command lifecycle and atomic refusals.
    call('document.new',{'discard':True});session=call('sketch.begin',{'plane':'XY'});call('sketch.rectangle',{'x1':10,'y1':8,'x2':30,'y2':20,'units':'mm'});source=draft();ids=[e['id'] for e in source['parameters']['entities']];h=state()['draftHistory'];committed=call('document.inspect')['authored']
    call('sketch.copy',{'entityIds':ids[:3]},error='sketch_copy_failed');assert draft()==source and state()['draftHistory']==h
    call('sketch.copy',{'entityIds':ids});call('sketch.paste');call('sketch.pastePreview',{'x':35,'y':0,'units':'mm'});candidate=state()['pendingSketchPaste'];call('sketch.addPoint',{'x':100,'y':100,'units':'mm'},error='pending_paste');assert state()['pendingSketchPaste']==candidate and draft()==source;call('sketch.lookAt');assert state()['pendingSketchPaste']==candidate
    for command,args in [('sketch.finish',{}),('command.commit',{'session':session['id']})]:call(command,args,error='pending_paste')
    for x in (0,10,20):call('sketch.pastePreview',{'x':x,'y':0,'units':'mm'},error='sketch_copy_failed');assert draft()==source and state()['draftHistory']==h and 'pendingSketchPaste' not in state()
    call('sketch.finish',error='pending_paste');call('command.commit',{'session':session['id']},error='pending_paste');call('sketch.pasteAccept',error='paste_preview_required');call('sketch.pasteCancel');assert draft()==source and call('document.inspect')['authored']==committed
    call('sketch.paste');call('sketch.pastePreview',{'x':35,'y':0});call('command.cancel',{'session':session['id']});assert not call('app.state')['activeSession'] and call('document.inspect')['authored']==committed
    # An external relation is refused rather than omitted from a cloned constraint list.
    session=call('sketch.begin',{'plane':'XY'});call('sketch.rectangle',{'x1':10,'y1':8,'x2':30,'y2':20,'units':'mm'});s=draft();ids=[e['id'] for e in s['parameters']['entities']];call('sketch.addPoint',{'x':10,'y':8,'units':'mm'});anchor=draft()['parameters']['entities'][-1]['pointId'];call('sketch.constraint',{'type':'coincident','pointIds':[s['parameters']['points'][0]['id'],anchor]});linked=draft();h=state()['draftHistory'];call('sketch.copy',{'entityIds':ids},error='sketch_copy_failed');assert draft()==linked and state()['draftHistory']==h;call('sketch.cancel')
    (out/'report.json').write_text(json.dumps({'passed':True,'nativeMesh':mesh,'editedMesh':edited_mesh,'sourceSizedBounds':sb,'final':final,'gpu':gpu,'copyIds':copy_ids},indent=2));(out/'transcript.json').write_text(json.dumps(log,indent=2))
