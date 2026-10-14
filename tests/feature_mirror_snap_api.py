#!/usr/bin/env python3
"""Real-app retained Join/three-Chamfer/Fillet feature Mirror ingredient."""
import argparse
import json
from pathlib import Path
from app_harness import application

p=argparse.ArgumentParser()
p.add_argument('--app',required=True)
p.add_argument('--source',required=True)
p.add_argument('--output',required=True)
a=p.parse_args()
out=Path(a.output).resolve()

def target_body(state,target):
    body=next(b for b in state['evaluation']['bodies'] if b['bodyId']==target)
    assert body['valid'] and body['closed'],body
    return body

with application(a.app,out) as (call,ledger):
    call('window.resize',{'width':1440,'height':900})
    assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
    call('document.open',{'path':str(Path(a.source).resolve())})
    source=call('document.inspect')
    features=source['authored']['features']
    group=features[-5:]
    assert [f['type'] for f in group]==['extrude','chamfer','chamfer','chamfer','fillet']
    target=group[0]['parameters']['targetBodyIds'][0]
    ids=[f['id'] for f in group]
    start=target_body(source,target)['volumeMm3']
    args={'targetBodyId':target,'sourceFeatureIds':ids,
          'mirrorPlane':{'kind':'origin','plane':'XZ'}}
    preview=call('solid.featureMirrorDialog',args)
    assert preview['previewed'] and preview['geometryEvaluated'],preview
    call('capture.dialog',{'path':str(out/'api-snap-mirror-panel.png')})
    call('capture.screenshot',{'path':str(out/'api-snap-mirror-preview.png')})
    call('command.cancel',{'session':preview['id']})
    assert call('document.inspect')['authored']==source['authored']
    call('solid.featureMirror',args)
    result=call('document.inspect')
    mirror=result['authored']['features'][-1]
    assert mirror['type']=='featureMirror' and mirror['parameters']['sourceFeatureIds']==ids
    mirrored=target_body(result,target)
    assert mirrored['volumeMm3']>start+150 and mirrored['volumeMm3']<start+163,mirrored
    assert any(e['available'] and e['provenance'].get('mirrorFeatureId')==mirror['id']
               for e in mirrored['solidEdges'])
    edit=call('feature.editDialog',{'featureId':mirror['id']})
    assert edit['previewed'] and edit['geometryEvaluated'],edit
    call('command.cancel',{'session':edit['id']})
    assert call('document.inspect')['authored']==result['authored']
    call('document.save',{'path':str(out/'snap-mirrored.alloy')})
    call('history.undo')
    assert abs(target_body(call('document.inspect'),target)['volumeMm3']-start)<1e-5
    call('history.redo')
    assert call('document.inspect')['authored']==result['authored']
    call('document.open',{'path':str(out/'snap-mirrored.alloy')})
    reopened=call('document.inspect')
    assert reopened['authored']==result['authored']
    assert abs(target_body(reopened,target)['volumeMm3']-mirrored['volumeMm3'])<1e-5
    call('capture.screenshot',{'path':str(out/'api-snap-mirror-accepted.png')})
    (out/'report.json').write_text(json.dumps({'passed':True,'sourceGroupIds':ids,
       'mirrorFeatureId':mirror['id'],'sourceVolumeMm3':start,
       'mirroredVolumeMm3':mirrored['volumeMm3'],'scope':'one-prism Join and four retained finishes'},indent=2))
    (out/'transcript.json').write_text(json.dumps(ledger,indent=2))
