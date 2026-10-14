#!/usr/bin/env python3
"""SRF-013 sketch-tool Trim through the real editor command authority."""
import argparse, copy, json, uuid
from pathlib import Path
from app_harness import application

p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--source',required=True);p.add_argument('--output',required=True)
a=p.parse_args();out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
with application(a.app,out,command_timeout=120) as (call,ledger):
    call('window.resize',{'width':1440,'height':920})
    call('document.open',{'path':str(Path(a.source).resolve()),'discard':True})
    before=call('document.inspect');authored=before['authored']
    target=next(b for b in before['evaluation']['bodies'] if b['type']=='surface' and b['topology']['faces']==2)
    tool=next(f for f in authored['features'] if f['name']=='Paired handle splines')
    entity_ids=[e['id'] for e in tool['parameters']['entities']]
    assert len(entity_ids)==2 and all(e['type']=='fitSpline' for e in tool['parameters']['entities'])
    call('view.home')
    session=call('surface.trimDialog',{'targetBodyId':target['bodyId']})
    assert session['command']=='surface.trim' and session['selectionRole']=='tool'
    for entity_id in entity_ids:call('surface.trimEntity',{'sketchId':tool['id'],'entityId':entity_id})
    staged=call('app.state')['activeSession']
    rows=staged['regions'];inside=[r for r in rows if r['inside']]
    assert len(rows)==4 and len(inside)==2 and inside[0]['sourceFaceId']!=inside[1]['sourceFaceId']
    assert staged['selectionRole']=='region' and not staged['previewed']
    for row in inside:call('surface.trimRegion',{'regionId':row['regionId']})
    preview=call('command.preview',{'session':session['id']})
    assert preview['previewed'] and preview['geometryEvaluated']
    assert preview['geometry']['projection']['mode']=='sketchNormalThroughTarget'
    call('capture.dialog',{'path':str(out/'trim-controls.png')})
    call('capture.screenshot',{'path':str(out/'trim-red-preview.png')})
    assert call('document.inspect')['authored']==authored
    call('command.cancel',{'session':session['id']})
    assert call('document.inspect')['authored']==authored
    args={'targetBodyId':target['bodyId'],'toolSketchId':tool['id'],'toolEntityIds':entity_ids,
          'removedRegionIds':[r['regionId'] for r in inside],'throughMarginMm':10.0,'name':'Handle openings'}
    create=call('surface.trimDialog',{**args,'removedRegionIds':[]})
    for row in call('app.state')['activeSession']['regions']:
        if row['inside']:call('surface.trimRegion',{'regionId':row['regionId']})
    committed_regions=call('app.state')['activeSession']['inputs']['removedRegionIds']
    assert call('app.state')['activeSession']['previewed']
    call('command.commit',{'session':create['id']})
    args['removedRegionIds']=committed_regions
    current=call('document.inspect');feature=current['authored']['features'][-1];body=next(b for b in current['evaluation']['bodies'] if b['bodyId']==target['bodyId'])
    assert feature['type']=='surfaceTrim' and feature['ownerId']==target['ownerId']
    assert feature['parameters']['projectionMode']=='sketchNormalThroughTarget'
    assert body['valid'] and body['type']=='surface' and body['topology']['faces']==2 and body['topology']['solids']==0
    assert body['areaMm2']<target['areaMm2']-1000
    assert len([e for e in body['solidEdges'] if e['provenance'].get('role')=='trimBoundary'])==4
    face_ids=[f['faceId'] for f in body['supportFaces']]
    call('view.home');call('capture.screenshot',{'path':str(out/'trim-kept-sheet.png')})
    stable=current['authored']
    bad=dict(args,featureId=feature['id'],removedRegionIds=[str(uuid.uuid4())])
    call('surface.trim',bad,error='evaluation_failed')
    assert call('document.inspect')['authored']==stable
    edit=call('feature.editDialog',{'featureId':feature['id']})
    assert edit['previewed'] and edit['inputs']['removedRegionIds']==args['removedRegionIds']
    call('command.cancel',{'session':edit['id']})
    assert call('document.inspect')['authored']==stable
    call('history.undo');assert call('document.inspect')['authored']==authored
    call('history.redo');assert call('document.inspect')['authored']==stable
    edited=copy.deepcopy(tool['parameters']);mid=edited['entities'][0]['pointIds'][1]
    for point in edited['points']:
        if point['id']==mid:point['vMm']+=1
    call('feature.edit',{'featureId':tool['id'],'parameters':edited})
    changed=call('document.inspect');moved=next(b for b in changed['evaluation']['bodies'] if b['bodyId']==target['bodyId'])
    assert [f['faceId'] for f in moved['supportFaces']]==face_ids
    assert abs(moved['areaMm2']-body['areaMm2'])>.01
    final=changed['authored'];model=out/'trimmed-sheet.alloy';call('document.save',{'path':str(model)})
    call('document.new',{'discard':True});call('document.open',{'path':str(model),'discard':True})
    reopened=call('document.inspect');assert reopened['authored']==final
    assert [f['faceId'] for f in next(b for b in reopened['evaluation']['bodies'] if b['bodyId']==target['bodyId'])['supportFaces']]==face_ids
    (out/'result.json').write_text(json.dumps({'passed':True,'featureId':feature['id'],'bodyId':target['bodyId'],
        'toolEntityIds':entity_ids,'removedRegionIds':args['removedRegionIds'],'keptFaceIds':face_ids,
        'beforeAreaMm2':target['areaMm2'],'afterAreaMm2':body['areaMm2'],'editedAreaMm2':moved['areaMm2'],
        'scope':'two-face rail sheet, paired XZ splines; central seam opening on this fixture'},indent=2)+'\n')
    (out/'transcript.json').write_text(json.dumps(ledger,indent=2)+'\n')
