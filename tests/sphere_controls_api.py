#!/usr/bin/env python3
"""Face-pick/center-click Sphere shared API; separate physical input qualification."""
import argparse,json,math
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
def support(face):return {k:face[k] for k in ('bodyId','producerFeatureId','faceId')}
def uv(face,world):
    f=face['frame'];delta=[world[i]-f['origin'][i] for i in range(3)]
    return {key:sum(delta[i]*f[axis][i] for i in range(3)) for key,axis in [('uMm','uAxis'),('vMm','vAxis')]}
with application(a.app,out) as (call,log):
    call('sketch.begin',{'plane':'XY'});call('sketch.rectangle',{'x1':-10,'y1':-10,'x2':10,'y2':10,'units':'mm'});call('sketch.finish')
    sketch=call('document.inspect')['evaluation']['sketches'][0];call('solid.extrude',{'sketchId':sketch['sketchId'],'profileId':sketch['profiles'][0]['profileId'],'distance':20,'units':'mm'})
    stock=call('document.inspect');body=stock['evaluation']['bodies'][0];face=next(f for f in body['supportFaces'] if f['frame']['normal'][2]>.99);ref=support(face);center=uv(face,[0,0,20]);call('view.orientation',{'name':'top'});call('view.home')
    s=call('solid.sphereDialog');assert s['selectionRole']=='face';px=call('view.projectFacePoint',dict(support=ref,**center));picked=call('selection.pick',{'x':px['x'],'y':px['y']});assert picked['support']==ref
    assert call('app.state')['activeSession']['selectionRole']=='center'
    # The same native click command maps a second actual ray to the retained plane.
    placed=call('selection.pick',{'x':px['x'],'y':px['y']});assert placed['previewed'];assert math.isclose(placed['geometry']['removedVolumeMm3'],2.25*math.pi,abs_tol=1e-5)
    call('capture.screenshot',{'path':str(out/'sphere-red-cut-preview.png')});call('command.cancel',{'session':s['id']});assert call('document.inspect')['authored']==stock['authored']
    s=call('solid.sphereDialog',{'support':ref,'diameterMm':3});assert s['selectionRole']=='center';call('selection.pick',{'x':px['x'],'y':px['y']});call('command.commit',{'session':s['id']});first=call('document.inspect');assert first['evaluation']['bodies'][0]['bodyId']==body['bodyId']
    call('history.undo');assert call('document.inspect')['authored']==stock['authored'];call('history.redo');assert call('document.inspect')['authored']==first['authored']
    edit=call('feature.editDialog',{'featureId':first['authored']['features'][-1]['id']});call('command.update',{'session':edit['id'],'inputs':{'diameterMm':4}});assert call('command.preview',{'session':edit['id']})['previewed'];call('command.cancel',{'session':edit['id']});assert call('document.inspect')['authored']==first['authored']
    edit=call('feature.editDialog',{'featureId':first['authored']['features'][-1]['id']});call('command.update',{'session':edit['id'],'inputs':{'diameterMm':4}});call('command.commit',{'session':edit['id']});assert call('document.inspect')['evaluation']['bodies'][0]['diameterMm']==4
    # Repeated face binding must clear the center, including after an edit.
    repeat=call('solid.repeatSphere',{'support':ref});assert repeat['selectionRole']=='center' and repeat['inputs']['diameterMm']==4 and 'uMm' not in repeat['inputs'] and 'vMm' not in repeat['inputs']
    second=uv(face,[5,5,20]);pixel=call('view.projectFacePoint',dict(support=ref,**second));call('selection.pick',{'x':pixel['x'],'y':pixel['y']});call('command.commit',{'session':repeat['id']})
    cut=call('document.inspect');assert len(cut['evaluation']['bodies'])==1
    bad=call('solid.sphereDialog',dict(support=ref,diameterMm=3,uMm=10000,vMm=10000));assert not bad['previewed'];call('command.cancel',{'session':bad['id']});assert call('document.inspect')['authored']==cut['authored']
    new=call('solid.sphereDialog',dict(support=ref,operation='newBody',diameterMm=2,**uv(face,[-5,5,20])));preview=call('command.preview',{'session':new['id']});reserved=preview['documentDraft']['features'][-1];assert call('command.preview',{'session':new['id']})['documentDraft']['features'][-1]['bodyId']==reserved['bodyId'];call('command.commit',{'session':new['id']});assert len(call('document.inspect')['evaluation']['bodies'])==2;call('history.undo')
    path=out/'sphere-pips.alloy';call('document.save',{'path':str(path)});call('document.open',{'path':str(path)});assert call('document.inspect')['authored']==cut['authored'];call('capture.screenshot',{'path':str(out/'sphere-pips-committed.png')})
    (out/'report.json').write_text(json.dumps({'passed':True,'scope':'shared API face/center rays, geometry, history and storage; physical OS input separate','stock':stock,'result':cut},indent=2));(out/'transcript.json').write_text(json.dumps(log,indent=2))
