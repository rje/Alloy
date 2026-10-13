#!/usr/bin/env python3
"""Focused real-entrypoint Hole chain from a retained supported sketch Point."""
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

    def body():
        return inspect()['evaluation']['bodies'][0]

    call('sketch.begin', {'plane': 'XY'})
    call('sketch.centerRectangle', {'centerX': 0, 'centerY': 0, 'width': 75, 'height': 50, 'units': 'mm'})
    base = call('app.state')['activeSession']['draftSketch']
    width_id = base['parameters']['dimensions'][0]['id']
    call('sketch.finish')
    profile = inspect()['evaluation']['sketches'][0]['profiles'][0]['profileId']
    call('solid.extrude', {'sketchId': base['id'], 'profileId': profile, 'distance': 10, 'units': 'mm'})
    target = body()['bodyId']
    edges = [edge['edgeId'] for edge in body()['solidEdges'] if edge['provenance']['role'] == 'sideCorner']
    assert len(edges) == 4
    call('solid.fillet', {'targetBodyId': target, 'edgeIds': edges, 'radiusMm': 5})
    cap = next(face for face in body()['supportFaces'] if face['role'] == 'extrude.endCap')
    support = {key: cap[key] for key in ('producerFeatureId', 'bodyId', 'faceId')}
    call('sketch.begin', {'support': support})
    call('sketch.addPoint', {'x': -27.5, 'y': 15, 'units': 'mm'})
    point_sketch = call('app.state')['activeSession']['draftSketch']
    point = point_sketch['parameters']['entities'][0]
    lines = call('app.state')['activeSession']['projectionContext']['lines']
    left = next(line for line in lines if abs(line['u1Mm'] + 37.5) < 1e-6 and abs(line['u2Mm'] + 37.5) < 1e-6)
    upper = next(line for line in lines if abs(line['v1Mm'] - 25) < 1e-6 and abs(line['v2Mm'] - 25) < 1e-6)
    for line in (left, upper):
        call('sketch.dimension', {'kind': 'pointLineDistance', 'pointIds': [point['pointId']],
            'entityId': line['id'], 'value': 10, 'units': 'mm'})
    call('sketch.finish')
    before = inspect()
    inputs = {'sourceSketchId': point_sketch['id'], 'sourceEntityId': point['id'],
        'sourcePointId': point['pointId'], 'targetBodyId': target,
        'placement': 'fromSketch', 'holeType': 'countersink', 'extent': 'all',
        'boreDiameterMm': 4, 'sinkDiameterMm': 10, 'includedAngleDeg': 90}
    call('solid.hole', inputs)
    created = inspect()
    feature = created['authored']['features'][-1]
    assert feature['type'] == 'hole' and feature['parameters'] == inputs
    assert body()['bodyId'] == target and body()['valid'] and body()['closed']
    assert math.isclose(body()['removedVolumeMm3'], 67*math.pi, abs_tol=1e-5)
    assert math.isclose(body()['sinkDepthMm'], 3, abs_tol=1e-8)
    assert math.isclose(body()['volumeMm3'], 36500 + 183*math.pi, abs_tol=1e-5)
    call('view.orientation', {'name': 'top'})
    call('capture.screenshot', {'path': str(out/'seed-hole.png')})
    call('history.undo')
    assert inspect()['authored'] == before['authored']
    call('history.redo')
    assert inspect()['authored'] == created['authored']
    call('sketch.edit', {'sketchId': base['id']})
    call('sketch.dimensionUpdate', {'dimensionId': width_id, 'value': 85, 'units': 'mm'})
    call('sketch.finish')
    assert math.isclose(body()['entryMm'][0], -32.5, abs_tol=1e-6)
    saved = inspect()
    path = out/'seed-hole.alloy'
    call('document.save', {'path': str(path)})
    call('document.open', {'path': str(path)})
    assert inspect()['authored'] == saved['authored']
    assert math.isclose(body()['entryMm'][0], -32.5, abs_tol=1e-6)
    (out/'report.json').write_text(json.dumps({'passed': True, 'source': 'plate-countersink-pattern-20261013',
        'scope': 'shared API Hole ingredient, not native gesture or full plate pattern',
        'volumeMm3': body()['volumeMm3'], 'authored': inspect()['authored']}, indent=2))
    (out/'transcript.json').write_text(json.dumps(transcript, indent=2))
