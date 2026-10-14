#!/usr/bin/env python3
"""Real-entrypoint retained back-wall extent for a Box/Shell cutter and sibling lid."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
def ref(face):return {k:face[k] for k in ('producerFeatureId','bodyId','faceId')}
with application(a.app,out) as (call,transcript):
    assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
    box_owner=call('component.create',{'name':'Box'})['definitionId'];call('component.activate',{'definitionId':box_owner})
    call('document.parameterAdd',{'name':'Length','value':80,'units':'mm'})
    call('document.parameterAdd',{'name':'Clearance','value':0.25,'units':'mm'})
    call('solid.box',{'width':50,'depthParameter':'Length','height':40,'units':'mm','name':'Box'})
    body=call('document.inspect')['evaluation']['bodies'][0];top=next(f for f in body['supportFaces'] if f['role']=='box.z1')
    call('solid.shell',{'targetBodyId':body['bodyId'],'removedFace':ref(top),'thicknessMm':4})
    body=call('document.inspect')['evaluation']['bodies'][0]
    back=next(f for f in body['supportFaces'] if f['role']=='shell-inner' and abs(f['frame']['origin'][1]-76)<1e-6)
    sketch=call('sketch.begin',{'plane':'XZ','name':'End-extent cutter'})['draftSketch']['id']
    call('sketch.rectangle',{'x1':4,'y1':36,'x2':46,'y2':42,'units':'mm'});call('sketch.finish')
    source=call('document.inspect');profile=source['evaluation']['sketches'][-1]['profiles'][0]['profileId']
    inputs={'sketchId':sketch,'profileId':profile,'extent':'toObject','endSupport':ref(back),'operation':'cut','targetBodyIds':[body['bodyId']]}
    session=call('solid.extrudeDialog',inputs);preview=call('command.preview',{'session':session['id']})
    assert math.isclose(preview['geometry']['volumeMm3'],50464,abs_tol=1e-4)
    assert call('document.inspect')==source
    call('view.home');call('capture.screenshot',{'path':str(out/'cut-to-back-preview.png')})
    call('command.cancel',{'session':session['id']});assert call('document.inspect')==source
    call('solid.extrude',inputs)
    cut=call('document.inspect');assert math.isclose(cut['evaluation']['bodies'][0]['volumeMm3'],50464,abs_tol=1e-4)
    lid_owner=call('component.create',{'name':'Lid'})['definitionId'];call('component.activate',{'definitionId':lid_owner})
    sketch=call('sketch.begin',{'plane':'XZ','name':'Clearance extent profile'})['draftSketch']['id']
    call('sketch.rectangle',{'x1':4.25,'y1':36.25,'x2':45.75,'y2':39.75,'units':'mm'});call('sketch.finish')
    source=call('document.inspect');profile=source['evaluation']['sketches'][-1]['profiles'][0]['profileId']
    session=call('solid.extrudeDialog',{'sketchId':sketch,'profileId':profile,'extent':'toObject','endSupport':ref(back),'endOffsetParameter':'Clearance'})
    preview=call('command.preview',{'session':session['id']});assert math.isclose(preview['resolvedDistanceMm'],-75.75,abs_tol=1e-6)
    call('capture.screenshot',{'path':str(out/'lid-clearance-preview.png')});call('command.commit',{'session':session['id']})
    accepted=call('document.inspect');feature=accepted['authored']['features'][-1];assert feature['ownerId']==lid_owner
    assert 'distanceMm' not in feature['parameters'] and feature['parameters']['endSupport']==ref(back)
    length=next(r for r in accepted['authored']['userParameters'] if r['name']=='Length')
    call('document.parameterUpdate',{'parameterId':length['id'],'name':'Length','value':90,'units':'mm'})
    resized=call('document.inspect');assert math.isclose(resized['evaluation']['bodies'][-1]['boundsMm']['max'][1],85.75,abs_tol=1e-6)
    call('history.undo');assert call('document.inspect')['authored']==accepted['authored'];call('history.redo')
    edit=call('feature.editDialog',{'featureId':feature['id']});assert edit['inputs']['extent']=='toObject' and edit['inputs']['endOffsetParameter']=='Clearance'
    call('command.update',{'session':edit['id'],'inputs':{'endOffset':0.5}});call('command.preview',{'session':edit['id']});call('command.commit',{'session':edit['id']})
    final=call('document.inspect');assert math.isclose(final['evaluation']['bodies'][-1]['boundsMm']['max'][1],85.5,abs_tol=1e-6)
    assert 'endOffsetParameterId' not in final['authored']['features'][-1]['parameters']
    path=out/'retained-lid-extent.alloy';call('document.save',{'path':str(path)});call('document.open',{'path':str(path),'discard':True})
    assert call('document.inspect')['authored']==final['authored']
    call('view.home');call('capture.screenshot',{'path':str(out/'extent-reopened.png')})
(out/'receipt.json').write_text(json.dumps({'source':source,'accepted':accepted,'final':final,'transcript':transcript},indent=2)+'\n')
print('PASS real app Cut, sibling extent, clearance, preview/cancel, edit, undo and reopen')
