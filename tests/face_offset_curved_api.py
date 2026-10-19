#!/usr/bin/env python3
"""Real-app circular Offset Face edge Fillet and linked Project workflow."""
import argparse
import hashlib
import json
import math
from pathlib import Path

from app_harness import application


def near(actual, expected):
    assert math.isclose(actual, expected, rel_tol=1e-8, abs_tol=1e-5), (actual, expected)


def ref(face):
    return {key: face[key] for key in ('bodyId', 'producerFeatureId', 'faceId')}


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--app', required=True)
parser.add_argument('--output', required=True)
args = parser.parse_args()
out = Path(args.output).resolve()
out.mkdir(parents=True, exist_ok=True)

with application(args.app, out) as (call, transcript):
    def inspect():
        return call('document.inspect')

    def stock():
        call('sketch.begin', {'plane': 'XY', 'name': 'Round stock profile'})
        call('sketch.addCircle', {'centerX': 0, 'centerY': 0,
                                  'diameter': 20, 'units': 'mm'})
        call('sketch.finish')
        sketch = inspect()['evaluation']['sketches'][-1]
        call('solid.extrude', {'sketchId': sketch['sketchId'],
                               'profileId': sketch['profiles'][0]['profileId'],
                               'distance': 10, 'units': 'mm'})
        source = inspect()['evaluation']['bodies'][0]
        near(source['volumeMm3'], 1000 * math.pi)
        top = next(face for face in source['supportFaces'] if face['role'] == 'extrude.endCap')
        rim = next(edge for edge in source['solidEdges']
                   if edge['provenance'].get('role') == 'endRim')
        call('solid.faceOffset', {'targetBodyId': source['bodyId'],
                                  'face': ref(top), 'distanceMm': 5})
        state = inspect()
        body = state['evaluation']['bodies'][0]
        near(body['volumeMm3'], 1500 * math.pi)
        moved = next(face for face in body['supportFaces'] if face['role'] == 'faceOffset.end')
        actual = next(edge for edge in body['solidEdges'] if edge['edgeId'] == rim['edgeId'])
        assert actual['available'] and actual['provenance']['lineage'] == 'uniqueOffsetCircle'
        near(actual['startMm'][2], 15)
        assert len(moved['edges']) == 1 and moved['edges'][0]['type'] == 'circle'
        return source['bodyId'], ref(top), state['authored']['features'][-1]['id'], rim['edgeId'], moved

    def edit(target, source, feature, distance):
        call('solid.faceOffset', {'targetBodyId': target, 'face': source,
                                  'featureId': feature, 'distanceMm': distance})

    def reopen(state, name):
        path = out / name
        call('document.save', {'path': str(path)})
        call('document.new', {'discard': False})
        call('document.open', {'path': str(path)})
        current = inspect()
        assert current['authored'] == state['authored']
        return current

    target, source, feature, rim_id, moved = stock()
    # The support-boundary ID and solid Fillet ID are separate exact catalogs.
    # The circular support Project is linked to the actual moved cap boundary.
    projected_id = moved['edges'][0]['edgeId']
    call('sketch.begin', {'support': ref(moved), 'name': 'Linked circular rim'})
    call('sketch.projectBegin')
    assert call('sketch.projectSelect', {'bodyId': target,
                                         'edgeId': projected_id})['selectedCount'] == 1
    call('sketch.projectCommit')
    projected = call('app.state')['activeSession']['draftSketch']
    links = projected['parameters']['circleProjections']
    assert len(links) == 1 and links[0]['source'] == {
        'bodyId': target, 'edgeId': projected_id}
    call('capture.screenshot', {'path': str(out / 'linked-circular-rim-sketch.png')})
    call('sketch.finish')
    before = inspect()
    before_sketch = next(sketch for sketch in before['evaluation']['sketches']
                         if sketch['sketchId'] == projected['id'])
    near(before_sketch['projectedCircles'][0]['radiusMm'], 10)
    near(before_sketch['worldPoints'][0]['positionMm'][2], 15)
    edit(target, source, feature, 8)
    after = inspect()
    near(after['evaluation']['bodies'][0]['volumeMm3'], 1800 * math.pi)
    moved_again = next(face for face in after['evaluation']['bodies'][0]['supportFaces']
                       if face['role'] == 'faceOffset.end')
    near(moved_again['frame']['origin'][2], 18)
    after_sketch = next(sketch for sketch in after['evaluation']['sketches']
                        if sketch['sketchId'] == projected['id'])
    assert after_sketch['projectedCircles'][0]['source'] == links[0]['source']
    near(after_sketch['projectedCircles'][0]['radiusMm'], 10)
    near(after_sketch['worldPoints'][0]['positionMm'][2], 18)
    call('history.undo')
    assert inspect()['authored'] == before['authored']
    call('history.redo')
    assert inspect()['authored'] == after['authored']
    reopened = reopen(after, 'curved-offset-project.alloy')
    assert reopened['evaluation']['sketches'][-1]['projectedCircles'][0]['source'] == links[0]['source']

    call('document.new', {'discard': True})
    target, source, feature, rim_id, moved = stock()
    call('view.orientation', {'name': 'isometric'})
    call('view.fit')
    session = call('solid.filletDialog', {'targetBodyId': target})
    pixel = call('view.projectEdge', {'bodyId': target, 'edgeId': rim_id})
    picked = call('selection.pick', {'x': pixel['x'], 'y': pixel['y']})
    assert picked['edgeId'] == rim_id, picked
    call('command.update', {'session': session['id'], 'inputs': {'radiusMm': 1}})
    preview = call('command.preview', {'session': session['id']})
    assert preview['geometry']['valid'], preview
    call('capture.screenshot', {'path': str(out / 'curved-rim-fillet-preview.png')})
    call('command.commit', {'session': session['id']})
    filleted = inspect()
    assert filleted['evaluation']['bodies'][0]['volumeMm3'] < 1500 * math.pi
    edit(target, source, feature, 8)
    changed = inspect()
    assert changed['evaluation']['bodies'][0]['valid']
    near(changed['evaluation']['bodies'][0]['volumeMm3']
         - filleted['evaluation']['bodies'][0]['volumeMm3'], 300 * math.pi)
    near(changed['evaluation']['bodies'][0]['boundsMm']['max'][2], 18)
    call('history.undo')
    assert inspect()['authored'] == filleted['authored']
    call('history.redo')
    assert inspect()['authored'] == changed['authored']
    reopened = reopen(changed, 'curved-offset-fillet.alloy')
    assert reopened['evaluation']['bodies'][0]['valid']
    near(reopened['evaluation']['bodies'][0]['volumeMm3'],
         changed['evaluation']['bodies'][0]['volumeMm3'])
    call('capture.screenshot', {'path': str(out / 'curved-rim-fillet-reopened.png')})
    gpu = call('app.state')['viewport']['gpu']
    assert gpu['hardwareAccelerated'], gpu
    (out / 'receipt.json').write_text(json.dumps({
        'status': 'passed', 'scope': 'one circular cap rim through planar Offset Face',
        'appSha256': hashlib.sha256(Path(args.app).read_bytes()).hexdigest(),
        'gpu': gpu, 'projectedSource': links[0]['source'],
        'filletEdgeId': rim_id, 'filletPick': picked,
        'exactAuthoredUndoRedoReopen': True, 'calls': len(transcript)
    }, indent=2) + '\n')
    (out / 'transcript.json').write_text(json.dumps(transcript, indent=2) + '\n')

print('PASS: curved Offset Face rim, linked Project and Fillet through source edit')
