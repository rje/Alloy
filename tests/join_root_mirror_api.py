#!/usr/bin/env python3
"""Shared-app check of exact reflected Join attachment-root ancestry."""
import argparse
import json
from pathlib import Path
from app_harness import application

p = argparse.ArgumentParser()
p.add_argument('--app', required=True)
p.add_argument('--source', required=True)
p.add_argument('--output', required=True)
a = p.parse_args()
out = Path(a.output).resolve()

def root(state, target, mirror_id):
    body = next(b for b in state['evaluation']['bodies'] if b['bodyId'] == target)
    roots = [e for e in body['solidEdges'] if e['available']
             and e['provenance'].get('role') == 'join.root'
             and e['provenance'].get('mirrorFeatureId') == mirror_id]
    assert len(roots) == 1, roots
    r = roots[0]
    provenance = r['provenance']
    assert provenance.get('sourceToolEdgeId') and provenance.get('targetFaceId'), r
    assert provenance.get('lineage') == 'exactModifiedBaseRimTargetIncidence', r
    return r['edgeId']

with application(a.app, out, command_timeout=120) as (call, ledger):
    call('window.resize', {'width': 1440, 'height': 900})
    call('document.open', {'path': str(Path(a.source).resolve())})
    source = call('document.inspect')
    group = source['authored']['features'][-5:]
    assert [f['type'] for f in group] == ['extrude', 'chamfer', 'chamfer', 'chamfer', 'fillet']
    call('component.activate', {'definitionId': group[0]['ownerId']})
    target = group[0]['parameters']['targetBodyIds'][0]
    args = {'targetBodyId': target, 'sourceFeatureIds': [f['id'] for f in group],
            'mirrorPlane': {'kind': 'origin', 'plane': 'YZ'}}
    preview = call('solid.featureMirrorDialog', args)
    assert preview['previewed'] and preview['geometryEvaluated'], preview
    call('command.cancel', {'session': preview['id']})
    assert call('document.inspect')['authored'] == source['authored']
    call('solid.featureMirror', args)
    accepted = call('document.inspect')
    mirror_id = accepted['authored']['features'][-1]['id']
    root_id = root(accepted, target, mirror_id)
    call('capture.screenshot', {'path': str(out/'reflected-root.png')})
    path = out/'reflected-root.alloy'
    call('document.save', {'path': str(path)})
    call('document.open', {'path': str(path)})
    reopened = call('document.inspect')
    assert reopened['authored'] == accepted['authored']
    assert root(reopened, target, mirror_id) == root_id
    (out/'report.json').write_text(json.dumps({'passed': True, 'mirrorFeatureId': mirror_id,
        'reflectedRootEdgeId': root_id, 'scope': 'technical finish group, source Chamfer2 excluded'}, indent=2))
    (out/'transcript.json').write_text(json.dumps(ledger, indent=2))
