#!/usr/bin/env python3
"""Blank-document source workflow via shared API commands; native ingredients reused."""
import argparse
from contextlib import ExitStack
import hashlib
import json
import math
from pathlib import Path
import time
from app_harness import application
from stl_export_api import inspect_stl

VERTICES=[(0,0),(0,40),(14,40),(14,-20),(3,-20),(3,24),(11,24),(11,0)]
LENGTHS=[40,14,60,11,44,8,24]
POINTERS=[(0,20),(10,40),(14,0),(8,-20),(3,0),(8,24),(11,10)]
ANNOTATIONS=[(-4,20),(7,43),(18,10),(8.5,-23),(0,2),(7,27),(13,12)]
PATH_LENGTH=135+16.5*math.pi
VOLUME=PATH_LENGTH*math.pi
BOUNDS=[-1,-21,-1,15,41,1]


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--app',required=True);p.add_argument('--output',required=True);p.add_argument('--record-video',type=Path);a=p.parse_args();out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
    report={'result':'incomplete','sourceId':'sweep-paper-clip-warwick-2017-20261013','baseline':'46419a2','stages':[],'inputProvenance':'Shared API commands invoke real native authority. No OS keys or coordinate-seeded Line/Arc/Circle authoring. Native ingredients reused from221/222/224/226.'};transcript=[];recording=False
    try:
        with application(a.app,out) as (raw,transcript),ExitStack() as cleanup:
            def stop():
                nonlocal recording
                if recording:recording=False;report['recording']=raw('recording.stop')
            cleanup.callback(stop);raw('window.resize',{'width':1400,'height':1000});assert not raw('document.inspect')['authored']['features']
            if a.record_video:
                movie=a.record_video.resolve();assert not movie.is_relative_to(Path(__file__).resolve().parents[1]);movie.parent.mkdir(parents=True,exist_ok=True);raw('recording.start',{'path':str(movie),'fps':10});recording=True;time.sleep(.8)
            def call(command,parameters=None):
                r=raw(command,parameters)
                if recording and command.startswith(('sketch.','solid.','command.','view.')):time.sleep(.25)
                return r
            def stage(steps,text):
                report['stages'].append({'sourceSteps':steps,'caption':text})
                if recording:raw('recording.step',{'text':f'API | {steps}: {text}'});time.sleep(.6)
            def state():return raw('app.state')
            def inspect():return raw('document.inspect')
            def draft():return state()['activeSession']['draftSketch']
            def capture(name):
                call('capture.screenshot',{'path':str(out/(name+'.png'))});(out/(name+'.json')).write_text(json.dumps(state(),indent=2))
            def pixel(sketch,u,v):
                q=raw('view.projectSketchPoint',{'sketchId':sketch['id'],'uMm':u,'vMm':v});return {'x':q['x'],'y':q['y']}
            def frame_path():
                # Disclosed presentation-only framing; done outside value-lock sequences.
                for _ in range(6):
                    s=state();v=s['viewport'];q=pixel(draft(),7,10);call('view.pan',{'x':v['width']/2-q['x'],'y':v['height']/2-q['y']});points=[pixel(draft(),*p) for p in VERTICES+ANNOTATIONS];height=max(p['y'] for p in points)-min(p['y'] for p in points)
                    if not all(28<p['x']<v['width']-28 and 28<p['y']<v['height']-28 for p in points):call('view.zoom',{'factor':.85})
                    elif height<v['height']*.5:call('view.zoom',{'factor':1.15})
                    else:return
                raise AssertionError('Source path/dimension witnesses did not fit the actual viewport')
            stage('1–5','Blank document; Top / XY sketch, shared Line tool')
            call('sketch.begin',{'plane':'XY','units':'mm','name':'Paper-clip path'});call('sketch.lookAt');frame_path();call('sketch.tool',{'tool':'line'});call('sketch.placePoint',pixel(draft(),0,0));line_focus=[]
            for i,(length,pointer,expected) in enumerate(zip(LENGTHS,POINTERS,VERTICES[1:])):
                direction='vertical' if i%2==0 else 'horizontal';stage('1–5',f'Segment{i+1}: type{length}mm → Tab-lock command → pointer {direction} → click')
                call('sketch.lineLengthPreview',{'text':str(length)});line_focus.append({'segment':i+1,'beforeLock':state()['focus']});locked=call('sketch.lineLengthLock');assert locked['pendingLineLength']['phase']=='locked';line_focus[-1]['afterLock']=state()['focus'];assert state()['activeSession']['draftSketch']['parameters']['entities'].__len__()==i
                moved=call('sketch.pointerMove',pixel(draft(),*pointer));assert moved['point']['direction']==direction
                if i in(0,4):capture(f'01-line-{i+1}-locked')
                call('sketch.placePoint',pixel(draft(),*pointer));params=draft()['parameters'];assert len(params['entities'])==i+1 and params['dimensions'][-1]['valueMm']==length;end_id=params['entities'][-1]['endPointId'];end=next(q for q in params['points'] if q['id']==end_id);assert math.isclose(end['uMm'],expected[0],abs_tol=1e-6) and math.isclose(end['vMm'],expected[1],abs_tol=1e-6)
            call('sketch.tool',{'tool':'select'});sharp=draft();ids=[e['id'] for e in sharp['parameters']['entities']];dimension_ids=[d['id'] for d in sharp['parameters']['dimensions']];assert len(ids)==7 and [d['valueMm'] for d in sharp['parameters']['dimensions']]==LENGTHS
            stage('Alloy presentation','Place the existing seven labels clearly; geometry and values unchanged')
            for dim,placement in zip(dimension_ids,ANNOTATIONS):call('sketch.annotationMove',{'dimensionId':dim,'uMm':placement[0],'vMm':placement[1]})
            frame_path();capture('02-seven-locked-lines');report['sharpSource']=draft();report['lineFocus']=line_focus;report['annotationChoice']={'positionsMm':ANNOTATIONS,'reason':'Readable witnesses; source does not prescribe exact stored label anchors'}
            conversions=[]
            for pair,radius in enumerate((7,5.5,4)):
                stage('5–10',f'Actual paired corner picks: radius{radius}mm; inspect disclosed span conversion')
                before=draft();call('sketch.tool',{'tool':'fillet'})
                call('sketch.placePoint',pixel(draft(),*VERTICES[pair*2+1]));call('sketch.filletPreview',{'value':radius,'units':'mm'});call('sketch.placePoint',pixel(draft(),*VERTICES[pair*2+2]))
                pending=state()['activeSession']['pendingFillet'];assert len(pending['corners'])==2 and not pending.get('previewError');assert draft()==before;assert pending['previewDiagnostics']['convertedReferenceDimensionIds']==[dimension_ids[pair*2+1]];conversions.append(pending['previewDiagnostics']);capture(f'03-pair-{pair+1}-preview');call('sketch.filletAccept',{'value':radius,'units':'mm'})
            path=draft();params=path['parameters'];arcs=[e for e in params['entities'] if e['type']=='arc'];assert len(arcs)==6 and sorted(e['radiusMm'] for e in arcs)==[4,4,5.5,5.5,7,7];assert {e['id'] for e in params['entities'] if e['type']=='line'}=={ids[i] for i in(0,2,4,6)}
            for dim in (dimension_ids[i] for i in(1,3,5)):
                d=next(d for d in params['dimensions'] if d['id']==dim);assert not d['driving'] and d['kind']=='horizontal' and len(d['pointIds'])==2
            points={p['id']:(p['uMm'],p['vMm']) for p in params['points']};length=sum(math.dist(points[e['startPointId']],points[e['endPointId']]) if e['type']=='line' else e['radiusMm']*abs(e['sweepAngleDeg'])*math.pi/180 for e in params['entities']);assert math.isclose(length,PATH_LENGTH,abs_tol=1e-6);capture('04-six-real-fillet-arcs');call('sketch.finish');report.update(pathLengthMm=length,filletConversions=conversions,pathSource=path)
            stage('11–14','Front / XZ origin circle: type2 → Tab lock → Enter acceptance commands')
            call('sketch.begin',{'plane':'XZ','units':'mm','name':'Diameter2 section'});call('sketch.lookAt');call('view.zoom',{'factor':8});call('sketch.tool',{'tool':'circle'});call('sketch.placePoint',pixel(draft(),0,0));call('sketch.circleDiameterPreview',{'text':'2'});before_lock=state();call('sketch.circleDiameterLock');locked=state();assert locked['activeSession']['pendingCircleDiameter']['phase']=='locked' and locked['inlineEditor']['readOnly'];assert locked['focus']==before_lock['focus'];report['circleLock']={'beforeFocus':before_lock['focus'],'afterFocus':locked['focus'],'inlineEditor':locked['inlineEditor']};capture('05-circle-tab-locked');call('sketch.circleDiameterAccept');circle=draft();assert len(circle['parameters']['entities'])==1 and circle['parameters']['entities'][0]['radiusMm']==1;diameter=next(d for d in circle['parameters']['dimensions'] if d['kind']=='diameter');assert diameter['driving'] and diameter['valueMm']==2;capture('06-circle-accepted');call('sketch.finish');report['circleSource']=circle
            stage('15–21','Home → Sweep; actual Profile and Path pixel selectors, checked Chain Selection')
            call('view.home');session=call('solid.sweepDialog');assert session['selectionRole']=='profile' and session['chainSelection'];call('selection.pick',pixel(circle,0,0));assert state()['activeSession']['selectionRole']=='path';call('solid.sweepSelection',{'role':'path'});call('selection.pick',pixel(path,0,16));preview=call('command.preview',{'session':session['id']});assert preview['previewed'] and len(preview['inputs']['pathEntityIds'])==10;assert math.isclose(preview['geometry']['volumeMm3'],VOLUME,abs_tol=1e-5);assert preview['inputs']['extent']=='full' and preview['inputs']['orientation']=='perpendicular' and preview['inputs']['operation']=='newBody';capture('07-full-sweep-preview');call('command.commit',{'session':session['id']});full=inspect();assert len(full['evaluation']['bodies'])==1;body=full['evaluation']['bodies'][0];assert body['valid'] and body['closed'] and body['topology']['solids']==1 and math.isclose(body['volumeMm3'],VOLUME,abs_tol=1e-5)
            for actual,expected in zip(body['boundsMm']['min']+body['boundsMm']['max'],BOUNDS):assert math.isclose(actual,expected,abs_tol=1e-6)
            feature=next(f for f in full['authored']['features'] if f['type']=='sweep');call('view.home');capture('08-complete-paper-clip');report.update(fullBody=body,sourceAuthored=full['authored'],sweepFeature=feature,gpu=state()['viewport']['gpu'])
            stage('Alloy export','Save editable source and export one-component millimeter STL; source prescribes no mesh settings')
            model=out/'paper-clip.alloy';mesh=out/'paper-clip-mm.stl';call('document.save',{'path':str(model)});report['export']=call('document.exportStl',{'path':str(mesh),'bodyId':body['bodyId'],'units':'mm','format':'binary','surfaceDeviationMm':.01,'normalDeviationDeg':4});assert inspect()['authored']==full['authored'] and not report['export']['authoredChanged'];report.update(model=str(model),meshPath=str(mesh));
            if recording:time.sleep(1.2)
            stop()
            # Alloy integrity additions, outside source creation recording.
            edit=call('feature.editDialog',{'featureId':feature['id']});call('command.preview',{'session':edit['id']});cancel_before=inspect();call('command.cancel',{'session':edit['id']});assert inspect()['authored']==cancel_before['authored'] and inspect()['undo']==cancel_before['undo']
            call('sketch.edit',{'sketchId':circle['id']});call('sketch.dimensionUpdate',{'dimensionId':diameter['id'],'value':1.6,'units':'mm'});call('sketch.finish');edited=inspect();edited_body=edited['evaluation']['bodies'][0];assert edited_body['bodyId']==body['bodyId'] and math.isclose(edited_body['volumeMm3'],VOLUME*.64,abs_tol=1e-5);assert next(f for f in edited['authored']['features'] if f['type']=='sweep')==feature;call('document.save',{'path':str(out/'paper-clip-diameter1.6.alloy')});call('view.home');capture('09-controlled-upstream-edit');call('history.undo');assert inspect()['authored']==full['authored'];call('document.open',{'path':str(model),'discard':True});assert inspect()['authored']==full['authored'];report['integrity']={'editChoice':'Alloy addition: Circle diameter2→1.6mm, then undo','editedBody':edited_body,'cancelAuthoredHistory':'exact','reopen':'exact','stableFeatureId':feature['id'],'stableBodyId':body['bodyId']};report['result']='creation-and-integrity-pass'
        report['mesh']=inspect_stl(mesh,True);assert len(report['mesh']['components'])==1;assert report['mesh']['triangles']==report['export']['triangles'];assert math.isclose(report['mesh']['signedVolume'],VOLUME,rel_tol=.003)
        for actual,expected in zip(report['mesh']['bounds'][0]+report['mesh']['bounds'][1],BOUNDS):assert abs(actual-expected)<.02
        forbidden={'sketch.line','sketch.addArc','sketch.addCircle','sketch.rectangle'};assert not forbidden.intersection(item['command'] for item in transcript);report['result']='api-pass';report['sha256']={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in (model,mesh)}
    finally:
        report['transcript']=transcript;(out/'paper-clip-tutorial-api.json').write_text(json.dumps(report,indent=2))


if __name__=='__main__':main()
