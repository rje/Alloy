#!/usr/bin/env python3
"""Real-app two-reference Polygon propagation, conflicting edit, and STL."""
import argparse
import json
import math
from pathlib import Path

from app_harness import application
from stl_export_api import inspect_stl


def close(actual, expected, tolerance=1e-4):
    assert math.isclose(actual, expected, abs_tol=tolerance), (actual, expected)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    out = Path(args.output).resolve()
    out.mkdir(parents=True, exist_ok=True)

    with application(args.app, out) as (call, transcript):
        call('sketch.begin', {'plane': 'XY', 'name': 'Two-reference hexagon'})
        call('sketch.addPoint', {'x': 10, 'y': 0, 'units': 'mm'})
        draft = call('app.state')['activeSession']['draftSketch']
        point_id = next(e['pointId'] for e in draft['parameters']['entities']
                        if e['type'] == 'point')
        call('sketch.tool', {'tool': 'inscribedPolygon'})
        origin = call('view.projectSketchPoint', {'sketchId': draft['id'],
                                                  'uMm': 0, 'vMm': 0})
        call('sketch.placePoint', {'x': origin['x'], 'y': origin['y']})
        radial = call('view.projectSketchPoint', {'sketchId': draft['id'],
                                                  'uMm': 10, 'vMm': 0})
        preview = call('sketch.pointerMove', {'x': radial['x'], 'y': radial['y']})
        assert preview['previewed']
        state = call('app.state')
        assert state['activeSession']['snap']['status'] == 'acquired'
        assert state['activeSession']['snap']['target'] == {'kind': 'point', 'pointId': point_id}
        assert state['viewport']['gpu']['sketchSnapCue']['visible']
        call('capture.screenshot', {'path': str(out/'second-driver-snap-cue.png')})
        call('sketch.tool', {'tool': 'select'})
        call('sketch.polygon', {'mode': 'inscribed', 'x': 0, 'y': 0,
                               'size': 10, 'count': 6, 'side': 1, 'units': 'mm',
                               'anchorSnapTarget': {'kind': 'origin'},
                               'driverSnapTarget': {'kind': 'point', 'pointId': point_id}})
        draft = call('app.state')['activeSession']['draftSketch']
        sketch_id = draft['id']
        group = draft['parameters']['polygonGroups'][0]
        assert group['anchorSource']['kind'] == 'origin'
        assert group['driverSource'] == {'kind': 'point', 'pointId': point_id}
        assert group['driverPointId'] and group['driverConstraintId']
        close(group['sizeMm'], 10)
        close(group['angleDeg'], 0)
        call('sketch.finish')
        source = call('document.inspect')
        profile = source['evaluation']['sketches'][0]['profiles'][0]
        close(profile['areaMm2'], 150 * math.sqrt(3))
        call('solid.extrude', {'sketchId': sketch_id,
                               'profileId': profile['profileId'],
                               'distance': 5, 'units': 'mm'})
        before = call('document.inspect')
        body_before = before['evaluation']['bodies'][0]
        close(body_before['volumeMm3'], 750 * math.sqrt(3))

        call('sketch.edit', {'sketchId': sketch_id})
        draft_before = call('app.state')['activeSession']['draftSketch']
        call('sketch.polygonUpdate', {'polygonId': group['id'],
                                      'size': 11, 'units': 'mm'},
             error='sketch_solve_failed')
        assert call('app.state')['activeSession']['draftSketch'] == draft_before
        call('sketch.movePoint', {'pointId': point_id,
                                  'x': 0, 'y': 12, 'units': 'mm'})
        moved_group = call('app.state')['activeSession']['draftSketch']['parameters']['polygonGroups'][0]
        assert moved_group['id'] == group['id']
        assert moved_group['entityIds'] == group['entityIds']
        assert moved_group['anchorSource'] == group['anchorSource']
        assert moved_group['driverSource'] == group['driverSource']
        close(moved_group['sizeMm'], 12)
        close(moved_group['angleDeg'], 90)
        call('capture.screenshot', {'path': str(out/'rotated-sketch.png')})
        call('sketch.finish')
        rotated = call('document.inspect')
        body = rotated['evaluation']['bodies'][0]
        assert body['bodyId'] == body_before['bodyId']
        close(body['volumeMm3'], 1080 * math.sqrt(3))
        assert body['boundsMm'] != body_before['boundsMm']
        call('history.undo')
        assert call('document.inspect')['authored'] == before['authored']
        call('history.redo')
        assert call('document.inspect')['authored'] == rotated['authored']

        path = out/'two-reference-polygon.alloy'
        call('document.save', {'path': str(path)})
        stl = out/'two-reference-polygon.stl'
        call('document.exportStl', {'path': str(stl), 'bodyId': body['bodyId'],
                                    'units': 'mm', 'format': 'binary',
                                    'surfaceDeviationMm': 0.05,
                                    'normalDeviationDeg': 8})
        mesh = inspect_stl(stl, True)
        assert len(mesh['components']) == 1 and mesh['triangles'] >= 20
        for bound, mesh_bound in (('min', mesh['bounds'][0]),
                                  ('max', mesh['bounds'][1])):
            for actual, expected in zip(mesh_bound, body['boundsMm'][bound]):
                close(actual, expected, 0.001)
        call('view.home')
        call('capture.screenshot', {'path': str(out/'rotated-solid.png')})
        call('document.open', {'path': str(path)})
        reopened = call('document.inspect')
        assert reopened['authored'] == rotated['authored']
        close(reopened['evaluation']['bodies'][0]['volumeMm3'], body['volumeMm3'])
        (out/'receipt.json').write_text(json.dumps({'passed': True,
            'sourcePointId': point_id, 'polygonId': group['id'],
            'sketchId': sketch_id, 'bodyId': body['bodyId'],
            'mesh': mesh, 'transcript': transcript}, indent=2)+'\n')
    print('PASS two-driver Polygon, source rotation/scale, refusal, reopen, STL')


if __name__ == '__main__':
    main()
