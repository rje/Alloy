#!/usr/bin/env python3
"""Source-frame numeric resize through retained Rotate and independent printable readback."""
import argparse,copy,json,math
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--native-evidence',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve();native=Path(a.native_evidence).resolve();receipt=json.loads((native/'native-report.json').read_text())
def circles(a,ra,b,rb,seed):
    dx,dy=b[0]-a[0],b[1]-a[1];d=math.hypot(dx,dy);u,v=dx/d,dy/d;x=(ra*ra-rb*rb+d*d)/(2*d);h=math.sqrt(ra*ra-x*x);q=(a[0]+x*u,a[1]+x*v);c=[(q[0]-h*v,q[1]+h*u),(q[0]+h*v,q[1]-h*u)];return min(c,key=lambda p:math.dist(p,seed))
def analytic(length,deg=30,pivot=(6,5)):
    a=(12,6);b=(12+length*20/math.sqrt(416),6+length*4/math.sqrt(416));c=circles(a,math.sqrt(481),b,13,(27,22));d=circles(a,math.sqrt(153),c,math.sqrt(340),(9,18));source=[a,b,c,d];t=math.radians(deg);x,y=pivot;output=[(x+(u-x)*math.cos(t)-(v-y)*math.sin(t),y+(u-x)*math.sin(t)+(v-y)*math.cos(t)) for u,v in source];area=sum(source[i][0]*source[(i+1)%4][1]-source[(i+1)%4][0]*source[i][1] for i in range(4))/2;return source,output,area
with application(a.app,out) as (call,log):
    call('document.open',{'path':str(native/'legacy-before-size.alloy')});legacy=call('document.inspect');assert legacy['authored']==receipt['baselineDocument']['authored'];assert 'sourceFrame' not in legacy['authored']['features'][0]['parameters']['rotateGroups'][0];call('document.save',{'path':str(out/'legacy-roundtrip.alloy')});call('document.open',{'path':str(out/'legacy-roundtrip.alloy')});assert call('document.inspect')['authored']==legacy['authored']
    call('document.open',{'path':str(native/'resized-rotated-profile.alloy')});initial=call('document.inspect');assert initial['authored']==receipt['authored'];sid=receipt['sketchId'];dim=receipt['dimensionId'];base=receipt['pivot'];g=base['parameters']['rotateGroups'][0];frame=g['sourceFrame'];aid=g['axisLineId'];gid=g['id'];body=initial['evaluation']['bodies'][0]['bodyId'];profile=initial['evaluation']['sketches'][0]['profiles'][0]['profileId']
    def verify(feature,length,deg=30,pivot=(6,5)):
        pp=feature['parameters'];group=pp['rotateGroups'][0];source,wanted,area=analytic(length,deg,pivot);assert group['id']==gid and group['sourceFrame']==frame and group['angleDeg']==deg and group['axisLineId']==aid;assert pp['entities']==base['parameters']['entities'] and pp.get('constraints')==base['parameters'].get('constraints')
        for n,(s,o) in enumerate(zip(source,wanted)):
            for key,value in zip(('uMm','vMm'),s):assert math.isclose(group['sourcePoints'][n][key],value,abs_tol=1e-6),(group['sourcePoints'][n],s)
            for key,value in zip(('uMm','vMm'),o):assert math.isclose(pp['points'][n][key],value,abs_tol=1e-6),(pp['points'][n],o)
            assert pp['points'][n]['id']==base['parameters']['points'][n]['id']
        assert pp['dimensions'][0]['valueMm']==length and group['dimensions']==pp['dimensions'];assert 'source_orientation' not in json.dumps(feature);return area
    def mesh(path,length,deg=30,pivot=(6,5)):
        m=inspect_stl(path,True);s,o,area=analytic(length,deg,pivot);assert len(m['components'])==1 and m['triangles']==12 and m['eulerCharacteristic']==2 and math.isclose(m['signedVolume'],area*8,abs_tol=.001)
        wanted=[[min(p[0] for p in o),min(p[1] for p in o),0],[max(p[0] for p in o),max(p[1] for p in o),8]]
        for x,y in zip(sum(m['bounds'],[]),sum(wanted,[])):assert math.isclose(x,y,abs_tol=3e-6),(m,wanted)
        return m
    verify(receipt['sized'],22,pivot=(4,3));area22=verify(base,22);mesh22=mesh(native/'resized-rotated-profile.stl',22);assert math.isclose(area22,250.3818169699,abs_tol=1e-7)
    call('document.parameterAdd',{'name':'SourceSize','value':22,'units':'mm'});with_parameter=call('document.inspect');target=next(r['target'] for r in with_parameter['modelParameters'] if r.get('target',{}).get('dimensionId')==dim)
    for expression in ('SourceSize','23 mm'):
        call('document.modelParameterSet',{'target':target,'expression':expression},error='parameter_evaluation_failed');assert call('document.inspect')['authored']==with_parameter['authored']
    call('sketch.edit',{'sketchId':sid})
    def state():return call('app.state')['activeSession']
    def draft():return state()['draftSketch']
    baseline=draft();history=state()['draftHistory'];call('sketch.dimensionUpdate',{'dimensionId':dim,'value':23,'units':'mm'});sized=draft();verify(sized,23);call('history.undo');assert draft()==baseline;call('history.redo');assert draft()==sized
    h=state()['draftHistory']
    for args in ({'value':1},{'expression':'23 mm'},{'parameterName':'SourceSize'}):call('sketch.dimensionUpdate',dict({'dimensionId':dim,'units':'mm'},**args),error='sketch_solve_failed');assert draft()==sized and state()['draftHistory']==h
    # Strict snapshots/cache and owner data cannot be altered through a generic model edit.
    changed=copy.deepcopy(sized['parameters']['dimensions']);changed[0]['valueMm']=24;call('feature.edit',{'featureId':sid,'parameters':{'dimensions':changed}},error='evaluation_failed');assert draft()==sized and state()['draftHistory']==h
    call('sketch.rotatePreview',{'groupId':gid,'axisLineId':aid,'angleDeg':20});pending=state()['pendingSketchMove'];call('sketch.dimensionUpdate',{'dimensionId':dim,'value':24,'units':'mm'},error='pending_transform');assert draft()==sized and state()['draftHistory']==h and state()['pendingSketchMove']==pending;call('sketch.rotateCancel')
    call('sketch.moveStart');call('sketch.dimensionUpdate',{'dimensionId':dim,'value':24,'units':'mm'},error='pending_transform');assert draft()==sized and state()['draftHistory']==h;call('sketch.moveCancel')
    # Copy/Paste is sketch-scoped: capture an independent distant rectangle here.
    call('sketch.rectangle',{'x1':100,'y1':80,'x2':110,'y2':90,'units':'mm'});extra=[e['id'] for e in draft()['parameters']['entities'][4:]];call('sketch.copy',{'entityIds':extra});with_rectangle=draft();rect_history=state()['draftHistory'];call('sketch.movePreview',{'entityIds':extra,'createCopy':True,'x':20,'y':0,'units':'mm'},error='sketch_move_failed');assert draft()==with_rectangle and state()['draftHistory']==rect_history and not state().get('pendingSketchMove');call('sketch.moveCancel');call('history.undo');assert draft()==sized;h=state()['draftHistory'];call('sketch.paste');call('sketch.dimensionUpdate',{'dimensionId':dim,'value':24,'units':'mm'},error='pending_transform');assert draft()==sized and state()['draftHistory']==h;call('sketch.pasteCancel')
    call('sketch.rotatePreview',{'groupId':gid,'axisLineId':aid,'angleDeg':-15});verify(state()['pendingSketchMove']['draft'],23,-15);assert draft()==sized;call('sketch.rotateAccept');angle_edit=draft();verify(angle_edit,23,-15);call('sketch.3dMode',{'enabled':True});call('sketch.axis3d',{'axisId':aid,'start':{'u':4,'v':3,'w':0},'end':{'u':4,'v':3,'w':10},'units':'mm'});final_draft=draft();area23=verify(final_draft,23,-15,(4,3));assert final_draft['parameters']['rotateGroups']==angle_edit['parameters']['rotateGroups'];call('history.undo');assert draft()==angle_edit;call('history.redo');assert draft()==final_draft;call('sketch.finish');final=call('document.inspect');assert final['evaluation']['bodies'][0]['bodyId']==body and final['evaluation']['sketches'][0]['profiles'][0]['profileId']==profile and math.isclose(final['evaluation']['bodies'][0]['volumeMm3'],area23*8,abs_tol=1e-5)
    call('history.undo');assert call('document.inspect')['authored']==with_parameter['authored'];call('history.redo');assert call('document.inspect')['authored']==final['authored'];call('document.save',{'path':str(out/'source23-angle-minus15.alloy')});call('document.open',{'path':str(out/'source23-angle-minus15.alloy')});assert call('document.inspect')['authored']==final['authored'];call('document.exportStl',{'path':str(out/'source23-angle-minus15.stl'),'bodyId':body,'units':'mm','format':'binary','surfaceDeviationMm':.01,'normalDeviationDeg':5});mesh23=mesh(out/'source23-angle-minus15.stl',23,-15,(4,3));call('view.home');call('capture.screenshot',{'path':str(out/'source23-angle-minus15-solid.png')});gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated']
    (out/'report.json').write_text(json.dumps({'passed':True,'sourceFrame':frame,'area22Mm2':area22,'area23Mm2':area23,'mesh22':mesh22,'mesh23':mesh23,'final':final,'gpu':gpu},indent=2));(out/'transcript.json').write_text(json.dumps(log,indent=2))
