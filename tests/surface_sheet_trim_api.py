#!/usr/bin/env python3
"""SRF-018 bounded planar crossing-sheet Trim through the real application."""
import argparse, copy, json
from pathlib import Path
from app_harness import application

p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--source',required=True);p.add_argument('--output',required=True)
a=p.parse_args();out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
with application(a.app,out) as (call,ledger):
    call('window.resize',{'width':1440,'height':920})
    call('document.open',{'path':str(Path(a.source).resolve()),'discard':True})
    before=call('document.inspect');authored=before['authored']
    target=next(b for b in before['evaluation']['bodies'] if b['name']=='Target sheet')
    tool=next(b for b in before['evaluation']['bodies'] if b['name']=='Tool sheet')
    source=next(f for f in authored['features'] if f['name']=='Tool sheet source')
    body=lambda state,identity:next(b for b in state['evaluation']['bodies'] if b['bodyId']==identity)
    call('view.home')
    session=call('surface.trimDialog',{'targetBodyId':target['bodyId']})
    staged=call('surface.trimBody',{'bodyId':tool['bodyId']})
    rows=staged['regions'];assert len(rows)==2 and {r['side'] for r in rows}=={'negative','positive'}
    assert all(r['removable'] and not r['inside'] and abs(r['areaMm2']-400)<1e-4 for r in rows)
    call('surface.trimRegion',{'regionId':rows[0]['regionId']})
    preview=call('command.preview',{'session':session['id']});assert preview['previewed']
    assert preview['geometry']['projection']['mode']=='surfaceIntersection'
    call('capture.dialog',{'path':str(out/'sheet-trim-controls.png')})
    call('capture.screenshot',{'path':str(out/'sheet-trim-red-preview.png')})
    assert call('document.inspect')['authored']==authored
    call('command.cancel',{'session':session['id']});assert call('document.inspect')['authored']==authored
    session=call('surface.trimDialog',{'targetBodyId':target['bodyId'],'toolBodyId':tool['bodyId']})
    call('surface.trimRegion',{'regionId':session['regions'][0]['regionId']})
    call('command.commit',{'session':session['id']})
    current=call('document.inspect');feature=current['authored']['features'][-1]
    assert feature['parameters']['toolBodyId']==tool['bodyId'] and feature['parameters']['projectionMode']=='surfaceIntersection'
    kept=body(current,target['bodyId']);assert kept['valid'] and kept['topology']['faces']==1 and kept['topology']['solids']==0
    assert abs(kept['areaMm2']-400)<1e-4 and body(current,tool['bodyId'])==tool
    saved=current['authored'];face_id=kept['supportFaces'][0]['faceId'];edges={e['edgeId'] for e in kept['solidEdges']}
    edit=call('feature.editDialog',{'featureId':feature['id']});assert edit['previewed']
    other=next(r for r in edit['regions'] if r['regionId'] not in edit['inputs']['removedRegionIds'])
    call('surface.trimRegion',{'regionId':other['regionId']});assert call('app.state')['activeSession']['previewed']
    assert call('app.state')['activeSession']['inputs']['removedRegionIds']==[other['regionId']]
    call('command.cancel',{'session':edit['id']});assert call('document.inspect')['authored']==saved
    call('history.undo');assert call('document.inspect')['authored']==authored
    call('history.redo');assert call('document.inspect')['authored']==saved
    edited=copy.deepcopy(source['parameters'])
    for point in edited['points']:point['vMm']=2
    call('feature.edit',{'featureId':source['id'],'parameters':edited})
    changed=call('document.inspect');moved=body(changed,target['bodyId'])
    assert moved['supportFaces'][0]['faceId']==face_id and {e['edgeId'] for e in moved['solidEdges']}==edges
    assert abs(moved['areaMm2']-kept['areaMm2'])>10
    assert abs(body(changed,tool['bodyId'])['areaMm2']-tool['areaMm2'])<1e-5
    failed=copy.deepcopy(edited)
    for point in failed['points']:point['vMm']=30
    call('feature.edit',{'featureId':source['id'],'parameters':failed},error='evaluation_failed')
    assert call('document.inspect')['authored']==changed['authored']
    call('view.home');call('capture.screenshot',{'path':str(out/'sheet-trim-edited-result.png')})
    model=out/'planar-sheet-trim.alloy';call('document.save',{'path':str(model)})
    call('document.new',{'discard':True});call('document.open',{'path':str(model),'discard':True})
    reopened=call('document.inspect');assert reopened['authored']==changed['authored']
    assert body(reopened,target['bodyId'])['supportFaces'][0]['faceId']==face_id
    (out/'result.json').write_text(json.dumps({'passed':True,'scope':'one-face planar single-line crossing sheets only',
        'featureId':feature['id'],'targetBodyId':target['bodyId'],'toolBodyId':tool['bodyId'],
        'keptAreaMm2':kept['areaMm2'],'editedAreaMm2':moved['areaMm2'],'calls':len(ledger)},indent=2)+'\n')
    (out/'transcript.json').write_text(json.dumps(ledger,indent=2)+'\n')
