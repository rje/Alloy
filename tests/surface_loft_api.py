#!/usr/bin/env python3
"""SRF-009 retained open arc/line Loft with a fit-spline Centerline in the real editor."""
import argparse
import copy
import json
from pathlib import Path
from app_harness import application

p=argparse.ArgumentParser()
p.add_argument('--app',required=True)
p.add_argument('--source',required=True)
p.add_argument('--output',required=True)
a=p.parse_args();out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
with application(a.app,out) as (call,transcript):
    call('window.resize',{'width':1440,'height':900})
    call('document.open',{'path':str(Path(a.source).resolve()),'discard':True})
    initial=call('document.inspect')
    source=initial['authored']['features']
    path=next(f for f in source if f['type']=='sketch' and f['name']=='Path')
    front=next(f for f in source if f['type']=='sketch' and f['name']=='Arc')
    rear=next(f for f in source if f['type']=='sketch' and f['name']=='Line')
    def entity(f,kind):return next(e for e in f['parameters']['entities'] if e['type']==kind)
    sections=[{'sketchId':front['id'],'entityId':entity(front,'arc')['id']},
              {'sketchId':rear['id'],'entityId':entity(rear,'line')['id']}]
    guide={'sketchId':path['id'],'entityId':entity(path,'fitSpline')['id']}
    call('view.home')
    dialog=call('solid.loftDialog',{'operation':'newComponent','componentName':'Shoehorn',
                                   'name':'Guided surface'})
    assert not dialog['previewed']
    for role,ref in zip(('section1','section2','centerline'),sections+[guide]):
        picked=call('solid.loftEntity',{'role':role,**ref})
        assert picked['entityId']==ref['entityId']
    preview=call('command.preview',{'session':dialog['id']})
    assert preview['geometry']['type']=='surface' and preview['geometry']['topology']['faces']==1
    assert preview['geometry']['topology']['solids']==0 and preview['geometry']['topology']['edges']==4
    call('capture.screenshot',{'path':str(out/'guided-sheet-preview.png')})
    call('capture.dialog',{'path':str(out/'guided-loft-controls.png')})
    call('command.cancel',{'session':dialog['id']})
    assert call('document.inspect')['authored']==initial['authored']
    inputs={'operation':'newComponent','componentName':'Shoehorn','name':'Guided surface',
            'sections':sections,'centerline':guide}
    session=call('command.begin',{'command':'solid.loft','inputs':inputs})
    shown=call('command.preview',{'session':session['id']})
    call('command.commit',{'session':session['id']})
    committed=call('document.inspect')
    body=next(b for b in committed['evaluation']['bodies'] if b['bodyId']==shown['bodyId'])
    loft=committed['authored']['features'][-1]
    face=body['supportFaces'][0]
    assert body['valid'] and not body['closed'] and body['type']=='surface'
    assert len(committed['authored']['components'])==1
    assert body['ownerId']==committed['authored']['components'][0]['id']
    assert not face['planar'] and len(face['edges'])==4
    assert loft['parameters']=={'operation':'newComponent','sections':sections,'centerline':guide}
    for f in source:call('feature.visibility',{'featureId':f['id'],'visible':False})
    call('view.home');call('capture.screenshot',{'path':str(out/'guided-sheet-committed.png')})
    path_params=copy.deepcopy(path['parameters'])
    middle=entity(path,'fitSpline')['pointIds'][1]
    for point in path_params['points']:
        if point['id']==middle:point['vMm']=20
    call('feature.edit',{'featureId':path['id'],'parameters':path_params})
    edited=call('document.inspect')
    current=next(b for b in edited['evaluation']['bodies'] if b['bodyId']==body['bodyId'])
    assert current['supportFaces'][0]['faceId']==face['faceId']
    assert abs(current['areaMm2']-body['areaMm2'])>1
    call('history.undo');call('history.redo')
    assert call('document.inspect')['authored']==edited['authored']
    target=out/'shoehorn-surface.alloy';call('document.save',{'path':str(target)})
    call('document.new',{'discard':True})
    call('document.open',{'path':str(target),'discard':True})
    reopened=call('document.inspect')
    assert reopened['authored']==edited['authored']
    assert reopened['evaluation']['bodies'][0]['supportFaces'][0]['faceId']==face['faceId']
    call('view.home');call('capture.screenshot',{'path':str(out/'guided-sheet-reopened.png')})
(out/'surface-loft-ledger.json').write_text(json.dumps({'initial':initial,'committed':committed,
    'edited':edited,'reopened':reopened,'transcript':transcript},indent=2))
print('PASS: open arc/line guided Loft preview/Cancel, New Component sheet, stable face edit/history/reopen')
