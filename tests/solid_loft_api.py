#!/usr/bin/env python3
"""Real-entrypoint circular Loft and downstream 2mm Shell; native review is separate."""
import argparse
import copy
import json
from pathlib import Path
from app_harness import application

p=argparse.ArgumentParser()
p.add_argument('--app',required=True)
p.add_argument('--output',required=True)
a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,transcript):
    initial=call('app.state');assert initial['viewport']['gpu']['hardwareAccelerated']
    preceding=None;planes=[];references=[]
    for index,diameter in enumerate((38,16,30,44)):
        if index:
            plane=call('construction.plane',{'source':{'sketchId':preceding},'offset':18 if index==1 else 20,'units':'mm','name':f'Plane {index}'})['planeId'];planes.append(plane)
            call('sketch.begin',{'planeId':plane,'name':f'Circle {diameter}'})
        else:call('sketch.begin',{'plane':'XY','name':'Circle 38'})
        call('sketch.addCircle',{'centerX':0,'centerY':0,'diameter':diameter,'units':'mm'});call('sketch.finish')
        sketch=call('document.inspect')['evaluation']['sketches'][-1];preceding=sketch['sketchId'];references.append({'sketchId':preceding,'profileId':sketch['profiles'][0]['profileId']})
    before=call('document.inspect');call('view.home')
    session=call('solid.loftDialog',{'profiles':[],'name':'Egg cup Loft'})
    for ref in references:
        point=call('view.projectSketchPoint',{'sketchId':ref['sketchId'],'uMm':0,'vMm':0})
        picked=call('selection.pick',{'x':point['x'],'y':point['y']});assert picked['sketchId']==ref['sketchId'] and picked['profileId']==ref['profileId']
    preview=call('command.preview',{'session':session['id']});assert preview['inputs']['profiles']==references and preview['geometry']['valid'] and preview['geometry']['closed']
    assert call('command.preview',{'session':session['id']})['bodyId']==preview['bodyId']
    call('capture.screenshot',{'path':str(out/'loft-preview.png')});call('capture.dialog',{'path':str(out/'loft-panel.png')})
    call('command.cancel',{'session':session['id']});assert call('document.inspect')['authored']==before['authored']
    session=call('command.begin',{'command':'solid.loft','inputs':{'profiles':references,'name':'Egg cup Loft'}})
    preview=call('command.preview',{'session':session['id']});call('command.commit',{'session':session['id']})
    loft=call('document.inspect');body=loft['evaluation']['bodies'][0];assert body['bodyId']==preview['bodyId'];feature=loft['authored']['features'][-1]
    assert body['valid'] and body['closed'] and body['topology']['solids']==1
    for f in loft['authored']['features']:
        if f['type'] in ('sketch','constructionPlane'):call('feature.visibility',{'featureId':f['id'],'visible':False})
    call('view.home');call('capture.screenshot',{'path':str(out/'loft-solid.png')})
    cap=next(f for f in body['supportFaces'] if f['role']=='loft.end');support={k:cap[k] for k in ('producerFeatureId','bodyId','faceId')}
    call('solid.shell',{'targetBodyId':body['bodyId'],'removedFace':support,'thicknessMm':2,'name':'Shell 2mm'})
    shell=call('document.inspect');assert shell['evaluation']['bodies'][0]['valid'] and shell['evaluation']['bodies'][0]['closed']
    call('view.home');call('capture.screenshot',{'path':str(out/'shell-cup.png')})
    edit=call('feature.editDialog',{'featureId':feature['id']});ep=call('command.preview',{'session':edit['id']});assert ep['bodyId']==body['bodyId'];call('command.cancel',{'session':edit['id']});assert call('document.inspect')['authored']==shell['authored']
    bad=copy.deepcopy(references);bad[1],bad[2]=bad[2],bad[1]
    call('solid.loft',{'featureId':feature['id'],'profiles':bad},error='evaluation_failed');assert call('document.inspect')['authored']==shell['authored']
    call('feature.edit',{'featureId':planes[0],'parameters':{'offsetMm':19}})
    edited=call('document.inspect');assert abs(edited['evaluation']['bodies'][0]['boundsMm']['max'][2]-59)<1e-4
    base=edited['authored']['features'][0];entities=copy.deepcopy(base['parameters']['entities']);entities[0]['radiusMm']=20
    call('feature.edit',{'featureId':base['id'],'parameters':{'entities':entities}})
    edited=call('document.inspect');assert edited['evaluation']['bodies'][0]['bodyId']==body['bodyId']
    call('history.undo');call('history.redo');assert call('document.inspect')['authored']==edited['authored']
    path=out/'egg-cup.alloy';call('document.save',{'path':str(path)});call('document.new',{'discard':True});call('document.open',{'path':str(path),'discard':True});assert call('document.inspect')['authored']==edited['authored']
(out/'loft-api-ledger.json').write_text(json.dumps({'initial':initial,'loft':loft,'shell':shell,'edited':edited,'transcript':transcript},indent=2))
print('PASS: ordered pick/preview/cancel, stable Loft body/caps, 2mm Shell, edit/order refusal/history/storage')
