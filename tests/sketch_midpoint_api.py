#!/usr/bin/env python3
"""Persistent circle-center midpoint inference; tangent and region gates are separate."""
import argparse
import hashlib
import json
import math
from pathlib import Path
from app_harness import application


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--fixture', default=str(Path(__file__).parent / 'workflows/fixtures/sketch-midpoint.json'))
    args = parser.parse_args()
    output = Path(args.output).resolve()
    fixture = json.loads(Path(args.fixture).read_text())
    captures = []
    with application(args.app, output) as (call, transcript):
        initial = call('app.state')
        session = call('sketch.begin', {'plane': 'XY', 'name': 'Persistent midpoint circle'})

        def draft():
            return call('command.preview', {'session': session['id']})['authoredDraft']

        def project(u, v):
            point = call('view.projectSketchPoint', {'plane': 'XY', 'uMm': u, 'vMm': v})
            return {'x': point['x'], 'y': point['y']}

        call('sketch.rectangle', {'x1': 0, 'y1': 0, 'x2': fixture['widthMm'], 'y2': fixture['heightMm'], 'units': 'mm'})
        edges = draft()['parameters']['entities']
        line_id = edges[0]['id']
        call('sketch.constraint', {'type': 'fixed', 'pointIds': [edges[0]['startPointId']]})
        call('sketch.dimension', {'kind': 'horizontal', 'entityId': line_id, 'value': fixture['widthMm'], 'units': 'mm'})
        width_id = draft()['parameters']['dimensions'][-1]['id']
        call('sketch.dimension', {'kind': 'vertical', 'entityId': edges[1]['id'], 'value': fixture['heightMm'], 'units': 'mm'})
        call('sketch.construction', {'entityIds': [line_id], 'construction': True})
        before = draft()
        call('sketch.tool', {'tool': 'circle'})
        history_before = call('app.state')['activeSession']['draftHistory']
        midpoint = project(fixture['widthMm'] / 2, 0)
        snap = call('sketch.pointerMove', midpoint)['snap']
        assert snap['status'] == 'acquired' and snap['target'] == {'kind': 'midpoint', 'entityId': line_id}
        assert not snap['authoredChange']
        captures.append(call('capture.screenshot', {'path': str(output / 'midpoint-cue.png')}))
        call('sketch.placePoint', midpoint)
        assert draft() == before
        assert call('app.state')['activeSession']['draftHistory'] == history_before
        call('sketch.placePoint', project(fixture['widthMm'] / 2 + fixture['radiusMm'], 0))
        call('sketch.tool', {'tool': 'select'})
        created = draft()
        circle = created['parameters']['entities'][-1]
        relation = next(c for c in created['parameters']['constraints'] if c['type'] == 'midpoint')
        assert relation['entityIds'] == [line_id] and relation['pointIds'] == [circle['centerPointId']]
        assert circle['centerPointId'] not in {p['id'] for p in before['parameters']['points']}

        def check_midpoint():
            current = draft()['parameters']
            points = {p['id']: p for p in current['points']}
            a, b = points[edges[0]['startPointId']], points[edges[0]['endPointId']]
            center = points[circle['centerPointId']]
            for axis in ('uMm', 'vMm'):
                assert math.isclose(center[axis], (a[axis] + b[axis]) / 2, abs_tol=1e-6)
            assert relation in current['constraints']

        check_midpoint()
        call('history.undo')
        assert draft() == before
        call('history.redo')
        assert draft() == created
        call('sketch.dimensionUpdate', {'dimensionId': width_id, 'value': fixture['editedWidthMm'], 'units': 'mm'})
        check_midpoint()
        call('sketch.construction', {'entityIds': [line_id], 'construction': False})
        check_midpoint()
        captures.append(call('capture.screenshot', {'path': str(output / 'midpoint-after-line-edit.png')}))
        call('sketch.finish')
        committed = call('document.inspect')
        path = output / 'midpoint.alloy'
        call('document.save', {'path': str(path)})
        call('document.new', {'discard': False})
        call('document.open', {'path': str(path)})
        assert call('document.inspect')['authored'] == committed['authored']
        final = call('app.state')
    receipt = {
        'result': 'passed', 'fixtureSha256': hashlib.sha256(Path(args.fixture).read_bytes()).hexdigest(),
        'verifiedCapabilities': ['sketch.persistent_circle_midpoint'], 'covers': [],
        'initialState': initial, 'finalState': final, 'transcript': transcript, 'captures': captures,
        'evidenceDimensions': {'api': {'status': 'passed'}, 'geometry': {'status': 'passed'},
                               'save_reopen': {'status': 'passed'}, 'visual': {'status': 'captured'},
                               'physical_input': {'status': 'not_run', 'reason': 'Pointer API does not qualify native input.'}},
        'limits': ['Tangent placement and combined line/circle profile regions remain separate gates.']}
    (output / 'sketch-midpoint-api.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print('PASS: midpoint cue, atomic circle relation, line-driven center, history and storage')


if __name__ == '__main__':
    main()
