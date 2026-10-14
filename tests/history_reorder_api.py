#!/usr/bin/env python3
"""Real-app retained Timeline reorder on a supplied post-Mirror enclosure prefix."""
import argparse
import json
from pathlib import Path
from app_harness import application

p=argparse.ArgumentParser()
p.add_argument('--app',required=True)
p.add_argument('--source',required=True)
p.add_argument('--fillet-id',required=True)
p.add_argument('--after-feature-id',required=True)
p.add_argument('--opposite-edge-id',required=True)
p.add_argument('--output',required=True)
a=p.parse_args()
out=Path(a.output).resolve()

def by_id(features,identity):
    return next(f for f in features if f['id']==identity)

with application(a.app,out) as (call,ledger):
    call('window.resize',{'width':1440,'height':900})
    assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
    call('document.open',{'path':str(Path(a.source).resolve())})
    before=call('document.inspect')
    features=before['authored']['features']
    fillet=by_id(features,a.fillet_id)
    anchor=by_id(features,a.after_feature_id)
    assert fillet['type']=='fillet' and anchor['type']=='featureMirror'
    first=next(i for i,f in enumerate(features) if f['id']==a.fillet_id)
    last=next(i for i,f in enumerate(features) if f['id']==a.after_feature_id)
    assert first<last and sum(f['type']=='featureMirror' for f in features[first+1:last+1])>=2
    old_order=[f['id'] for f in features]
    moved=call('feature.reorder',{'featureId':a.fillet_id,'afterFeatureId':a.after_feature_id})
    assert moved['changed'] and moved['movedFeatureId']==a.fillet_id
    current=call('document.inspect')
    new_order=[f['id'] for f in current['authored']['features']]
    assert new_order==moved['featureOrder'] and set(new_order)==set(old_order)
    assert new_order.index(a.fillet_id)==new_order.index(a.after_feature_id)+1
    assert by_id(current['authored']['features'],a.fillet_id)==fillet
    assert call('feature.reorder',{'featureId':a.fillet_id,
        'afterFeatureId':a.after_feature_id})['changed'] is False
    prior_edge=fillet['parameters']['edgeIds'][0]
    body=fillet['parameters']['targetBodyId']
    assert prior_edge!=a.opposite_edge_id
    call('solid.fillet',{'featureId':a.fillet_id,'targetBodyId':body,
        'edgeIds':[prior_edge,a.opposite_edge_id],'radiusMm':0.5})
    edited=call('document.inspect')
    assert by_id(edited['authored']['features'],a.fillet_id)['parameters']['edgeIds']==[prior_edge,a.opposite_edge_id]
    assert all(b['valid'] and b['closed'] for b in edited['evaluation']['bodies'])
    dependent=anchor['parameters']['sourceFeatureIds'][0]
    call('feature.reorder',{'featureId':dependent,'afterFeatureId':a.after_feature_id},error='evaluation_failed')
    assert call('document.inspect')['authored']==edited['authored']
    call('document.save',{'path':str(out/'reordered-enclosure.alloy')})
    call('history.undo');assert call('document.inspect')['authored']==current['authored']
    call('history.undo');assert call('document.inspect')['authored']==before['authored']
    call('history.redo');call('history.redo')
    assert call('document.inspect')['authored']==edited['authored']
    call('document.open',{'path':str(out/'reordered-enclosure.alloy')})
    reopened=call('document.inspect')
    assert reopened['authored']==edited['authored']
    assert all(b['valid'] and b['closed'] for b in reopened['evaluation']['bodies'])
    call('capture.screenshot',{'path':str(out/'reordered-enclosure.png')})
    (out/'report.json').write_text(json.dumps({'passed':True,'oldOrder':old_order,
        'newOrder':new_order,'filletId':a.fillet_id,'anchorId':a.after_feature_id,
        'originalEdgeId':prior_edge,'oppositeEdgeId':a.opposite_edge_id},indent=2))
    (out/'transcript.json').write_text(json.dumps(ledger,indent=2))
