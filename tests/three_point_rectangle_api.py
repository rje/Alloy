#!/usr/bin/env python3
"""Real-app three-point Rectangle, retained source, extrusion and STL gate."""
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
        call('sketch.begin', {'plane': 'XY', 'name': 'Rotated bracket', 'units': 'mm'})
        call('sketch.addPoint', {'x': 0, 'y': 0, 'units': 'mm'})
        draft = call('app.state')['activeSession']['draftSketch']
        source_id = next(e['pointId'] for e in draft['parameters']['entities'] if e['type'] == 'point')
        sketch_id = draft['id']
        call('sketch.threePointRectangle', {
            'x1': 0, 'y1': 0, 'x2': 30, 'y2': 40, 'x3': -10, 'y3': 70,
            'firstDistance': 50, 'secondDistance': 50, 'units': 'mm',
            'startSnapTarget': {'kind': 'point', 'pointId': source_id}})
        placed = call('app.state')['activeSession']['draftSketch']
        parameters = placed['parameters']
        edges = [e for e in parameters['entities'] if e['type'] == 'line']
        assert len(edges) == 4 and len(parameters['dimensions']) == 2
        assert {c['type'] for c in parameters['constraints']} >= {'parallel', 'perpendicular', 'coincident'}
        near(sorted(d['valueMm'] for d in parameters['dimensions'])[0], 50)
        call('sketch.threePointRectangle', {
            'x1': 0, 'y1': 0, 'x2': 30, 'y2': 40, 'x3': 60, 'y3': 80,
            'units': 'mm'}, error='sketch_solve_failed')
        assert call('app.state')['activeSession']['draftSketch'] == placed
        call('capture.screenshot', {'path': str(out/'rotated-rectangle-sketch.png')})
        call('sketch.finish')
        inspected = call('document.inspect')
        profile = inspected['evaluation']['sketches'][0]['profiles'][0]
        near(profile['areaMm2'], 2500)
        call('solid.extrude', {'sketchId': sketch_id, 'profileId': profile['profileId'],
                               'distance': 5, 'units': 'mm'})
        before_edit = call('document.inspect')
        body_before = before_edit['evaluation']['bodies'][0]
        near(body_before['volumeMm3'], 12500)

        call('sketch.edit', {'sketchId': sketch_id})
        draft = call('app.state')['activeSession']['draftSketch']
        first_dimension = next(d for d in draft['parameters']['dimensions'] if d['pointIds'][0] == edges[0]['startPointId'])
        call('sketch.dimensionUpdate', {'dimensionId': first_dimension['id'], 'value': 60, 'units': 'mm'})
        call('sketch.movePoint', {'pointId': source_id, 'x': 5, 'y': 7, 'units': 'mm'})
        call('sketch.finish')
        after = call('document.inspect')
        body = after['evaluation']['bodies'][0]
        assert body['bodyId'] == body_before['bodyId']
        near(body['volumeMm3'], 15000)
        assert body['boundsMm'] != body_before['boundsMm']
        call('history.undo')
        assert call('document.inspect')['authored'] == before_edit['authored']
        call('history.redo')
        assert call('document.inspect')['authored'] == after['authored']

        model = out/'three-point-rectangle.alloy'
        stl = out/'three-point-rectangle.stl'
        call('document.save', {'path': str(model)})
        call('document.exportStl', {'path': str(stl), 'bodyId': body['bodyId'],
                                    'units': 'mm', 'format': 'binary',
                                    'surfaceDeviationMm': 0.05, 'normalDeviationDeg': 8})
        mesh = inspect_stl(stl, True)
        assert len(mesh['components']) == 1 and mesh['triangles'] >= 12
        for key, mesh_bound in (('min', mesh['bounds'][0]), ('max', mesh['bounds'][1])):
            for actual, expected in zip(mesh_bound, body['boundsMm'][key]):
                near(actual, expected, 0.001)
        call('view.home')
        call('capture.screenshot', {'path': str(out/'rotated-rectangle-solid.png')})
        call('document.open', {'path': str(model)})
        reopened = call('document.inspect')
        assert reopened['authored'] == after['authored']
        near(reopened['evaluation']['bodies'][0]['volumeMm3'], body['volumeMm3'])
        (out/'receipt.json').write_text(json.dumps({'passed': True, 'sourcePointId': source_id,
            'sketchId': sketch_id, 'bodyId': body['bodyId'], 'mesh': mesh,
            'transcript': transcript}, indent=2)+'\n')
    print('PASS three-point Rectangle, source/size edit, reopen and STL')


if __name__ == '__main__':
    main()
