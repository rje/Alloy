#!/usr/bin/env python3
"""SRF-012 exterior endpoint-contact rail Loft through the real editor API."""
import argparse
import copy
import json
from pathlib import Path
from app_harness import application

p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--source',required=True)
p.add_argument('--output',required=True);a=p.parse_args()
out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
with application(a.app,out,command_timeout=90) as (call,transcript):
    call('window.resize',{'width':1440,'height':900})
    call('document.open',{'path':str(Path(a.source).resolve()),'discard':True})
    initial=call('document.inspect');source=initial['authored']['features']
    back=next(f for f in source if f['name']=='Back open arc')
    lower=next(f for f in source if f['name']=='Mirrored lower chain')
    rail=next(f for f in source if f['name']=='Exterior guide rail')
    def ids(feature):return [e['id'] for e in feature['parameters']['entities']]
    sections=[{'sketchId':back['id'],'entityIds':ids(back)},
              {'sketchId':lower['id'],'entityIds':ids(lower)}]
    rail_ref={'sketchId':rail['id'],'entityId':ids(rail)[0]}
    call('view.home')
    dialog=call('surface.loftDialog',{'guideType':'rail','componentName':'Iron upper',
                                      'name':'Exterior rail sheet'})
    assert not dialog['previewed']
    for role,feature,entity_id in [('section1',back,ids(back)[0]),
                                   ('section2',lower,ids(lower)[0]),
                                   ('section2',lower,ids(lower)[1]),
                                   ('rail',rail,rail_ref['entityId'])]:
        call('solid.loftEntity',{'role':role,'sketchId':feature['id'],'entityId':entity_id})
    preview=call('command.preview',{'session':dialog['id']})
    assert preview['previewed'] and preview['geometry']['topology']['faces']==2
    assert preview['geometry']['topology']['edges']==7 and preview['geometry']['topology']['solids']==0
    call('capture.dialog',{'path':str(out/'rail-loft-controls.png')})
    call('capture.screenshot',{'path':str(out/'rail-loft-preview.png')})
    call('command.cancel',{'session':dialog['id']})
    assert call('document.inspect')['authored']==initial['authored']
    inputs={'operation':'newComponent','guideType':'rail','componentName':'Iron upper',
            'name':'Exterior rail sheet','sections':sections,'rail':rail_ref}
    session=call('command.begin',{'command':'solid.loft','inputs':inputs})
    call('command.commit',{'session':session['id']})
    committed=call('document.inspect');body=committed['evaluation']['bodies'][0]
    loft=committed['authored']['features'][-1];face_ids=[face['faceId'] for face in body['supportFaces']]
    assert body['type']=='surface' and body['valid'] and not body['closed']
    assert body['topology']['faces']==2 and body['topology']['solids']==0
    assert len(face_ids)==2 and face_ids[0]!=face_ids[1]
    assert all(len(face['edges'])==4 for face in body['supportFaces'])
    assert loft['parameters']=={'operation':'newComponent','guideType':'rail','sections':sections,'rail':rail_ref}
    assert body['ownerId']==committed['authored']['components'][0]['id']
    for feature in source:call('feature.visibility',{'featureId':feature['id'],'visible':False})
    call('view.home');call('capture.screenshot',{'path':str(out/'rail-loft-committed.png')})
    rail_params=copy.deepcopy(rail['parameters']);middle=rail_params['entities'][0]['pointIds'][1]
    for point in rail_params['points']:
        if point['id']==middle:point['uMm']=-36
    call('feature.edit',{'featureId':rail['id'],'parameters':rail_params})
    edited=call('document.inspect');current=edited['evaluation']['bodies'][0]
    assert [face['faceId'] for face in current['supportFaces']]==face_ids
    assert abs(current['areaMm2']-body['areaMm2'])>1
    call('history.undo');call('history.redo')
    assert call('document.inspect')['authored']==edited['authored']
    target=out/'exterior-rail-sheet.alloy';call('document.save',{'path':str(target)})
    call('document.new',{'discard':True});call('document.open',{'path':str(target),'discard':True})
    reopened=call('document.inspect')
    assert reopened['authored']==edited['authored']
    assert [face['faceId'] for face in reopened['evaluation']['bodies'][0]['supportFaces']]==face_ids
    edit=call('feature.editDialog',{'featureId':loft['id']})
    assert edit['previewed'] and edit['inputs']['guideType']=='rail'
    call('command.cancel',{'session':edit['id']})
    assert call('document.inspect')['authored']==edited['authored']
    call('view.home');call('capture.screenshot',{'path':str(out/'rail-loft-reopened.png')})
    (out/'result.json').write_text(json.dumps({'passed':True,'bodyId':body['bodyId'],
        'featureId':loft['id'],'faceIds':face_ids,'initialAreaMm2':body['areaMm2'],
        'editedAreaMm2':current['areaMm2']},indent=2)+'\n')
    (out/'transcript.json').write_text(json.dumps(transcript,indent=2)+'\n')
