#!/usr/bin/env python3
"""Focused real-entrypoint Hole chain from a retained supported sketch Point."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl

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
    extrusion_id = inspect()['authored']['features'][-1]['id']
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
    call('sketch.addPoint', {'x': -15, 'y': 10, 'units': 'mm'})
    second = call('app.state')['activeSession']['draftSketch']['parameters']['entities'][-1]
    call('sketch.finish')
    before = inspect()
    inputs = {'sourceSketchId': point_sketch['id'], 'sourceEntityId': point['id'],
        'sourcePointId': point['pointId'], 'targetBodyId': target,
        'placement': 'fromSketch', 'holeType': 'counterbore', 'extent': 'distance',
        'boreDiameterMm': 4, 'distanceMm': 5, 'recessDiameterMm': 10, 'recessDepthMm': 2}
    session = call('solid.holeDialog', inputs)
    sid = session['id']
    assert session['previewed'] and math.isclose(session['geometry']['removedVolumeMm3'], 62*math.pi, abs_tol=1e-5)
    call('view.orientation', {'name': 'isometric'})
    call('capture.screenshot', {'path': str(out/'blind-preview.png')})
    call('command.cancel', {'session': sid})
    assert inspect()['authored'] == before['authored']
    call('solid.hole', inputs)
    created = inspect()
    feature = created['authored']['features'][-1]
    hole_id = feature['id']
    assert feature['parameters'] == inputs
    for update in ({'recessDiameterMm': 4}, {'recessDepthMm': 0}, {'recessDepthMm': 5}):
        call('solid.hole', {'featureId': hole_id, **update}, error='invalid_arguments')
        assert inspect()['authored'] == created['authored']
    assert body()['bodyId'] == target and body()['valid'] and body()['closed']
    assert math.isclose(body()['removedVolumeMm3'], 62*math.pi, abs_tol=1e-5)
    call('history.undo'); assert inspect()['authored'] == before['authored']
    call('history.redo'); assert inspect()['authored'] == created['authored']
    # Edit the Hole placement by choosing another retained standalone Point.
    call('solid.hole', {'featureId': hole_id, 'sourceEntityId': second['id'], 'sourcePointId': second['pointId'],
                        'boreDiameterMm': 6, 'distanceMm': 7, 'recessDiameterMm': 12, 'recessDepthMm': 3})
    assert body()['entryMm'] == [-15, 10, 10]
    assert math.isclose(body()['removedVolumeMm3'], 144*math.pi, abs_tol=1e-5)
    blind = inspect()
    call('document.exportStl', {'path': str(out/'blind-hole.stl'), 'bodyId': target, 'units': 'mm',
         'format': 'binary', 'surfaceDeviationMm': .02, 'normalDeviationDeg': 5})
    call('solid.hole', {'featureId': hole_id, 'extent': 'all'})
    assert math.isclose(body()['removedVolumeMm3'], 171*math.pi, abs_tol=1e-5)
    assert 'distanceMm' not in inspect()['authored']['features'][-1]['parameters']
    # All follows an upstream target thickness edit.
    call('solid.extrude', {'featureId': extrusion_id, 'sketchId': base['id'], 'profileId': profile, 'distance': 12, 'units': 'mm'})
    assert math.isclose(body()['removedVolumeMm3'], 189*math.pi, abs_tol=1e-5)
    # Return to the edge-associated first point, then prove the source edit follows.
    call('solid.hole', {'featureId': hole_id, 'sourceEntityId': point['id'], 'sourcePointId': point['pointId']})
    call('sketch.edit', {'sketchId': base['id']})
    call('sketch.dimensionUpdate', {'dimensionId': width_id, 'value': 85, 'units': 'mm'})
    call('sketch.finish')
    assert math.isclose(body()['entryMm'][0], -32.5, abs_tol=1e-6)
    # A downstream outer-edge chamfer must survive an earlier Hole diameter edit.
    edges = body()['solidEdges']
    (out/'edges.json').write_text(json.dumps(edges, indent=2))
    edge = next(e for e in edges if e.get('available') and e.get('provenance', {}).get('role') == 'endRim')
    call('solid.chamfer', {'targetBodyId': target, 'edgeIds': [edge['edgeId']], 'distanceMm': .5})
    downstream = inspect()['authored']['features'][-1]['id']
    call('solid.hole', {'featureId': hole_id, 'boreDiameterMm': 5})
    edited = inspect()
    assert edited['authored']['features'][-1]['id'] == downstream
    assert body()['valid'] and body()['closed']
    call('history.undo'); call('history.redo'); assert inspect()['authored'] == edited['authored']
    path = out/'counterbore-hole.alloy'
    call('document.save', {'path': str(path)})
    call('document.open', {'path': str(path)})
    assert inspect()['authored'] == edited['authored']
    assert math.isclose(body()['volumeMm3'], edited['evaluation']['bodies'][0]['volumeMm3'], abs_tol=1e-6)
    call('document.exportStl', {'path': str(out/'all-hole.stl'), 'bodyId': target, 'units': 'mm',
         'format': 'binary', 'surfaceDeviationMm': .02, 'normalDeviationDeg': 5})
    call('capture.screenshot', {'path': str(out/'final-hole.png')})
    meshes = {name: inspect_stl(out/name, True) for name in ('blind-hole.stl', 'all-hole.stl')}
    for name, expected, bounds, euler in (
        ('blind-hole.stl', blind['evaluation']['bodies'][0]['volumeMm3'], [[-37.5,-25,0],[37.5,25,10]], 2),
        ('all-hole.stl', body()['volumeMm3'], [[-42.5,-25,0],[42.5,25,12]], 0)):
        mesh = meshes[name]
        assert len(mesh['components']) == 1 and mesh['eulerCharacteristic'] == euler
        assert math.isclose(mesh['signedVolume'], expected, rel_tol=.001)
        for actual, wanted in zip(sum(mesh['bounds'], []), sum(bounds, [])):
            assert math.isclose(actual, wanted, abs_tol=.03)
    (out/'meshes.json').write_text(json.dumps(meshes, indent=2))
    (out/'report.json').write_text(json.dumps({'passed': True, 'source': 'hole', 'blind': blind,
        'final': inspect(), 'state': call('app.state')}, indent=2))
    (out/'transcript.json').write_text(json.dumps(transcript, indent=2))
