#!/usr/bin/env python3
"""Real-app retained Offset Face boundary Project and downstream Fillet gate."""
import argparse
import hashlib
import json
import math
from pathlib import Path

from app_harness import application


def near(actual, expected):
    assert math.isclose(actual, expected, rel_tol=1e-8, abs_tol=1e-5), (actual, expected)


def support(face):
    return {key: face[key] for key in ('producerFeatureId', 'bodyId', 'faceId')}


def endpoints(edge):
    source = edge.get('provenance', edge)
    return [source['startMm'], source['endMm']]


def same_endpoints(actual, expected):
    assert min(max(math.dist(a, b) for a, b in zip(actual, ordered))
               for ordered in (expected, list(reversed(expected)))) < 1e-5, (actual, expected)


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--app', required=True)
parser.add_argument('--output', required=True)
args = parser.parse_args()
out = Path(args.output).resolve()
out.mkdir(parents=True, exist_ok=True)

with application(args.app, out) as (call, transcript):
    def inspect():
        return call('document.inspect')

    def body(state):
        result = state['evaluation']['bodies'][0]
        assert result['valid'] and result['closed']
        return result

    def offset_fixture():
        call('solid.box', {'width': 40, 'depth': 30, 'height': 20,
                           'units': 'mm', 'name': 'Edge reference stock'})
        stock = body(inspect())
        top = next(f for f in stock['supportFaces'] if f['role'] == 'box.z1')
        source = support(top)
        call('solid.faceOffset', {'targetBodyId': stock['bodyId'],
                                 'face': source, 'distanceMm': 5})
        state = inspect()
        moved = body(state)
        feature = state['authored']['features'][-1]
        cap = next(f for f in moved['supportFaces']
                   if f['producerFeatureId'] == feature['id'])
        # Choose the exact front/top boundary geometrically, never a catalog index.
        candidates = [edge for edge in cap['edges']
                      if all(abs(p[1]) < 1e-5 and abs(p[2] - 25) < 1e-5
                             for p in endpoints(edge))]
        assert len(candidates) == 1, candidates
        boundary = candidates[0]
        named = [e for e in moved['solidEdges']
                 if e['edgeId'] == boundary['edgeId'] and e['available']]
        assert len(named) == 1, (boundary, moved['solidEdges'])
        same_endpoints(endpoints(named[0]), endpoints(boundary))
        near(math.dist(*endpoints(named[0])), 40)
        return moved['bodyId'], source, feature['id'], support(cap), named[0]

    def edit_offset(target, source, feature, distance):
        call('solid.faceOffset', {'featureId': feature, 'targetBodyId': target,
                                 'face': source, 'distanceMm': distance})

    def history_roundtrip(before, after):
        call('history.undo')
        assert inspect()['authored'] == before['authored']
        call('history.redo')
        assert inspect()['authored'] == after['authored']

    def reopen(state, filename):
        path = out / filename
        call('document.save', {'path': str(path)})
        call('document.new', {'discard': False})
        call('document.open', {'path': str(path)})
        result = inspect()
        assert result['authored'] == state['authored']
        near(body(result)['volumeMm3'], body(state)['volumeMm3'])
        return result

    target, source, offset_id, cap, edge = offset_fixture()
    call('solid.box', {'width': 45, 'depth': 5, 'height': 10,
                       'x': -2, 'y': -10, 'z': 0, 'units': 'mm', 'name': 'Projection host'})
    host = next(b for b in inspect()['evaluation']['bodies'] if b['name'] == 'Projection host')
    host_front = next(f for f in host['supportFaces'] if f['role'] == 'box.y0')
    call('sketch.begin', {'support': support(host_front), 'name': 'Retained moved boundary'})
    call('sketch.projectBegin')
    selected = call('sketch.projectSelect', {'bodyId': target, 'edgeId': edge['edgeId']})
    assert selected['selectedCount'] == 1
    call('sketch.projectCommit')
    draft = call('app.state')['activeSession']['draftSketch']
    projections = draft['parameters']['projections']
    matching = [p for p in projections if p['source'].get('edgeId') == edge['edgeId']]
    assert len(matching) == 1, matching
    projection = matching[0]
    assert projection['source'] == {'bodyId': target, 'edgeId': edge['edgeId']}
    call('capture.screenshot', {'path': str(out / 'linked-boundary-active-sketch.png')})
    call('sketch.finish')

    def projected_endpoints(state, height):
        sketch = next(s for s in state['evaluation']['sketches'] if s['sketchId'] == draft['id'])
        line = next(l for l in sketch['projectedLines'] if l['id'] == projection['id'])
        assert line['source'] == projection['source']
        points = {p['id']: p['positionMm'] for p in sketch['worldPoints']}
        ends = [points[line['startPointId']], points[line['endPointId']]]
        same_endpoints(ends, [[0, -10, height], [40, -10, height]])
        retained = next(f for f in state['authored']['features'] if f['id'] == draft['id'])
        assert retained['parameters']['projections'] == projections
        return ends

    projected_before = inspect()
    before_ends = projected_endpoints(projected_before, 25)
    call('view.orientation', {'name': 'isometric'})
    call('view.fit')
    call('capture.screenshot', {'path': str(out / 'linked-boundary-before.png')})
    edit_offset(target, source, offset_id, 8)
    projected_after = inspect()
    after_ends = projected_endpoints(projected_after, 28)
    near(body(projected_after)['volumeMm3'], 40 * 30 * 28)
    history_roundtrip(projected_before, projected_after)
    projected_endpoints(inspect(), 28)
    projected_reopened = reopen(projected_after, 'offset-linked-boundary.alloy')
    projected_endpoints(projected_reopened, 28)
    call('capture.screenshot', {'path': str(out / 'linked-boundary-reopened.png')})

    # Separate fixture prevents the sketch/support from influencing the Fillet route.
    call('document.new', {'discard': True})
    target, source, offset_id, cap, edge = offset_fixture()
    call('view.orientation', {'name': 'isometric'})
    call('view.fit')
    session = call('solid.filletDialog', {'targetBodyId': target})
    point = call('view.projectEdge', {'bodyId': target, 'edgeId': edge['edgeId']})
    assert math.isfinite(point['x']) and math.isfinite(point['y'])
    picked = call('selection.pick', {'x': point['x'], 'y': point['y']})
    assert picked['edgeId'] == edge['edgeId'], picked
    call('command.update', {'session': session['id'], 'inputs': {'radiusMm': 1}})
    preview = call('command.preview', {'session': session['id']})
    assert preview['geometry']['valid'], preview
    call('capture.screenshot', {'path': str(out / 'offset-edge-fillet-preview.png')})
    call('command.commit', {'session': session['id']})
    filleted_before = inspect()
    fillet = filleted_before['authored']['features'][-1]
    assert fillet['type'] == 'fillet'
    assert edge['edgeId'] in fillet['parameters']['edgeIds']
    near(body(filleted_before)['boundsMm']['size'][2], 25)
    assert 0 < body(filleted_before)['volumeMm3'] < 40 * 30 * 25
    edit_offset(target, source, offset_id, 8)
    filleted_after = inspect()
    assert next(f for f in filleted_after['authored']['features'] if f['id'] == fillet['id']) == fillet
    near(body(filleted_after)['boundsMm']['size'][2], 28)
    # Translating this one horizontal round changes only the added stock volume.
    near(body(filleted_after)['volumeMm3'] - body(filleted_before)['volumeMm3'], 40 * 30 * 3)
    history_roundtrip(filleted_before, filleted_after)
    filleted_reopened = reopen(filleted_after, 'offset-edge-fillet.alloy')
    near(body(filleted_reopened)['boundsMm']['size'][2], 28)
    call('view.fit')
    call('capture.screenshot', {'path': str(out / 'offset-edge-fillet-reopened.png')})
    gpu = call('app.state')['viewport']['gpu']
    assert gpu['hardwareAccelerated'], gpu
    (out / 'receipt.json').write_text(json.dumps({
        'status': 'passed', 'scope': 'single planar offset, retained straight boundary Project and one Fillet',
        'appSha256': hashlib.sha256(Path(args.app).read_bytes()).hexdigest(),
        'gpu': gpu, 'projection': projection, 'projectedEndpointsBeforeMm': before_ends,
        'projectedEndpointsAfterMm': after_ends, 'projectionBefore': projected_before,
        'projectionAfter': projected_after, 'filletEdge': edge, 'pickedEdge': picked,
        'filletBefore': filleted_before, 'filletAfter': filleted_after,
        'exactAuthoredUndoRedoReopen': True,
        'nativePointerInput': 'separate validation gate', 'calls': len(transcript)
    }, indent=2) + '\n')
    (out / 'transcript.json').write_text(json.dumps(transcript, indent=2) + '\n')

print('PASS: retained Offset Face edge Project and Fillet, source edit, history/reopen and GPU')
