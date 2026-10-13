#!/usr/bin/env python3
"""Real-app bounded straight rim lineage; circle center is not a Point tool."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application

parser = argparse.ArgumentParser()
parser.add_argument('--app', required=True)
parser.add_argument('--output', required=True)
args = parser.parse_args()
out = Path(args.output).resolve()
with application(args.app, out) as (call, transcript):
    def inspect():
        return call('document.inspect')
    def draft():
        return call('app.state')['activeSession']['draftSketch']
    def body():
        return inspect()['evaluation']['bodies'][0]
    def top(state):
        return next(f for f in state['supportFaces'] if f['role'] == 'extrude.endCap')
    def verify(width, radius):
        state = body()
        assert state['valid'] and state['closed']
        assert math.isclose(state['volumeMm3'], (width*50-(4-math.pi)*radius**2)*10, abs_tol=1e-6)
        edges = top(state)['edges']
        assert len(edges) == 4
        for edge in edges:
            assert edge['lineage'] == 'uniqueModifiedStraight'
            assert edge['endpointProvenance'] == 'currentKernelBoundary'
            assert 'startContact' not in edge and 'endContact' not in edge
            assert edge['sourceBoundaryContacts']
            a, b = edge['startMm'], edge['endMm']
            expected = width-2*radius if abs(a[0]-b[0]) > 1e-6 else 50-2*radius
            assert math.isclose(math.dist(a, b), expected, abs_tol=1e-6)
            assert a[2] == b[2] == 10
        named = [e for e in state['solidEdges'] if e['available'] and e['provenance']['role'] == 'endRim']
        assert len(named) == 4
        assert all(e['provenance']['endpointProvenance'] == 'currentKernelBoundary' for e in named)
        return state
    call('sketch.begin', {'plane': 'XY'})
    call('sketch.centerRectangle', {'centerX': 0, 'centerY': 0, 'width': 75, 'height': 50, 'units': 'mm'})
    base = draft()
    base_id = base['id']
    width_id = base['parameters']['dimensions'][0]['id']
    call('sketch.finish')
    profile_id = inspect()['evaluation']['sketches'][0]['profiles'][0]['profileId']
    call('solid.extrude', {'sketchId': base_id, 'profileId': profile_id, 'distance': 10, 'units': 'mm'})
    stock = body()
    target = stock['bodyId']
    seeds = [e['edgeId'] for e in stock['solidEdges'] if e['provenance']['role'] == 'sideCorner']
    assert len(seeds) == 4
    call('solid.fillet', {'targetBodyId': target, 'edgeIds': seeds, 'radiusMm': 5})
    fillet_id = inspect()['authored']['features'][-1]['id']
    rounded = verify(75, 5)
    named = next(e for e in rounded['solidEdges'] if e['available'] and e['provenance']['role'] == 'endRim')
    call('view.home')
    # Live projection proves the retained direction identity is usable by the
    # viewport; no future Pattern implementation or physical-input claim.
    projected = call('view.projectEdge', {'bodyId': target, 'edgeId': named['edgeId']})
    assert math.isfinite(projected['x']) and math.isfinite(projected['y'])
    call('capture.screenshot', {'path': str(out/'rounded-plate.png')})
    support = {k: top(rounded)[k] for k in ('bodyId', 'producerFeatureId', 'faceId')}
    call('sketch.begin', {'support': support})
    call('sketch.addCircle', {'centerX': -27.5, 'centerY': 15, 'diameter': 2, 'units': 'mm'})
    hole = draft()
    circle = hole['parameters']['entities'][0]
    point_id = circle['centerPointId']
    lines = call('app.state')['activeSession']['projectionContext']['lines']
    left = next(l for l in lines if abs(l['u1Mm']+37.5) < 1e-6 and abs(l['u2Mm']+37.5) < 1e-6)
    upper = next(l for l in lines if abs(l['v1Mm']-25) < 1e-6 and abs(l['v2Mm']-25) < 1e-6)
    for line in (left, upper):
        call('sketch.dimension', {'kind': 'pointLineDistance', 'pointIds': [point_id], 'entityId': line['id'], 'value': 10, 'units': 'mm'})
    call('sketch.dimension', {'kind': 'radius', 'entityId': circle['id'], 'value': 1, 'units': 'mm'})
    call('view.orientation', {'name': 'top'})
    call('capture.screenshot', {'path': str(out/'two-edge-dimensions.png')})
    hole = draft()
    call('sketch.finish')
    before = inspect()
    def center(x):
        points = inspect()['evaluation']['sketches'][-1]['worldPoints']
        p = next(p for p in points if p['id'] == point_id)['positionMm']
        assert math.dist(p, [x, 15, 10]) < 1e-6
    center(-27.5)
    call('sketch.edit', {'sketchId': base_id})
    call('sketch.dimensionUpdate', {'dimensionId': width_id, 'value': 85, 'units': 'mm'})
    call('sketch.finish')
    verify(85, 5)
    center(-32.5)
    call('solid.fillet', {'featureId': fillet_id, 'targetBodyId': target, 'edgeIds': seeds, 'radiusMm': 4})
    verify(85, 4)
    center(-32.5)
    after = inspect()
    retained = next(f for f in after['authored']['features'] if f['id'] == hole['id'])['parameters']
    assert retained['dimensions'] == hole['parameters']['dimensions']
    assert retained['projections'] == hole['parameters']['projections']
    for _ in range(2):
        call('history.undo')
    assert inspect()['authored'] == before['authored']
    center(-27.5)
    for _ in range(2):
        call('history.redo')
    assert inspect()['authored'] == after['authored']
    revision = inspect()['revision']
    call('solid.fillet', {'featureId': fillet_id, 'targetBodyId': target, 'edgeIds': seeds, 'radiusMm': 30}, error='evaluation_failed')
    assert inspect()['authored'] == after['authored'] and inspect()['revision'] == revision
    call('solid.fillet', {'featureId': fillet_id, 'targetBodyId': target, 'edgeIds': [left['source']['edgeId']], 'radiusMm': 4}, error='evaluation_failed')
    assert inspect()['authored'] == after['authored'] and inspect()['revision'] == revision
    path = out/'rim-dimensions.alloy'
    call('document.save', {'path': str(path)})
    call('document.open', {'path': str(path)})
    assert inspect()['authored'] == after['authored']
    verify(85, 4)
    center(-32.5)
    call('sketch.edit', {'sketchId': hole['id']})
    call('view.orientation', {'name': 'top'})
    call('capture.screenshot', {'path': str(out/'reopened-edited-dimensions.png')})
    call('sketch.cancel')
    app_state = call('app.state')
    (out/'report.json').write_text(json.dumps({'passed': True, 'scope': 'unique straight rim lineage via circle-center dimension foundation, not standalone Point or complete tutorial', 'before': before, 'after': after, 'appState': app_state}, indent=2))
    (out/'transcript.json').write_text(json.dumps(transcript, indent=2))
