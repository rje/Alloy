#!/usr/bin/env python3
"""Independent retained body snapshot: real entrypoint, history and interchange."""
import argparse,hashlib,json,math,subprocess
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--step-tool',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
def volumes(state,expected):
    bodies=state['evaluation']['bodies'];assert len(bodies)==len(expected)
    for b,v in zip(bodies,expected):assert b['valid'] and b['closed'] and b['topology']['solids']==1 and math.isclose(b['volumeMm3'],v,abs_tol=1e-6),b
    return bodies
def bound(observed,expected):assert all(math.isclose(x,y,abs_tol=1e-5) for x,y in zip(observed,expected)),(observed,expected)
with application(a.app,out) as (call,transcript):
    gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated']
    call('body.paste',{'xMm':30},error='invalid_body_copy')
    call('sketch.begin',{'plane':'XY','name':'Source profile'});call('sketch.rectangle',{'x1':-10,'y1':-10,'x2':10,'y2':10,'units':'mm'});call('sketch.finish')
    sketch=call('document.inspect')['evaluation']['sketches'][0]
    extrude={'sketchId':sketch['featureId'],'profileId':sketch['profiles'][0]['profileId'],'distance':10,'units':'mm','operation':'newBody','name':'Source'}
    call('solid.extrude',extrude);source=call('document.inspect');source_id=source['evaluation']['bodies'][0]['bodyId'];source_feature=source['authored']['features'][-1]['id']
    frozen=call('body.copySelection',{'bodyId':source_id});assert call('document.inspect')['authored']==source['authored']
    # Freeze time is Copy, even when the source changes before Paste begins.
    call('solid.extrude',{**extrude,'featureId':source_feature,'distance':14});volumes(call('document.inspect'),[5600])
    timing=call('body.pasteDialog',{'xMm':30});assert math.isclose(timing['geometry']['volumeMm3'],4000,abs_tol=1e-6);call('command.cancel',{'session':timing['id']})
    call('history.undo');assert call('document.inspect')['authored']==source['authored']
    source=call('document.inspect')
    session=call('body.pasteDialog',{'xMm':30,'name':'Snapshot'});assert session['previewed'];assert math.isclose(session['geometry']['volumeMm3'],4000,abs_tol=1e-6)
    call('view.home');call('capture.screenshot',{'path':str(out/'copy-preview.png')});call('capture.dialog',{'path':str(out/'copy-panel.png')})
    call('command.cancel',{'session':session['id']});cancel=call('document.inspect');assert cancel['authored']==source['authored'] and cancel['undo']==source['undo']
    session=call('body.pasteDialog',{'xMm':30,'yMm':0,'zMm':0,'name':'Snapshot'});copy_id=session['newBodyId'];copy_feature=session['newFeatureId'];call('command.commit',{'session':session['id']})
    copied=call('document.inspect');bodies=volumes(copied,[4000,4000]);assert bodies[1]['bodyId']==copy_id and copy_id!=source_id and copy_feature!=source_feature
    assert copied['authored']['features'][-1]['id']==copy_feature and copied['authored']['features'][-1]['type']=='bodyCopy';snapshot=copied['authored']['features'][-1]['parameters']['snapshot'];assert snapshot['sha256']==frozen['snapshotSha256']
    bound(bodies[1]['boundsMm']['min'],[20,-10,0]);bound(bodies[1]['boundsMm']['max'],[40,10,10]);assert copied['undo']['index']==source['undo']['index']+1
    call('history.undo');assert call('document.inspect')['authored']==source['authored'];call('history.redo');assert call('document.inspect')['authored']==copied['authored']
    call('solid.extrude',{**extrude,'featureId':source_feature,'distance':14});edited=call('document.inspect');volumes(edited,[5600,4000]);assert edited['authored']['features'][-1]==copied['authored']['features'][-1]
    call('history.undo');assert call('document.inspect')['authored']==copied['authored'];call('history.redo');assert call('document.inspect')['authored']==edited['authored']
    # Copied-face support is deliberately unavailable. Target a solid from an XY circle instead.
    call('sketch.begin',{'plane':'XY','name':'Independent copy hole'});call('sketch.addCircle',{'centerX':30,'centerY':0,'diameter':4,'units':'mm'});call('sketch.finish');hole=call('document.inspect')['evaluation']['sketches'][-1]
    pre_cut=call('document.inspect');call('solid.extrude',{'sketchId':hole['featureId'],'profileId':hole['profiles'][0]['profileId'],'distance':20,'units':'mm','operation':'cut','targetBodyIds':[copy_id],'name':'Snapshot Cut'})
    final=call('document.inspect');expected=[5600,4000-40*math.pi];bodies=volumes(final,expected);bound(bodies[0]['boundsMm']['max'],[10,10,14]);bound(bodies[1]['boundsMm']['max'],[40,10,10]);bound(bodies[1]['boundsMm']['min'],[20,-10,0])
    call('history.undo');assert call('document.inspect')['authored']==pre_cut['authored'];volumes(call('document.inspect'),[5600,4000]);call('history.redo');assert call('document.inspect')['authored']==final['authored']
    # A later independent Move is separately editable/undoable and never affects Source.
    call('solid.move',{'targetBodyId':copy_id,'zMm':5});moved=call('document.inspect');volumes(moved,expected);bound(moved['evaluation']['bodies'][1]['boundsMm']['min'],[20,-10,5]);bound(moved['evaluation']['bodies'][0]['boundsMm']['min'],[-10,-10,0]);call('history.undo');assert call('document.inspect')['authored']==final['authored'];call('history.redo');assert call('document.inspect')['authored']==moved['authored'];call('history.undo')
    saved=out/'independent-body-copy.alloy';call('document.save',{'path':str(saved)});call('document.new',{'discard':True});call('body.paste',{'xMm':30},error='invalid_body_copy');call('document.open',{'path':str(saved),'discard':True});reopened=call('document.inspect');assert reopened['authored']==final['authored'];volumes(reopened,expected)
    call('view.home');call('selection.set',{'bodyId':copy_id});call('capture.screenshot',{'path':str(out/'independent-solids.png')})
    exports=[]
    for name,selection,indices in [('source',{'bodyId':source_id},[0]),('snapshot',{'bodyId':copy_id},[1]),('all-visible',{'allVisible':True},[0,1])]:
        before=call('document.inspect');step=out/(name+'.step');stl=out/(name+'.stl')
        call('document.exportStep',{'path':str(step),'units':'mm',**selection});result=subprocess.run([str(Path(a.step_tool).resolve()),'read-all',str(step)],check=True,capture_output=True,text=True);read=json.loads(result.stdout.splitlines()[-1]);assert read['solidCount']==len(indices);step_bodies=sorted(read['solids'],key=lambda b:b['boundsMm'][0])
        call('document.exportStl',{'path':str(stl),'units':'mm','format':'binary','surfaceDeviationMm':0.05,'normalDeviationDeg':5,**selection});mesh=inspect_stl(stl,True);mesh_bodies=sorted(mesh['components'],key=lambda b:b['bounds'][0][0]);assert len(mesh_bodies)==len(indices)
        for slot,i in enumerate(indices):
            b=bodies[i];s=step_bodies[slot];m=mesh_bodies[slot];assert s['valid'];assert math.isclose(s['volumeMm3'],expected[i],abs_tol=1e-5);bound(s['boundsMm'],b['boundsMm']['min']+b['boundsMm']['max']);bound(m['bounds'][0],b['boundsMm']['min']);bound(m['bounds'][1],b['boundsMm']['max']);assert math.isclose(m['signedVolume'],expected[i],rel_tol=.001)
        after=call('document.inspect');assert before['authored']==after['authored'] and before['undo']==after['undo'];exports.append({'name':name,'step':read,'stl':mesh})
    (out/'receipt.json').write_text(json.dumps({'passed':True,'gpu':gpu,'source':source,'copied':copied,'final':final,'reopened':reopened,'exports':exports,'transcript':transcript},indent=2)+'\n')
print('PASS body snapshot: source isolation, independent Cut/Move, history/reopen and selected/all independent STEP/STL readback')
