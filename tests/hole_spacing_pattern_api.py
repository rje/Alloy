#!/usr/bin/env python3
"""Focused real-app Hole Spacing/Adjust and padded-target oracle."""
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
    call('sketch.finish')
    profile = inspect()['evaluation']['sketches'][0]['profiles'][0]['profileId']
    call('solid.extrude', {'sketchId': base['id'], 'profileId': profile, 'distance': 10, 'units': 'mm'})
    target = body()['bodyId']
    corners = [edge['edgeId'] for edge in body()['solidEdges'] if edge['provenance']['role'] == 'sideCorner']
    assert len(corners) == 4
    call('solid.fillet', {'targetBodyId': target, 'edgeIds': corners, 'radiusMm': 5})
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
    call('solid.hole', {'sourceSketchId': point_sketch['id'], 'sourceEntityId': point['id'],
        'sourcePointId': point['pointId'], 'targetBodyId': target, 'placement': 'fromSketch',
        'holeType': 'countersink', 'extent': 'all', 'boreDiameterMm': 4,
        'sinkDiameterMm': 10, 'includedAngleDeg': 90})
    seed = inspect()['authored']['features'][-1]
    def axis(index, spacing):
        for edge in body()['solidEdges']:
            if not edge['available'] or edge['provenance']['role'] != 'endRim':
                continue
            start, end = edge['startMm'], edge['endMm']
            if abs(start[2]-10)>1e-6 or abs(end[2]-10)>1e-6:
                continue
            vector = [end[i]-start[i] for i in range(3)]
            if abs(vector[index]) <= 1 or any(abs(vector[i])>1e-6 for i in range(3) if i != index):
                continue
            signed = math.copysign(spacing, vector[index]) if index == 0 else -math.copysign(spacing, vector[index])
            return {'edgeId': edge['edgeId'], 'quantity': 3, 'spacingMm': signed, 'direction': 'one'}
        raise AssertionError('Missing retained oriented top rim')
    axes = [axis(0, 27.5), axis(1, 15)]
    inputs = {'sourceFeatureId': seed['id'], 'targetBodyId': target,
        'distribution': 'spacing', 'computeType': 'adjust', 'axes': axes}
    before = inspect()
    call('solid.rectangularPattern', inputs)
    patterned = inspect()
    pattern = patterned['authored']['features'][-1]
    geometry = body()
    assert pattern['parameters'] == inputs and geometry['instanceCount'] == 9
    assert geometry['valid'] and geometry['closed']
    assert math.isclose(geometry['volumeMm3'], 36500-353*math.pi, abs_tol=1e-5)
    centers = sorted(tuple(round(v, 6) for v in instance['centerMm']) for instance in geometry['instances'])
    assert centers == sorted((x,y,10) for x in (-27.5,0,27.5) for y in (15,0,-15))
    call('view.orientation', {'name': 'top'})
    call('capture.screenshot', {'path': str(out/'nine-hole-plate.png')})
    call('history.undo')
    assert inspect()['authored'] == before['authored']
    call('history.redo')
    assert inspect()['authored'] == patterned['authored']
    call('history.undo')
    call('sketch.begin', {'plane': 'XY'})
    call('sketch.rectangle', {'x1': -7, 'y1': -25, 'x2': 7, 'y2': 25, 'units': 'mm'})
    call('sketch.finish')
    pad = inspect()['evaluation']['sketches'][-1]
    call('solid.extrude', {'sketchId': pad['sketchId'], 'profileId': pad['profiles'][0]['profileId'],
        'distance': -3, 'units': 'mm', 'operation': 'join', 'targetBodyIds': [target]})
    call('solid.rectangularPattern', inputs)
    padded = inspect()
    assert padded['evaluation']['bodies'][0]['bodyId'] == target
    assert math.isclose(body()['volumeMm3'], 36500-353*math.pi+2100-36*math.pi, abs_tol=1e-5)
    call('view.home')
    call('capture.screenshot', {'path': str(out/'padded-nine-hole-plate.png')})
    path = out/'padded-nine-hole-plate.alloy'
    call('document.save', {'path': str(path)})
    call('document.open', {'path': str(path)})
    assert inspect()['authored'] == padded['authored']
    (out/'report.json').write_text(json.dumps({'passed': True, 'scope': 'Hole Spacing/Adjust API ingredient, not native or full parameterized tutorial',
        'unpadVolumeMm3': geometry['volumeMm3'], 'paddedVolumeMm3': body()['volumeMm3'],
        'authored': inspect()['authored']}, indent=2))
    (out/'transcript.json').write_text(json.dumps(transcript, indent=2))
