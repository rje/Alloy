#!/usr/bin/env python3
"""Real-app angular sketch -> solid -> upstream edit workflow; bounded tutorial slice."""
import argparse
import hashlib
import json
import math
from pathlib import Path
from app_harness import application


def near(actual, expected):
    assert math.isclose(actual, expected, rel_tol=1e-7, abs_tol=1e-5), (actual, expected)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--fixture', default=str(Path(__file__).parent / 'workflows/fixtures/angular-sketch.json'))
    args = parser.parse_args()
    fixture = json.loads(Path(args.fixture).read_text())
    assert fixture['case'] == 'angular-sketch'
    output = Path(args.output).resolve()
    captures = []
    with application(args.app, output) as (call, transcript):
        initial = call('app.state')
        assert initial['viewport']['gpu']['hardwareAccelerated']
        session = call('sketch.begin', {'plane': 'XY', 'name': 'Angular triangular plate'})
        # Closed connected profile; angle uses the opposite ray of the closing edge.
        for coords in ((0, 0, 30, 0), (30, 0, 15, 20), (15, 20, 0, 0)):
            call('sketch.line', dict(zip(('x1', 'y1', 'x2', 'y2'), coords)))
        def preview():
            return call('command.preview', {'session': session['id']})
        def params():
            return preview()['authoredDraft']['parameters']
        edges = params()['entities']
        for point in (edges[0]['startPointId'], edges[0]['endPointId']):
            call('sketch.constraint', {'type': 'fixed', 'pointIds': [point]})
        call('sketch.dimension', {'kind': 'linear', 'entityId': edges[2]['id'], 'value': 30, 'annotation': {'uMm': -6, 'vMm': 17}})
        call('sketch.dimension', {'kind': 'angle', 'entityIds': [edges[0]['id'], edges[2]['id']],
             'raySigns': [1, -1], 'valueDeg': 45, 'annotation': {'uMm': 15, 'vMm': 6}})
        angular = params()['dimensions'][-1]
        angle_id = angular['id']
        call('sketch.selectionSet', {'entityIds': [edges[0]['id'], edges[2]['id']]})
        # Pointer lies in actual unequal wedges, not nearest-bisector 90-degree bins.
        for u, v, expected in ((15, 5, 45), (5, 15, 135)):
            pixel = call('view.projectSketchPoint', {'plane': 'XY', 'uMm': u, 'vMm': v})
            pending = call('sketch.dimensionPreview', {'x': pixel['x'], 'y': pixel['y']})
            near(pending['measuredValueDeg'], expected)
        call('sketch.tool', {'tool': 'select'})
        assert angular['raySigns'] == [1, -1] and angular['valueDeg'] == 45
        before = preview()['authoredDraft']
        call('sketch.annotationMove', {'dimensionId': angle_id, 'uMm': 18, 'vMm': 8})
        after = preview()['authoredDraft']
        assert before['parameters']['points'] == after['parameters']['points']
        assert params()['dimensions'][-1]['raySigns'] == [1, -1]
        info = preview()
        near(info['sketchGeometry']['profiles'][0]['areaMm2'], 450 * math.sin(math.radians(45)))
        assert info['sketchGeometry']['solver']['dof'] == 0
        captures.append(call('capture.screenshot', {'path': str(output / 'angle-45.png')}))
        sketch_id = info['authoredDraft']['id']
        profile_id = info['sketchGeometry']['profiles'][0]['profileId']
        call('sketch.finish')
        extrude = call('command.begin', {'command': 'solid.extrude', 'inputs': {
            'sketchId': sketch_id, 'profileId': profile_id, 'distance': 5, 'units': 'mm'}})
        call('command.preview', {'session': extrude['id']})
        call('command.commit', {'session': extrude['id']})
        original = call('document.inspect')
        near(original['evaluation']['bodies'][0]['volumeMm3'], 2250 * math.sin(math.radians(45)))
        call('document.units', {'units': 'in'})
        session = call('sketch.edit', {'sketchId': sketch_id})
        call('sketch.dimensionUpdate', {'dimensionId': angle_id, 'valueDeg': 60})
        assert params()['dimensions'][-1]['valueDeg'] == 60
        call('sketch.finish')
        edited = call('document.inspect')
        near(edited['evaluation']['bodies'][0]['volumeMm3'], 2250 * math.sin(math.radians(60)))
        assert edited['evaluation']['sketches'][0]['profiles'][0]['profileId'] == profile_id
        call('history.undo')
        near(call('document.inspect')['evaluation']['bodies'][0]['volumeMm3'], 2250 * math.sin(math.radians(45)))
        call('history.redo')
        assert call('document.inspect')['authored'] == edited['authored']
        call('view.home')
        captures.append(call('capture.screenshot', {'path': str(output / 'angular-plate-edited.png')}))
        path = str(output / 'angular-plate.alloy')
        call('document.save', {'path': path})
        call('document.new', {'discard': False})
        call('document.open', {'path': path})
        reopened = call('document.inspect')
        assert reopened['authored'] == edited['authored']
        near(reopened['evaluation']['bodies'][0]['volumeMm3'], 2250 * math.sin(math.radians(60)))
    (output / 'angular-workflow-api.json').write_text(json.dumps({
        'result': 'passed', 'scope': 'Bounded angular sketch/extrusion/upstream edit; not full illustrated tutorial',
        'transcript': transcript, 'captures': captures, 'initialState': initial,
        'fixtureSha256': hashlib.sha256(Path(args.fixture).read_bytes()).hexdigest(),
        'verifiedCapabilities': ['sketch.two_line_angles', 'sketch.angle_sector_selection', 'sketch.angle_upstream_solid', 'sketch.angle_history_storage'],
        'evidenceDimensions': {key: {'status': 'passed'} for key in ('api', 'hardware', 'geometry', 'save_reopen')},
        'nativeInput': 'separate', 'visual': 'captured; independent review required'}, indent=2) + '\n')
    print('PASS: angular sketch, extrusion, upstream edit, history and reopen')


if __name__ == '__main__':
    main()
