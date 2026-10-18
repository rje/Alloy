#!/usr/bin/env python3
"""Retained symmetry of separately authored circles through the real Alloy entrypoint."""
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
        call('sketch.begin', {'plane': 'XY', 'name': 'Symmetric circles'})
        def draft():
            return call('app.state')['activeSession']['draftSketch']
        def circle(x, y, diameter):
            call('sketch.addCircle', {'centerX': x, 'centerY': y, 'diameter': diameter, 'units': 'mm'})
            return draft()['parameters']['entities'][-1]
        left, right = circle(-20, 0, 20), circle(21, 1, 18)
        call('sketch.line', {'x1': 0, 'y1': -25, 'x2': 0, 'y2': 25, 'units': 'mm'})
        axis = draft()['parameters']['entities'][-1]
        call('sketch.construction', {'entityIds': [axis['id']], 'construction': True})
        call('sketch.constraint', {'type': 'fixed', 'pointIds': [axis['startPointId']]})
        call('sketch.constraint', {'type': 'fixed', 'pointIds': [axis['endPointId']]})
        result = call('sketch.constraint', {'type': 'symmetry', 'entityIds': [left['id'], right['id'], axis['id']]})
        relation = result['draftSketch']['parameters']['constraints'][-1]
        assert relation['pointIds'] == [left['centerPointId'], right['centerPointId']]
        result = call('sketch.dimension', {'kind': 'diameter', 'entityId': left['id'], 'value': 20,
                      'units': 'mm', 'annotation': {'uMm': -37, 'vMm': -17}})
        diameter = result['draftSketch']['parameters']['dimensions'][-1]['id']
        call('sketch.dimension', {'kind': 'horizontal', 'pointIds': [left['centerPointId'], right['centerPointId']],
             'value': 40, 'units': 'mm', 'annotation': {'uMm': 0, 'vMm': 23}})
        assert call('app.state')['activeSession']['solverDiagnostics']['dof'] == 1
        def verify(expected_radius):
            data = draft()['parameters']
            points = {p['id']: p for p in data['points']}
            a, b = [points[x] for x in relation['pointIds']]
            assert math.isclose(a['uMm'] + b['uMm'], 0, abs_tol=1e-5)
            assert math.isclose(a['vMm'], b['vMm'], abs_tol=1e-5)
            assert next(c for c in data['constraints'] if c['id'] == relation['id']) == relation
            circles = [e for e in data['entities'] if e['type'] == 'circle']
            assert [c['id'] for c in circles] == [left['id'], right['id']]
            assert all(math.isclose(c['radiusMm'], expected_radius, abs_tol=1e-5) for c in circles)
            return a
        verify(10)
        call('sketch.lookAt')
        call('capture.screenshot', {'path': str(output / 'symmetric-circles.png')})
        initial = draft()
        sketch_id = initial['id']
        call('sketch.finish')
        call('sketch.edit', {'sketchId': sketch_id})
        call('sketch.dimensionUpdate', {'dimensionId': diameter, 'value': 24, 'units': 'mm'})
        previous = verify(12)
        call('sketch.movePoint', {'pointId': left['centerPointId'], 'x': -24, 'y': 5, 'units': 'mm'})
        moved = verify(12)
        assert abs(moved['vMm'] - previous['vMm']) > 0.1
        assert math.isclose(moved['uMm'], -20, abs_tol=1e-5)
        before = draft()
        call('sketch.dimension', {'kind': 'diameter', 'entityId': right['id'], 'value': 30, 'units': 'mm'}, 'sketch_solve_failed')
        assert draft() == before
        call('capture.screenshot', {'path': str(output / 'symmetric-circles-edited.png')})
        gpu = call('app.state')['viewport']['gpu']
        call('sketch.finish')
        final = call('document.inspect')['authored']
        call('history.undo')
        assert call('document.inspect')['authored']['features'][0] == initial
        call('history.redo')
        assert call('document.inspect')['authored'] == final
        saved = output / 'symmetric-circles.alloy'
        call('document.save', {'path': str(saved)})
        call('document.new', {'discard': False})
        call('document.open', {'path': str(saved)})
        assert call('document.inspect')['authored'] == final
        (output / 'result.json').write_text(json.dumps({'status': 'passed', 'relation': relation,
            'initialSketch': initial, 'authored': final, 'gpu': gpu, 'transcript': transcript}, indent=2) + '\n')
    print('PASS: circle symmetry, diameter/translation edits, atomic conflict, history and reopen')


if __name__ == '__main__':
    main()
