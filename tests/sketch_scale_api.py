#!/usr/bin/env python3
"""Retained uniform sketch Scale: authority/lifecycle and independent STL readback."""
import argparse,copy,json,math
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--native-evidence',required=True);p.add_argument('--output',required=True);a=p.parse_args();native=Path(a.native_evidence).resolve();out=Path(a.output).resolve();receipt=json.loads((native/'native-report.json').read_text());xy=[(10,10),(30,10),(27,20),(12,24)]
def verify(f,s=1.5,cx=27,pivot=(4,6)):
    pp=f['parameters'];g=pp['scaleGroups'][0];assert g['factor']==s and g['entityIds']==receipt['entityIds'] and g['pivotPointId']==receipt['pivotPointId']
    for i,(u,v) in enumerate(xy):
        if i==2:u=cx
        src=g['sourcePoints'][i];pt=pp['points'][i];assert src['pointId']==pt['id']==receipt['source']['parameters']['points'][i]['id'];assert src['uMm']==u and src['vMm']==v
        assert math.isclose(pt['uMm'],pivot[0]+s*(u-pivot[0]),abs_tol=1e-6) and math.isclose(pt['vMm'],pivot[1]+s*(v-pivot[1]),abs_tol=1e-6)
    for k in ('entities','constraints','dimensions'):assert pp.get(k)==receipt['source']['parameters'].get(k)
    return g

def mesh(path,volume):
    m=inspect_stl(path,True);assert len(m['components'])==1 and m['triangles']==12 and m['eulerCharacteristic']==2 and math.isclose(m['signedVolume'],volume,abs_tol=.001);assert m['bounds']==[[13.,12.,0.],[43.,33.,8.]];return m
with application(a.app,out) as (call,log):
    commands=call('commands.list');assert all(any(x['id']==name for x in commands['commands']) for name in ['sketch.scaleStart','sketch.scalePick','sketch.scalePreview','sketch.scaleAccept','sketch.scaleCancel','sketch.scaleEdit'])
    g=verify(receipt['factor15']);verify(receipt['factor2'],2);verify(receipt['sourceEdited'],1.5,25);mesh15=mesh(native/'scaled.stl',3762);mesh_edit=mesh(native/'source-edited-scale.stl',3510)
    call('document.open',{'path':str(native/'source-edited-scale.alloy')});initial=call('document.inspect');assert initial['authored']==receipt['authored'];sid=receipt['sketchId'];ids=receipt['entityIds'];pivot=receipt['pivotPointId'];gid=g['id'];point_ids=[r['id'] for r in receipt['source']['parameters']['points'][:4]]
    for key in ('points','scaleGroups'):
        call('feature.edit',{'featureId':sid,'parameters':{key:[]}},error='scale_owned');assert call('document.inspect')['authored']==initial['authored']
    call('sketch.edit',{'sketchId':sid})
    def state():return call('app.state')['activeSession']
    def draft():return state()['draftSketch']
    baseline=draft();h=state()['draftHistory'];call('sketch.scaleEdit',{'groupId':gid});candidate=state()['pendingSketchScale'];call('sketch.finish',error='pending_scale');call('command.commit',{'session':state()['id']},error='pending_scale');assert draft()==baseline and state()['draftHistory']==h and state()['pendingSketchScale']==candidate
    call('sketch.moveStart',error='pending_scale');call('sketch.rotateStart',error='pending_scale');call('sketch.addPoint',{'x':100,'y':100,'units':'mm'},error='pending_scale');assert state()['pendingSketchScale']==candidate and draft()==baseline
    for command,params in [('sketch.patternStart',{}),('sketch.circularPatternStart',{}),('sketch.patternEdit',{'groupId':gid}),('sketch.patternPreview',{'sourceEntityId':ids[0],'type':'rectangular'}),('sketch.patternPreview',{'sourceEntityId':ids[0],'type':'circular'}),('sketch.patternAccept',{}),('sketch.patternCancel',{})]:
        pattern_before=state().get('patternInteraction');call(command,params,error='pending_scale');after=state();assert after['pendingSketchScale']==candidate and after['draftSketch']==baseline and after['draftHistory']==h and after.get('patternInteraction')==pattern_before and not after.get('pendingSketchPattern')
    call('sketch.scaleCancel');assert draft()==baseline and 'scaleInteraction' not in state() and 'pendingSketchScale' not in state();call('sketch.scaleStart');call('sketch.scaleCancel')
    args={'groupId':gid,'pivotPointId':pivot,'factor':2};call('sketch.scalePreview',args);pending=state()['pendingSketchScale'];verify(pending['draft'],2,25);assert draft()==baseline and state()['draftHistory']==h
    call('sketch.scalePreview',dict(args,factor=1.5));assert state()['pendingSketchScale']['groupId']==gid
    for value in (0,-1,100001):
        before=state()['pendingSketchScale'];call('sketch.scalePreview',dict(args,factor=value),error='invalid_arguments');assert draft()==baseline and state()['draftHistory']==h and state()['pendingSketchScale']==before
    call('sketch.scalePreview',dict(args,pivotPointId=point_ids[0]),error='sketch_scale_failed');assert draft()==baseline and state()['draftHistory']==h and 'pendingSketchScale' not in state()
    preview=call('sketch.scalePreview',args);assert preview['status']=='pending' and preview['groupId']==gid;accepted=call('sketch.scaleAccept');assert accepted['status']=='committed' and accepted['groupId']==gid;second=draft();verify(second,2,25);call('history.undo');assert draft()==baseline;call('history.redo');assert draft()==second
    call('sketch.scaleEdit');call('sketch.scaleCancel');call('sketch.scalePreview',dict(args,factor=1.5));call('sketch.scaleAccept');assert draft()==baseline
    for command,params in [('sketch.deleteSelection',{'entityIds':ids}),('sketch.constraint',{'type':'vertical','entityIds':[ids[1]]}),('sketch.movePoint',{'pointId':point_ids[0],'x':13,'y':14,'units':'mm'})]:
        before=draft();hist=state()['draftHistory'];call(command,params,error='sketch_solve_failed');assert draft()==before and state()['draftHistory']==hist
    # A later pivot edit composes with unchanged authoritative source and factor.
    group_before=copy.deepcopy(baseline['parameters']['scaleGroups']);call('sketch.movePoint',{'pointId':pivot,'x':5,'y':7,'units':'mm'});shifted=draft();verify(shifted,1.5,25,(5,7));assert shifted['parameters']['scaleGroups']==group_before;call('history.undo');assert draft()==baseline
    call('sketch.scaleEdit');call('sketch.cancel');assert not call('app.state').get('activeSession') and call('document.inspect')['authored']==initial['authored']
    # Fresh source: creation UUID stays stable; fixed/external/dimensioned failures
    # preserve existing downstream body, draft, history and no authored owner.
    call('document.open',{'path':str(native/'source.alloy')});source_doc=call('document.inspect');call('sketch.edit',{'sketchId':sid});source=draft();h=state()['draftHistory'];create={'entityIds':ids,'pivotPointId':pivot,'factor':1.5};call('sketch.scalePreview',create);created=state()['pendingSketchScale']['groupId'];call('sketch.scalePreview',dict(create,factor=2));assert state()['pendingSketchScale']['groupId']==created;call('sketch.scaleCancel');assert draft()==source and state()['draftHistory']==h
    for bad in ({'groupId':'','factor':1.5,'pivotPointId':pivot},{'entityIds':[],'factor':1.5,'pivotPointId':pivot}):call('sketch.scalePreview',bad,error='sketch_scale_failed');assert draft()==source and state()['draftHistory']==h
    for kind in ('fixed','external','driving','reference'):
        if kind=='fixed':call('sketch.constraint',{'type':'fixed','pointIds':[point_ids[0]]})
        elif kind=='external':
            call('sketch.addPoint',{'x':10,'y':10,'units':'mm'});anchor=draft()['parameters']['entities'][-1]['pointId'];call('sketch.constraint',{'type':'coincident','pointIds':[point_ids[0],anchor]})
        else:call('sketch.dimension',dict({'kind':'linear','pointIds':point_ids[:2],'units':'mm','driving':kind=='driving'},**({'value':20} if kind=='driving' else {})))
        negative=draft();history=state()['draftHistory'];call('sketch.scalePreview',create,error='sketch_scale_failed');assert draft()==negative and state()['draftHistory']==history and 'pendingSketchScale' not in state() and call('document.inspect')['authored']==source_doc['authored'];call('history.undo')
        if kind=='external':call('history.undo')
        assert draft()==source
    call('sketch.scalePreview',create);call('sketch.scaleAccept');committed=draft();verify(committed);call('sketch.finish');final=call('document.inspect');assert final['evaluation']['bodies'][0]['bodyId']==source_doc['evaluation']['bodies'][0]['bodyId'] and math.isclose(final['evaluation']['bodies'][0]['volumeMm3'],3762,abs_tol=1e-6)
    call('document.save',{'path':str(out/'api-scale.alloy')});call('document.open',{'path':str(out/'api-scale.alloy')});assert call('document.inspect')['authored']==final['authored'];call('view.home');call('capture.screenshot',{'path':str(out/'scaled-solid.png')});gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated'];(out/'report.json').write_text(json.dumps({'passed':True,'mesh15':mesh15,'meshSourceEdit':mesh_edit,'final':final,'gpu':gpu},indent=2));(out/'transcript.json').write_text(json.dumps(log,indent=2))
