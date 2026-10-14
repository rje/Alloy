#!/usr/bin/env python3
"""Author and edit a retained finite-contact circle-line Tangent in the real app."""
import argparse
import json
import math
from pathlib import Path

from app_harness import application


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    output = Path(args.output).resolve()
    with application(args.app, output) as (call, transcript):
        started = call('sketch.begin', {'plane': 'XY', 'name': 'Circle tangent to datum'})

        def draft():
            return call('app.state')['activeSession']['draftSketch']

        def line(x1, y1, x2, y2):
            call('sketch.line', {'x1': x1, 'y1': y1, 'x2': x2, 'y2': y2, 'units': 'mm'})
            return draft()['parameters']['entities'][-1]

        base = line(-20, 0, 20, 0)
        target = line(-20, 10, 20, 10)
        call('sketch.construction', {'entityIds': [base['id'], target['id']], 'construction': True})
        call('sketch.addCircle', {'centerX': 0, 'centerY': 18, 'diameter': 10, 'units': 'mm'})
        circle = draft()['parameters']['entities'][-1]
        call('sketch.constraint', {'type': 'fixed', 'pointIds': [base['startPointId']]})
        call('sketch.constraint', {'type': 'horizontal', 'entityIds': [target['id']]})
        offset = call('sketch.dimension', {'kind': 'vertical',
            'pointIds': [base['startPointId'], target['startPointId']], 'value': 10, 'units': 'mm'})
        offset_id = offset['draftSketch']['parameters']['dimensions'][-1]['id']
        result = call('sketch.constraint', {'type': 'tangent', 'entityIds': [target['id'], circle['id']]})
        relation = next(c for c in result['draftSketch']['parameters']['constraints'] if c['type'] == 'tangent')
        assert relation['entityIds'] == [circle['id'], target['id']] and relation['side'] == 1
        before_duplicate = draft()
        call('sketch.constraint', {'type': 'tangent', 'entityIds': [circle['id'], target['id']]}, 'sketch_solve_failed')
        assert draft() == before_duplicate
        radius = call('sketch.dimension', {'kind': 'radius', 'entityId': circle['id'], 'value': 5, 'units': 'mm'})
        radius_id = radius['draftSketch']['parameters']['dimensions'][-1]['id']

        def check_contact(sketch, expected_radius, expected_line_y):
            params = sketch['parameters']
            points = {p['id']: p for p in params['points']}
            a, b = points[target['startPointId']], points[target['endPointId']]
            center = points[circle['centerPointId']]
            dx, dy = b['uMm'] - a['uMm'], b['vMm'] - a['vMm']
            length = math.hypot(dx, dy)
            signed_distance = (dx * (center['vMm'] - a['vMm']) - dy * (center['uMm'] - a['uMm'])) / length
            foot = ((center['uMm'] - a['uMm']) * dx + (center['vMm'] - a['vMm']) * dy) / (length * length)
            assert math.isclose(signed_distance, expected_radius, abs_tol=1e-6)
            assert -1e-8 <= foot <= 1 + 1e-8
            assert math.isclose(a['vMm'], expected_line_y, abs_tol=1e-6)
            assert next(e['radiusMm'] for e in params['entities'] if e['id'] == circle['id']) == expected_radius

        check_contact(draft(), 5, 10)
        preview = call('command.preview', {'session': started['id']})
        assert len(preview['sketchGeometry']['profiles']) == 1
        call('capture.screenshot', {'path': str(output / 'circle-tangent-sketch.png')})
        sketch_id = draft()['id']
        call('sketch.finish')
        call('sketch.edit', {'sketchId': sketch_id})
        call('sketch.dimensionUpdate', {'dimensionId': radius_id, 'value': 7, 'units': 'mm'})
        call('sketch.dimensionUpdate', {'dimensionId': offset_id, 'value': 12, 'units': 'mm'})
        edited = draft()
        check_contact(edited, 7, 12)
        assert next(c['id'] for c in edited['parameters']['constraints'] if c['type'] == 'tangent') == relation['id']
        call('capture.screenshot', {'path': str(output / 'circle-tangent-edited.png')})
        call('sketch.finish')
        call('history.undo')
        call('sketch.edit', {'sketchId': sketch_id})
        check_contact(draft(), 5, 10)
        call('sketch.cancel')
        call('history.redo')
        call('sketch.edit', {'sketchId': sketch_id})
        check_contact(draft(), 7, 12)
        call('sketch.finish')
        saved = output / 'circle-tangent.alloy'
        call('document.save', {'path': str(saved)})
        authored = call('document.inspect')['authored']
        call('document.new', {'discard': False})
        call('document.open', {'path': str(saved)})
        assert call('document.inspect')['authored'] == authored
        (output / 'result.json').write_text(json.dumps({'status': 'passed', 'calls': len(transcript),
            'relationId': relation['id'], 'radiusDimensionId': radius_id, 'offsetDimensionId': offset_id,
            'authored': authored, 'transcript': transcript}, indent=2) + '\n')
    print('PASS: explicit circle-line Tangent, radius/offset edit, history and reopen')


if __name__ == '__main__':
    main()
