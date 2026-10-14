#!/usr/bin/env python3
"""One retained Loft sheet → signed Thicken, preview/Cancel and strict solid export."""
import argparse
import json
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl

p=argparse.ArgumentParser()
p.add_argument('--app',required=True)
p.add_argument('--source',required=True)
p.add_argument('--output',required=True)
a=p.parse_args();out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
with application(a.app,out) as (call,transcript):
    call('window.resize',{'width':1440,'height':900})
    call('document.open',{'path':str(Path(a.source).resolve()),'discard':True})
    initial=call('document.inspect');sheet=initial['evaluation']['bodies'][0]
    face=sheet['supportFaces'][0]
    ref={key:face[key] for key in ('producerFeatureId','bodyId','faceId')}
    assert sheet['type']=='surface' and not face['planar'] and len(face['edges'])==4
    call('view.home');call('capture.screenshot',{'path':str(out/'unselected-yellow-sheet.png')})
    call('selection.faceSet',{'support':ref});call('capture.screenshot',{'path':str(out/'selected-blue-sheet.png')})
    dialog=call('solid.thickenDialog',{'signedThicknessMm':3.0})
    assert dialog['previewed'] and dialog['inputs']['sourceFace']==ref
    call('capture.screenshot',{'path':str(out/'thicken-preview.png')})
    call('capture.dialog',{'path':str(out/'thicken-controls.png')})
    call('command.cancel',{'session':dialog['id']})
    assert call('document.inspect')['authored']==initial['authored']
    session=call('solid.thickenDialog',{'sourceFace':ref,'signedThicknessMm':3.0})
    preview=call('command.preview',{'session':session['id']})
    assert preview['geometry']['type']=='solid' and preview['geometry']['closed']
    assert preview['geometry']['topology']=={'solids':1,'shells':1,'faces':6,'edges':12,'vertices':8}
    call('command.commit',{'session':session['id']})
    committed=call('document.inspect')
    assert len(committed['evaluation']['bodies'])==2
    retained,solid=committed['evaluation']['bodies'];feature=committed['authored']['features'][-1]
    assert retained['bodyId']==sheet['bodyId'] and not retained['visible']
    assert solid['bodyId']!=sheet['bodyId'] and solid['ownerId']==committed['authored']['documentId']
    assert solid['volumeMm3']>10000 and len(solid['supportFaces'])==6 and len(solid['solidEdges'])==12
    assert feature['parameters']=={'sourceFace':ref,'signedThicknessMm':3}
    assert len({f['faceId'] for f in solid['supportFaces']})==6
    assert len({e['edgeId'] for e in solid['solidEdges']})==12
    call('view.home');call('capture.screenshot',{'path':str(out/'gray-solid-committed.png')})
    stl=out/'shoehorn-thickened-mm.stl'
    exported=call('document.exportStl',{'path':str(stl),'bodyId':solid['bodyId'],'units':'mm','format':'binary',
        'surfaceDeviationMm':0.1,'normalDeviationDeg':10})
    mesh=inspect_stl(stl,True)
    assert mesh['triangles']==exported['triangles'] and mesh['signedVolume']>0
    edit=call('solid.thickenDialog',{'featureId':feature['id']})
    changed=call('command.update',{'session':edit['id'],'inputs':{'signedThicknessMm':-3.0}})
    assert changed['inputs']['signedThicknessMm']==-3
    flipped=call('command.preview',{'session':edit['id']})
    assert flipped['geometryEvaluated'] and abs(flipped['geometry']['volumeMm3']-solid['volumeMm3'])>100
    call('command.commit',{'session':edit['id']})
    revised=call('document.inspect');after=revised['evaluation']['bodies'][-1]
    assert after['bodyId']==solid['bodyId'] and after['volumeMm3']>0
    assert {f['faceId'] for f in after['supportFaces']}=={f['faceId'] for f in solid['supportFaces']}
    call('history.undo');call('history.redo')
    assert call('document.inspect')['authored']==revised['authored']
    saved=out/'shoehorn-thickened.alloy';call('document.save',{'path':str(saved)})
    call('document.new',{'discard':True});call('document.open',{'path':str(saved),'discard':True})
    reopened=call('document.inspect')
    assert reopened['authored']==revised['authored'] and reopened['evaluation']['bodies'][-1]['bodyId']==solid['bodyId']
    call('view.home');call('capture.screenshot',{'path':str(out/'solid-reopened.png')})
(out/'surface-thicken-ledger.json').write_text(json.dumps({'initial':initial,'committed':committed,
    'revised':revised,'reopened':reopened,'exported':exported,'mesh':mesh,'transcript':transcript},indent=2))
print('PASS: exact Loft sheet Thicken, signed side edit, stable topology, strict mm STL, reopen')
