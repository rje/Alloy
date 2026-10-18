#!/usr/bin/env python3
"""Real-app three-point Circle, retained sources, extrusion and STL gate."""
import argparse
import json
import math
from pathlib import Path

from app_harness import application
from stl_export_api import inspect_stl


def near(actual, expected, tolerance=1e-4):
    assert math.isclose(actual, expected, abs_tol=tolerance), (actual, expected)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    out = Path(args.output).resolve()
    out.mkdir(parents=True, exist_ok=True)

    with application(args.app, out) as (call, transcript):
        call('sketch.begin', {'plane': 'XY', 'name': 'Three-source circle', 'units': 'mm'})
        call('sketch.rectangle', {'x1': 0, 'y1': 0, 'x2': 20, 'y2': 20, 'units': 'mm'})
        draft = call('app.state')['activeSession']['draftSketch']
        sketch_id = draft['id']
        lines = [e for e in draft['parameters']['entities'] if e['type'] == 'line']
        assert len(lines) == 4
        sources = [lines[0]['startPointId'], lines[0]['endPointId'], lines[1]['endPointId']]
        call('sketch.constraint', {'type': 'fixed', 'pointIds': [sources[0]]})
        call('sketch.dimension', {'kind': 'horizontal', 'entityId': lines[0]['id'],
                                  'value': 20, 'units': 'mm'})
        call('sketch.dimension', {'kind': 'vertical', 'entityId': lines[1]['id'],
                                  'value': 20, 'units': 'mm'})
        dimensions = call('app.state')['activeSession']['draftSketch']['parameters']['dimensions']
        height_id = next(d['id'] for d in dimensions if d['kind'] == 'vertical')
        call('sketch.construction', {'entityIds': [e['id'] for e in lines],
                                     'construction': True})
        call('sketch.threePointCircle', {
            'x1': 0, 'y1': 0, 'x2': 20, 'y2': 0, 'x3': 20, 'y3': 20, 'units': 'mm',
            'firstSnapTarget': {'kind': 'point', 'pointId': sources[0]},
            'secondSnapTarget': {'kind': 'point', 'pointId': sources[1]},
            'thirdSnapTarget': {'kind': 'point', 'pointId': sources[2]}})
        placed = call('app.state')['activeSession']['draftSketch']
        circles = [e for e in placed['parameters']['entities'] if e['type'] == 'circle']
        assert len(circles) == 1
        circle_id = circles[0]['id']
        constraints = [c for c in placed['parameters']['constraints']
                       if c['type'] == 'pointOnCurve' and circle_id in c.get('entityIds', [])]
        assert len(constraints) == 3
        near(circles[0]['radiusMm'], math.sqrt(200))
        call('sketch.threePointCircle', {
            'x1': 0, 'y1': 0, 'x2': 10, 'y2': 0, 'x3': 20, 'y3': 0,
            'units': 'mm'}, error='sketch_solve_failed')
        assert call('app.state')['activeSession']['draftSketch'] == placed
        call('capture.screenshot', {'path': str(out/'three-point-circle-sketch.png')})
        call('sketch.finish')
        inspected = call('document.inspect')
        profile = inspected['evaluation']['sketches'][0]['profiles'][0]
        near(profile['areaMm2'], 200 * math.pi)
        call('solid.extrude', {'sketchId': sketch_id, 'profileId': profile['profileId'],
                               'distance': 5, 'units': 'mm'})
        before = call('document.inspect')
        body_before = before['evaluation']['bodies'][0]
        near(body_before['volumeMm3'], 1000 * math.pi)

        call('sketch.edit', {'sketchId': sketch_id})
        call('sketch.dimensionUpdate', {'dimensionId': height_id, 'value': 30, 'units': 'mm'})
        moved = call('app.state')['activeSession']['draftSketch']
        moved_circle = next(e for e in moved['parameters']['entities'] if e['id'] == circle_id)
        near(moved_circle['radiusMm'], math.sqrt(325))
        assert len([c for c in moved['parameters']['constraints']
                    if c['type'] == 'pointOnCurve' and circle_id in c.get('entityIds', [])]) == 3
        call('capture.screenshot', {'path': str(out/'source-edited-circle-sketch.png')})
        call('sketch.finish')
        after = call('document.inspect')
        body = after['evaluation']['bodies'][0]
        assert body['bodyId'] == body_before['bodyId']
        near(body['volumeMm3'], 1625 * math.pi)
        call('history.undo')
        assert call('document.inspect')['authored'] == before['authored']
        call('history.redo')
        assert call('document.inspect')['authored'] == after['authored']

        model = out/'three-point-circle.alloy'
        stl = out/'three-point-circle.stl'
        call('document.save', {'path': str(model)})
        call('document.exportStl', {'path': str(stl), 'bodyId': body['bodyId'],
                                    'units': 'mm', 'format': 'binary',
                                    'surfaceDeviationMm': 0.05, 'normalDeviationDeg': 8})
        mesh = inspect_stl(stl, True)
        assert len(mesh['components']) == 1 and mesh['triangles'] >= 32
        assert abs(mesh['signedVolume'] - body['volumeMm3']) / body['volumeMm3'] < 0.01
        for key, mesh_bound in (('min', mesh['bounds'][0]), ('max', mesh['bounds'][1])):
            for actual, expected in zip(mesh_bound, body['boundsMm'][key]):
                near(actual, expected, 0.1)
        call('view.home')
        call('capture.screenshot', {'path': str(out/'three-point-circle-solid.png')})
        call('document.open', {'path': str(model)})
        reopened = call('document.inspect')
        assert reopened['authored'] == after['authored']
        near(reopened['evaluation']['bodies'][0]['volumeMm3'], body['volumeMm3'])
        (out/'receipt.json').write_text(json.dumps({'passed': True, 'circleId': circle_id,
            'sourcePointIds': sources, 'sketchId': sketch_id, 'bodyId': body['bodyId'],
            'mesh': mesh, 'transcript': transcript}, indent=2)+'\n')
    print('PASS three-point Circle, retained source edit, reopen and STL')


if __name__ == '__main__':
    main()
