#!/usr/bin/env python3
"""Exercise retained circle-circle tangent branches through the real app."""
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
        call('sketch.begin', {'plane': 'XY', 'name': 'External circle contact'})

        def draft():
            return call('app.state')['activeSession']['draftSketch']

        def circle(x, y, diameter):
            call('sketch.addCircle', {'centerX': x, 'centerY': y, 'diameter': diameter, 'units': 'mm'})
            return draft()['parameters']['entities'][-1]

        a = circle(0, 0, 20)
        b = circle(20, 0, 10)
        call('sketch.constraint', {'type': 'fixed', 'pointIds': [a['centerPointId']]})
        offset = call('sketch.dimension', {'kind': 'vertical', 'pointIds': [a['centerPointId'], b['centerPointId']], 'value': 3, 'units': 'mm'})
        offset_id = offset['draftSketch']['parameters']['dimensions'][-1]['id']
        radius_ids = []
        for entity, radius in [(a, 10), (b, 5)]:
            result = call('sketch.dimension', {'kind': 'radius', 'entityId': entity['id'], 'value': radius, 'units': 'mm'})
            radius_ids.append(result['draftSketch']['parameters']['dimensions'][-1]['id'])
        result = call('sketch.constraint', {'type': 'tangent', 'entityIds': [b['id'], a['id']]})
        relation = next(c for c in result['draftSketch']['parameters']['constraints'] if c['type'] == 'circleTangent')
        assert relation['side'] == 1
        before = draft()
        call('sketch.constraint', {'type': 'tangent', 'entityIds': [a['id'], b['id']]}, 'sketch_solve_failed')
        assert draft() == before

        def contact(sketch, expected_radius, expected_offset):
            params = sketch['parameters']
            entities = {e['id']: e for e in params['entities']}
            points = {p['id']: p for p in params['points']}
            c1, c2 = points[a['centerPointId']], points[b['centerPointId']]
            assert math.isclose(math.hypot(c2['uMm']-c1['uMm'], c2['vMm']-c1['vMm']), 10+expected_radius, abs_tol=1e-6)
            assert math.isclose(c2['vMm']-c1['vMm'], expected_offset, abs_tol=1e-6)
            assert math.isclose(entities[b['id']]['radiusMm'], expected_radius, abs_tol=1e-6)
            assert next(c for c in params['constraints'] if c['type'] == 'circleTangent') == relation
        contact(draft(), 5, 3)
        profiles = call('command.preview', {'session': call('app.state')['activeSession']['id']})['sketchGeometry']['profiles']
        assert len(profiles) == 2, profiles
        call('sketch.lookAt')
        call('capture.screenshot', {'path': str(output / 'external-initial.png')})
        sketch_id = draft()['id']
        call('sketch.finish')
        call('sketch.edit', {'sketchId': sketch_id})
        call('sketch.dimensionUpdate', {'dimensionId': radius_ids[1], 'value': 7, 'units': 'mm'})
        call('sketch.dimensionUpdate', {'dimensionId': offset_id, 'value': 6, 'units': 'mm'})
        contact(draft(), 7, 6)
        assert len(call('command.preview', {'session': call('app.state')['activeSession']['id']})['sketchGeometry']['profiles']) == 2
        call('capture.screenshot', {'path': str(output / 'external-edited.png')})
        gpu = call('app.state')['viewport']['gpu']
        call('sketch.finish')
        call('history.undo')
        call('sketch.edit', {'sketchId': sketch_id})
        contact(draft(), 5, 3)
        call('sketch.cancel')
        call('history.redo')
        call('sketch.edit', {'sketchId': sketch_id})
        contact(draft(), 7, 6)
        call('sketch.finish')
        saved = output / 'circle-circle-tangent.alloy'
        call('document.save', {'path': str(saved)})
        authored = call('document.inspect')['authored']
        call('document.new', {'discard': False})
        call('document.open', {'path': str(saved)})
        assert call('document.inspect')['authored'] == authored
        call('sketch.edit', {'sketchId': sketch_id})
        contact(draft(), 7, 6)
        call('sketch.cancel')
        call('document.new', {'discard': False})
        call('sketch.begin', {'plane': 'XY', 'name': 'Internal branch retention'})
        outer = circle(0, 0, 30)
        inner = circle(8, 0, 10)
        call('sketch.constraint', {'type': 'fixed', 'pointIds': [outer['centerPointId']]})
        for entity, radius in [(outer, 15), (inner, 5)]:
            result = call('sketch.dimension', {'kind': 'radius', 'entityId': entity['id'], 'value': radius, 'units': 'mm'})
        inner_radius_id = result['draftSketch']['parameters']['dimensions'][-1]['id']
        result = call('sketch.constraint', {'type': 'tangent', 'entityIds': [inner['id'], outer['id']]})
        internal_relation = next(c for c in result['draftSketch']['parameters']['constraints'] if c['type'] == 'circleTangent')
        assert internal_relation['side'] == -1 and internal_relation['entityIds'] == [outer['id'], inner['id']]
        call('sketch.dimensionUpdate', {'dimensionId': inner_radius_id, 'value': 7, 'units': 'mm'})
        params = draft()['parameters']
        points = {p['id']: p for p in params['points']}
        c1, c2 = points[outer['centerPointId']], points[inner['centerPointId']]
        assert math.isclose(math.hypot(c2['uMm']-c1['uMm'], c2['vMm']-c1['vMm']), 8, abs_tol=1e-6)
        assert next(c for c in params['constraints'] if c['type'] == 'circleTangent') == internal_relation
        before = draft()
        call('sketch.dimensionUpdate', {'dimensionId': inner_radius_id, 'value': 20, 'units': 'mm'}, 'sketch_solve_failed')
        assert draft() == before
        call('sketch.lookAt')
        call('capture.screenshot', {'path': str(output / 'internal-edited.png')})
        call('sketch.finish')
        internal_saved = output / 'internal-circle-tangent.alloy'
        call('document.save', {'path': str(internal_saved)})
        internal_authored = call('document.inspect')['authored']
        call('document.new', {'discard': False})
        call('document.open', {'path': str(internal_saved)})
        assert call('document.inspect')['authored'] == internal_authored
        (output / 'result.json').write_text(json.dumps({'status': 'passed', 'calls': len(transcript), 'gpu': gpu, 'authored': authored, 'transcript': transcript}, indent=2)+'\n')
    print('PASS: circle-circle tangent radius/center edit, two regions, history and reopen')


if __name__ == '__main__':
    main()
