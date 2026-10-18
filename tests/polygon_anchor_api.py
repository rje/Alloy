#!/usr/bin/env python3
"""Real-app retained Polygon point anchor, downstream solid, history and STL."""
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
        call('sketch.begin', {'plane': 'XY', 'name': 'Anchored hexagon'})
        call('sketch.addPoint', {'x': 10, 'y': 0, 'units': 'mm'})
        draft = call('app.state')['activeSession']['draftSketch']
        point_id = next(e['pointId'] for e in draft['parameters']['entities']
                        if e['type'] == 'point')
        call('sketch.tool', {'tool': 'inscribedPolygon'})
        pixel = call('view.projectSketchPoint', {'sketchId': draft['id'],
                                                 'uMm': 10, 'vMm': 0})
        preview = call('sketch.pointerMove', {'x': pixel['x'], 'y': pixel['y']})
        assert preview['snap']['status'] == 'acquired'
        assert preview['snap']['target'] == {'kind': 'point', 'pointId': point_id}
        assert call('app.state')['viewport']['gpu']['sketchSnapCue']['visible']
        call('capture.screenshot', {'path': str(out/'anchor-snap-cue.png')})
        call('sketch.tool', {'tool': 'select'})
        call('sketch.polygon', {'mode': 'circumscribed', 'x': 10, 'y': 0,
                               'size': 10, 'count': 6, 'angleDeg': 0, 'side': 1,
                               'units': 'mm',
                               'anchorSnapTarget': {'kind': 'point', 'pointId': point_id}})
        draft = call('app.state')['activeSession']['draftSketch']
        sketch_id = draft['id']
        group = draft['parameters']['polygonGroups'][0]
        assert group['anchorSource'] == {'kind': 'point', 'pointId': point_id}
        assert group['anchorPointId'] and group['anchorConstraintId']
        call('sketch.finish')
        source = call('document.inspect')
        profile = source['evaluation']['sketches'][0]['profiles'][0]
        close(profile['areaMm2'], 200 * math.sqrt(3))
        call('solid.extrude', {'sketchId': sketch_id,
                               'profileId': profile['profileId'],
                               'distance': 5, 'units': 'mm'})
        before = call('document.inspect')
        original_body = before['evaluation']['bodies'][0]
        close(original_body['volumeMm3'], 1000 * math.sqrt(3))

        call('sketch.edit', {'sketchId': sketch_id})
        call('sketch.movePoint', {'pointId': point_id,
                                  'x': 25, 'y': 0, 'units': 'mm'})
        moved_group = call('app.state')['activeSession']['draftSketch']['parameters']['polygonGroups'][0]
        close(moved_group['uMm'], 25)
        assert moved_group['id'] == group['id']
        assert moved_group['entityIds'] == group['entityIds']
        call('sketch.finish')
        moved = call('document.inspect')
        moved_body = moved['evaluation']['bodies'][0]
        assert moved_body['bodyId'] == original_body['bodyId']
        close(moved_body['volumeMm3'], original_body['volumeMm3'])
        for bound in ('min', 'max'):
            close(moved_body['boundsMm'][bound][0] - original_body['boundsMm'][bound][0], 15)

        call('sketch.edit', {'sketchId': sketch_id})
        call('sketch.polygonUpdate', {'polygonId': group['id'], 'size': 12, 'units': 'mm'})
        resized_group = call('app.state')['activeSession']['draftSketch']['parameters']['polygonGroups'][0]
        assert resized_group['anchorSource'] == group['anchorSource']
        assert resized_group['anchorPointId'] == group['anchorPointId']
        close(resized_group['uMm'], 25)
        call('sketch.finish')
        resized = call('document.inspect')
        body = resized['evaluation']['bodies'][0]
        assert body['bodyId'] == original_body['bodyId']
        close(body['volumeMm3'], 1440 * math.sqrt(3))
        call('history.undo')
        assert call('document.inspect')['authored'] == moved['authored']
        call('history.redo')
        assert call('document.inspect')['authored'] == resized['authored']

        path = out/'anchored-polygon.alloy'
        call('document.save', {'path': str(path)})
        stl = out/'anchored-polygon.stl'
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
        call('capture.screenshot', {'path': str(out/'anchored-solid.png')})
        call('document.open', {'path': str(path)})
        reopened = call('document.inspect')
        assert reopened['authored'] == resized['authored']
        close(reopened['evaluation']['bodies'][0]['volumeMm3'], body['volumeMm3'])
        (out/'receipt.json').write_text(json.dumps({'passed': True,
            'sourcePointId': point_id, 'polygonId': group['id'],
            'sketchId': sketch_id, 'bodyId': body['bodyId'],
            'mesh': mesh, 'transcript': transcript}, indent=2)+'\n')
    print('PASS retained Polygon anchor, source edit, extrusion, history, reopen, STL')


if __name__ == '__main__':
    main()
