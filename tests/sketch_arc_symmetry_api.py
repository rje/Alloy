#!/usr/bin/env python3
"""Retained authored-arc symmetry on an editable rounded slot, through Alloy's real entrypoint."""
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
        session = call('sketch.begin', {'plane': 'XY', 'name': 'Symmetric rounded slot'})
        def draft():
            return call('app.state')['activeSession']['draftSketch']
        def arc(x, y, angle):
            call('sketch.addArc', {'mode': 'center', 'centerX': x, 'centerY': y,
                 'radius': 10, 'startAngleDeg': angle, 'sweepAngleDeg': 180, 'units': 'mm'})
            return draft()['parameters']['entities'][-1]
        def line(x1, y1, x2, y2):
            call('sketch.line', {'x1': x1, 'y1': y1, 'x2': x2, 'y2': y2, 'units': 'mm'})
            return draft()['parameters']['entities'][-1]
        left, right = arc(-20, 0, 90), arc(21, 1, 270)
        line(-20, -10, 21, -9)
        line(21, 11, -20, 10)
        axis = line(0, -25, 0, 25)
        call('sketch.construction', {'entityIds': [axis['id']], 'construction': True})
        call('sketch.constraint', {'type': 'fixed', 'pointIds': [axis['startPointId']]})
        call('sketch.constraint', {'type': 'vertical', 'entityIds': [axis['id']]})
        result = call('sketch.constraint', {'type': 'symmetry', 'entityIds': [left['id'], right['id'], axis['id']]})
        relation = next(c for c in result['draftSketch']['parameters']['constraints'] if c['type'] == 'symmetry')
        call('sketch.constraint', {'type': 'vertical', 'pointIds': [left['centerPointId'], left['startPointId']]})
        call('sketch.dimension', {'kind': 'arcSweep', 'entityId': left['id'], 'valueDeg': 180, 'annotation': {'uMm': -48, 'vMm': 25}})
        width = call('sketch.dimension', {'kind': 'diameter', 'entityId': left['id'], 'value': 20, 'units': 'mm', 'annotation': {'uMm': -42, 'vMm': -15}})
        width_id = width['draftSketch']['parameters']['dimensions'][-1]['id']
        length = call('sketch.dimension', {'kind': 'horizontal', 'pointIds': [left['centerPointId'], right['centerPointId']], 'value': 40, 'units': 'mm', 'annotation': {'uMm': 0, 'vMm': 23}})
        length_id = length['draftSketch']['parameters']['dimensions'][-1]['id']
        before_move = next(p['vMm'] for p in draft()['parameters']['points'] if p['id'] == left['centerPointId'])
        call('sketch.movePoint', {'pointId': left['centerPointId'], 'x': -20, 'y': 4, 'units': 'mm'})
        def reflections():
            data = draft()['parameters']
            points = {p['id']: p for p in data['points']}
            assert next(c for c in data['constraints'] if c['type'] == 'symmetry') == relation
            for a, b in zip(relation['pointIds'][::2], relation['pointIds'][1::2]):
                assert math.isclose(points[a]['uMm'] + points[b]['uMm'], 0, abs_tol=1e-5)
                assert math.isclose(points[a]['vMm'], points[b]['vMm'], abs_tol=1e-5)
            arcs = [e for e in data['entities'] if e['type'] == 'arc']
            assert math.isclose(arcs[0]['radiusMm'], arcs[1]['radiusMm'], abs_tol=1e-5)
            return arcs
        reflections()
        moved = next(p for p in draft()['parameters']['points'] if p['id'] == left['centerPointId'])
        assert abs(moved['vMm'] - before_move) > 0.1, (before_move, moved)
        before = draft()
        call('sketch.constraint', {'type': 'equal', 'entityIds': [left['id'], right['id']]}, 'sketch_solve_failed')
        assert draft() == before
        call('sketch.lookAt')
        call('capture.screenshot', {'path': str(output / 'rounded-slot-sketch.png')})
        profiles = call('command.preview', {'session': session['id']})['sketchGeometry']['profiles']
        assert len(profiles) == 1, profiles
        sketch_id = draft()['id']
        call('sketch.finish')
        extrusion = call('command.begin', {'command': 'solid.extrude', 'inputs':
            {'sketchId': sketch_id, 'profileId': profiles[0]['profileId'], 'distance': 5, 'units': 'mm'}})
        call('command.preview', {'session': extrusion['id']})
        call('command.commit', {'session': extrusion['id']})
        def volume():
            bodies = call('document.inspect')['evaluation']['bodies']
            assert len(bodies) == 1 and bodies[0]['valid']
            return bodies[0]['volumeMm3']
        initial = volume()
        assert math.isclose(initial, 5 * (40*20 + math.pi*100), abs_tol=1e-3), initial
        call('sketch.edit', {'sketchId': sketch_id})
        call('sketch.dimensionUpdate', {'dimensionId': width_id, 'value': 24, 'units': 'mm'})
        call('sketch.dimensionUpdate', {'dimensionId': length_id, 'value': 50, 'units': 'mm'})
        edited_arcs = reflections()
        assert all(math.isclose(a['radiusMm'], 12, abs_tol=1e-5) for a in edited_arcs)
        call('capture.screenshot', {'path': str(output / 'rounded-slot-edited.png')})
        call('sketch.finish')
        final = volume()
        assert math.isclose(final, 5 * (50*24 + math.pi*144), abs_tol=1e-3), final
        call('view.home')
        call('capture.screenshot', {'path': str(output / 'rounded-slot-solid.png')})
        gpu = call('app.state')['viewport']['gpu']
        call('history.undo')
        assert math.isclose(volume(), initial, abs_tol=1e-4)
        call('history.redo')
        assert math.isclose(volume(), final, abs_tol=1e-4)
        saved = output / 'rounded-slot.alloy'
        call('document.save', {'path': str(saved)})
        authored = call('document.inspect')['authored']
        call('document.new', {'discard': False})
        call('document.open', {'path': str(saved)})
        assert call('document.inspect')['authored'] == authored
        assert math.isclose(volume(), final, abs_tol=1e-4)
        (output / 'result.json').write_text(json.dumps({'status': 'passed', 'relation': relation,
            'initialVolumeMm3': initial, 'finalVolumeMm3': final, 'gpu': gpu,
            'authored': authored, 'transcript': transcript}, indent=2) + '\n')
    print('PASS: authored arc symmetry rounded slot, edits, extrusion, history, reopen')

if __name__ == '__main__':
    main()
