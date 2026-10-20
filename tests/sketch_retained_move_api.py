#!/usr/bin/env python3
"""Real entrypoint: retained source/cache, source dimensions, ownership and STL."""
import argparse,json,math
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--native-evidence',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve();native=Path(a.native_evidence).resolve();receipt=json.loads((native/'native-report.json').read_text())
with application(a.app,out) as (call,log):
    gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated']
    call('document.open',{'path':str(native/'retained-profile.alloy')});initial=call('document.inspect');assert initial['authored']==receipt['authored'];sketch=next(f for f in initial['authored']['features'] if f['id']==receipt['sketchId']);assert sketch==receipt['final'];group=sketch['parameters']['moveGroups'][0]
    assert (group['deltaUMm'],group['deltaVMm'])==(7,-2)
    def verify(s,width):
        ps=s['parameters'];g=ps['moveGroups'][0];assert g['id']==group['id'] and ps['entities']==sketch['parameters']['entities']
        source={p['pointId']:p for p in g['sourcePoints']}
        for p in ps['points']:
            q=source[p['id']];assert math.isclose(p['uMm'],q['uMm']+g['deltaUMm'],abs_tol=1e-9) and math.isclose(p['vMm'],q['vMm']+g['deltaVMm'],abs_tol=1e-9)
        assert math.isclose(max(p['uMm'] for p in ps['points'])-min(p['uMm'] for p in ps['points']),width,abs_tol=1e-6)
    verify(sketch,20)
    mesh=inspect_stl(native/'retained-profile.stl',True);assert len(mesh['components'])==1 and mesh['eulerCharacteristic']==2 and math.isclose(mesh['signedVolume'],1920,abs_tol=1e-5) and mesh['bounds']==[[17.,6.,0.],[37.,18.,8.]]
    session=call('sketch.edit',{'sketchId':receipt['sketchId']})
    def draft():return call('command.preview',{'session':session['id']})['authoredDraft']
    baseline=draft();history=call('app.state')['activeSession']['draftHistory'];args={'groupId':group['id'],'x':5,'y':-3,'units':'mm'}
    call('sketch.moveEdit',{'groupId':group['id']});call('sketch.movePreview',args)
    for command,params in [('sketch.finish',{}),('command.commit',{'session':session['id']})]:call(command,params,error='pending_move')
    assert draft()==baseline and call('app.state')['activeSession']['draftHistory']==history
    call('sketch.moveCancel');assert draft()==baseline
    call('sketch.movePreview',dict(args,x=0,y=0));call('sketch.moveAccept');zero=draft();assert zero['parameters']['moveGroups'][0]['id']==group['id'];call('history.undo');assert draft()==baseline;call('history.redo');assert draft()==zero;call('history.undo')
    call('sketch.movePreview',dict(args,x=100001),error='invalid_arguments');assert draft()==baseline
    # Existing driving width is solved in pre-Move source space; displacement remains total.
    width=baseline['parameters']['dimensions'][0]['id'];call('sketch.dimensionUpdate',{'dimensionId':width,'value':24,'units':'mm'});sized=draft();verify(sized,24);g=sized['parameters']['moveGroups'][0];assert (g['deltaUMm'],g['deltaVMm'])==(7,-2)
    call('history.undo');assert draft()==baseline;call('history.redo');assert draft()==sized
    call('sketch.movePreview',{'entityIds':receipt['entityIds'],'x':1,'y':1},error='sketch_move_failed');assert draft()==sized
    call('sketch.finish');final=call('document.inspect');body=final['evaluation']['bodies'][0];assert math.isclose(body['volumeMm3'],2304,abs_tol=1e-6) and body['bodyId']==initial['evaluation']['bodies'][0]['bodyId'];assert final['evaluation']['sketches'][0]['profiles'][0]['profileId']==receipt['profileId']
    call('feature.edit',{'featureId':receipt['sketchId'],'parameters':{'points':sized['parameters']['points']}},error='retained_move_owned')
    call('document.save',{'path':str(out/'source-sized-retained-move.alloy')});call('document.open',{'path':str(out/'source-sized-retained-move.alloy')});assert call('document.inspect')['authored']==final['authored']
    call('document.exportStl',{'path':str(out/'source-sized-retained-move.stl'),'bodyId':body['bodyId'],'units':'mm','format':'binary','surfaceDeviationMm':.01,'normalDeviationDeg':5});sized_mesh=inspect_stl(out/'source-sized-retained-move.stl',True);assert len(sized_mesh['components'])==1 and sized_mesh['eulerCharacteristic']==2 and math.isclose(sized_mesh['signedVolume'],2304,abs_tol=1e-5) and sized_mesh['bounds']==[[17.,6.,0.],[41.,18.,8.]]
    call('view.home');call('capture.screenshot',{'path':str(out/'source-sized-solid.png')})
    (out/'report.json').write_text(json.dumps({'passed':True,'gpu':gpu,'authored':final['authored'],'mesh1920':mesh,'mesh2304':sized_mesh,'sourceSized':sized},indent=2));(out/'transcript.json').write_text(json.dumps(log,indent=2))
