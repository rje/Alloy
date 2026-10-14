#!/usr/bin/env python3
"""Constrain a five-edge plate top and edit its width and split through the real app."""
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
        started = call('sketch.begin', {'plane': 'XY', 'name': 'Split top plate'})

        def draft():
            return call('app.state')['activeSession']['draftSketch']

        def line(x1, y1, x2, y2):
            call('sketch.line', {'x1': x1, 'y1': y1, 'x2': x2, 'y2': y2, 'units': 'mm'})
            return draft()['parameters']['entities'][-1]

        bottom = line(0, 0, 60, 0)
        right = line(60, 0, 60, 40)
        top_right = line(60, 40, 40, 40)
        top_left = line(40, 40, 0, 40.5)
        left = line(0, 40.5, 0, 0)
        assert top_right['endPointId'] == top_left['startPointId']
        call('sketch.constraint', {'type': 'fixed', 'pointIds': [bottom['startPointId']]})
        for kind, entity in [('horizontal', bottom), ('vertical', right),
                             ('horizontal', top_right), ('vertical', left)]:
            call('sketch.constraint', {'type': kind, 'entityIds': [entity['id']]})
        result = call('sketch.constraint', {'type': 'collinear',
            'entityIds': [top_left['id'], top_right['id']]})
        relation = next(c for c in result['draftSketch']['parameters']['constraints'] if c['type'] == 'collinear')
        assert relation['entityIds'] == [top_left['id'], top_right['id']]
        before_bad = draft()
        call('sketch.constraint', {'type': 'collinear',
            'entityIds': [top_right['id'], top_left['id']]}, 'sketch_solve_failed')
        assert draft() == before_bad
        width = call('sketch.dimension', {'kind': 'horizontal', 'entityId': bottom['id'], 'value': 60, 'units': 'mm'})
        width_id = width['draftSketch']['parameters']['dimensions'][-1]['id']
        call('sketch.dimension', {'kind': 'vertical', 'entityId': left['id'], 'value': 40, 'units': 'mm'})
        split = call('sketch.dimension', {'kind': 'horizontal', 'entityId': top_right['id'], 'value': 20, 'units': 'mm'})
        split_id = split['draftSketch']['parameters']['dimensions'][-1]['id']
        before_conflict = draft()
        call('sketch.dimension', {'kind': 'horizontal', 'entityId': bottom['id'], 'value': 61, 'units': 'mm'},
             'sketch_solve_failed')
        assert draft() == before_conflict
        preview = call('command.preview', {'session': started['id']})
        profiles = preview['sketchGeometry']['profiles']
        assert len(profiles) == 1, profiles
        sketch_id = draft()['id']
        call('capture.screenshot', {'path': str(output / 'collinear-plate-sketch.png')})
        call('sketch.finish')
        extrude = call('command.begin', {'command': 'solid.extrude', 'inputs':
            {'sketchId': sketch_id, 'profileId': profiles[0]['profileId'], 'distance': 5, 'units': 'mm'}})
        call('command.preview', {'session': extrude['id']})
        call('command.commit', {'session': extrude['id']})
        initial = call('document.inspect')['evaluation']['bodies'][0]['volumeMm3']
        assert math.isclose(initial, 12000, abs_tol=1e-3), initial
        call('sketch.edit', {'sketchId': sketch_id})
        call('sketch.dimensionUpdate', {'dimensionId': split_id, 'value': 25, 'units': 'mm'})
        call('sketch.dimensionUpdate', {'dimensionId': width_id, 'value': 65, 'units': 'mm'})
        edited = draft()
        assert next(c['id'] for c in edited['parameters']['constraints'] if c['type'] == 'collinear') == relation['id']
        points = {p['id']: p for p in edited['parameters']['points']}
        a, b = points[top_right['startPointId']], points[top_right['endPointId']]
        c = points[top_left['endPointId']]
        assert math.isclose(a['vMm'], b['vMm'], abs_tol=1e-6)
        assert math.isclose(c['vMm'], b['vMm'], abs_tol=1e-6)
        assert math.isclose(abs(a['uMm'] - b['uMm']), 25, abs_tol=1e-6)
        call('capture.screenshot', {'path': str(output / 'collinear-plate-edited.png')})
        call('sketch.finish')
        final = call('document.inspect')['evaluation']['bodies'][0]['volumeMm3']
        assert math.isclose(final, 13000, abs_tol=1e-3), final
        call('view.home')
        call('capture.screenshot', {'path': str(output / 'collinear-plate-solid.png')})
        call('history.undo')
        assert math.isclose(call('document.inspect')['evaluation']['bodies'][0]['volumeMm3'], initial, abs_tol=1e-3)
        call('history.redo')
        assert math.isclose(call('document.inspect')['evaluation']['bodies'][0]['volumeMm3'], final, abs_tol=1e-3)
        saved = output / 'collinear-plate.alloy'
        call('document.save', {'path': str(saved)})
        authored = call('document.inspect')['authored']
        call('document.new', {'discard': False})
        call('document.open', {'path': str(saved)})
        assert call('document.inspect')['authored'] == authored
        (output / 'result.json').write_text(json.dumps({'status': 'passed', 'calls': len(transcript),
            'relationId': relation['id'], 'widthDimensionId': width_id, 'splitDimensionId': split_id,
            'initialVolumeMm3': initial, 'finalVolumeMm3': final, 'authored': authored,
            'transcript': transcript}, indent=2) + '\n')
    print('PASS: retained Collinear plate, split/width edit, history and reopen')


if __name__ == '__main__':
    main()
