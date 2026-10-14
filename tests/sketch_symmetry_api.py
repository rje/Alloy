#!/usr/bin/env python3
"""One real-app plate width edit driven through a retained two-line symmetry relation."""
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
        session = call('sketch.begin', {'plane': 'XY', 'name': 'Symmetric plate'})
        for x1, y1, x2, y2 in [(-30, 0, 30, 0), (30, 0, 30, 30),
                                 (30, 30, -30, 30), (-30, 30, -30, 0)]:
            call('sketch.line', {'x1': x1, 'y1': y1, 'x2': x2, 'y2': y2, 'units': 'mm'})
        def draft():
            return call('app.state')['activeSession']['draftSketch']
        sides = draft()['parameters']['entities']
        for first, second in [(0, 1), (1, 2), (2, 3), (3, 0)]:
            if sides[first]['endPointId'] != sides[second]['startPointId']:
                call('sketch.constraint', {'type': 'coincident', 'pointIds':
                    [sides[first]['endPointId'], sides[second]['startPointId']]})
        call('sketch.line', {'x1': 0, 'y1': -5, 'x2': 0, 'y2': 35, 'units': 'mm'})
        axis_entity = draft()['parameters']['entities'][-1]
        axis = axis_entity['id']
        call('sketch.construction', {'entityIds': [axis], 'construction': True})
        call('sketch.constraint', {'type': 'fixed', 'pointIds': [axis_entity['startPointId']]})
        call('sketch.constraint', {'type': 'vertical', 'entityIds': [axis]})
        call('sketch.constraint', {'type': 'vertical', 'entityIds': [sides[3]['id']]})
        result = call('sketch.constraint', {'type': 'symmetry', 'entityIds': [sides[3]['id'], sides[1]['id'], axis]})
        relation = next(c for c in result['draftSketch']['parameters']['constraints'] if c['type'] == 'symmetry')
        assert len(relation['pointIds']) == 4 and relation['entityIds'][2] == axis
        before = call('command.preview', {'session': session['id']})['sketchGeometry']['profiles']
        width = call('sketch.dimension', {'kind': 'horizontal', 'entityId': sides[0]['id'], 'value': 60, 'units': 'mm'})
        width_id = width['draftSketch']['parameters']['dimensions'][-1]['id']
        call('sketch.dimension', {'kind': 'vertical', 'entityId': sides[3]['id'], 'value': 30, 'units': 'mm'})
        before_conflict = draft()
        call('sketch.dimension', {'kind': 'horizontal', 'entityId': sides[0]['id'], 'value': 61, 'units': 'mm'},
             'sketch_solve_failed')
        assert draft() == before_conflict
        sketch_id = draft()['id']
        profile_id = before[0]['profileId']
        call('sketch.finish')
        extrude = call('command.begin', {'command': 'solid.extrude', 'inputs':
            {'sketchId': sketch_id, 'profileId': profile_id, 'distance': 5, 'units': 'mm'}})
        call('command.preview', {'session': extrude['id']})
        call('command.commit', {'session': extrude['id']})
        initial_solid = call('document.inspect')
        assert math.isclose(initial_solid['evaluation']['bodies'][0]['volumeMm3'], 9000, abs_tol=1e-4)
        call('sketch.edit', {'sketchId': sketch_id})
        call('sketch.dimensionUpdate', {'dimensionId': width_id, 'value': 70, 'units': 'mm'})
        updated = draft()
        pts = {p['id']: p for p in updated['parameters']['points']}
        p0, p1, p2, p3 = [pts[x] for x in relation['pointIds']]
        assert math.isclose(p0['uMm'] + p1['uMm'], 0, abs_tol=1e-5)
        assert math.isclose(p2['uMm'] + p3['uMm'], 0, abs_tol=1e-5)
        assert math.isclose(abs(p0['uMm'] - p1['uMm']), 70, abs_tol=1e-5), (p0, p1, relation, updated['parameters']['entities'])
        call('capture.screenshot', {'path': str(output / 'symmetric-plate-sketch.png')})
        call('sketch.finish')
        final_solid = call('document.inspect')
        assert math.isclose(final_solid['evaluation']['bodies'][0]['volumeMm3'], 10500, abs_tol=1e-3)
        call('view.home')
        call('capture.screenshot', {'path': str(output / 'symmetric-plate-solid.png')})
        call('history.undo')
        assert math.isclose(call('document.inspect')['evaluation']['bodies'][0]['volumeMm3'], 9000, abs_tol=1e-4)
        call('history.redo')
        assert math.isclose(call('document.inspect')['evaluation']['bodies'][0]['volumeMm3'], 10500, abs_tol=1e-3)
        saved = output / 'symmetric-plate.alloy'
        call('document.save', {'path': str(saved)})
        authored = call('document.inspect')['authored']
        call('document.new', {'discard': False})
        call('document.open', {'path': str(saved)})
        assert call('document.inspect')['authored'] == authored
        (output / 'result.json').write_text(json.dumps({'status': 'passed', 'calls': len(transcript),
            'relationId': relation['id'], 'widthDimensionId': width_id, 'profileBefore': before,
            'authored': authored, 'transcript': transcript}, indent=2) + '\n')
    print('PASS: retained two-line symmetry, width edit, history and reopen')


if __name__ == '__main__':
    main()
