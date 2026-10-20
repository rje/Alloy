#!/usr/bin/env python3
"""Independent real-app two-solid overlap and clearance inspection fixture."""
import argparse
import json
import math
from pathlib import Path

from app_harness import application


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    out = Path(args.output).resolve()
    out.mkdir(parents=True, exist_ok=True)
    with application(args.app, out, command_timeout=90) as (call, transcript):
        def inspect():
            return call('document.inspect')

        def pair(first, second):
            return call('inspect.interferenceResult', {
                'first': {'bodyId': first['bodyId']},
                'second': {'bodyId': second['bodyId']}})

        call('window.resize', {'width': 1280, 'height': 900})
        for name, width, depth, height, x, y, z in (
            ('A', 10, 10, 10, 0, 0, 0),
            ('B overlap', 4, 4, 4, 8, 2, 3),
            ('B clear', 2, 4, 4, 12, 2, 3),
            ('B touch', 2, 4, 4, 10, 2, 3),
        ):
            call('solid.box', {'name': name, 'width': width, 'depth': depth,
                'height': height, 'x': x, 'y': y, 'z': z, 'units': 'mm'})
        bodies = inspect()['evaluation']['bodies']
        assert len(bodies) == 4
        a, overlap, clear, touch = bodies
        model = out/'inspection.alloy'
        call('document.save', {'path': str(model)})
        before = inspect()
        original_bytes = model.read_bytes()
        results = {}
        result = pair(a, overlap)
        assert result['kind'] == 'overlap' and math.isclose(
            result['canonical']['overlapVolumeMm3'], 32, abs_tol=1e-6), result
        assert {result['first']['bodyId'], result['second']['bodyId']} == {
            a['bodyId'], overlap['bodyId']}
        results['overlap'] = result
        result = pair(a, clear)
        assert result['kind'] == 'clearance' and math.isclose(
            result['canonical']['clearanceMm'], 2, abs_tol=1e-7), result
        closest = result['canonical']['closestPointsMm']
        assert len(closest) == 2
        assert math.isclose(math.dist(closest[0], closest[1]), 2, abs_tol=1e-7)
        results['clearance'] = result
        result = pair(a, touch)
        assert result['kind'] == 'touching' and math.isclose(
            result['canonical']['clearanceMm'], 0, abs_tol=1e-7), result
        results['touching'] = result
        for invalid, error in (({'sources': []}, 'interference_selection_count'),
                               ({'first': {'bodyId': a['bodyId']},
                                 'second': {'bodyId': a['bodyId']}}, 'interference_duplicate'),
                               ({'first': {'bodyId': a['bodyId']},
                                 'second': {'bodyId': 'missing'}}, 'interference_unavailable')):
            call('inspect.interferenceResult', invalid, error=error)
        assert inspect()['authored'] == before['authored']
        assert inspect()['undo'] == before['undo']
        assert inspect()['revision'] == before['revision']
        assert model.read_bytes() == original_bytes

        call('document.units', {'units': 'in'})
        inches = pair(a, overlap)
        assert inches['kind'] == 'overlap'
        assert math.isclose(inches['canonical']['overlapVolumeMm3'], 32, abs_tol=1e-6)
        assert math.isclose(inches['display']['volume'], 32/(25.4**3), rel_tol=1e-9)
        inch_gap = pair(a, clear)
        assert math.isclose(inch_gap['canonical']['clearanceMm'], 2, abs_tol=1e-7)
        assert math.isclose(inch_gap['display']['length'], 2/25.4, rel_tol=1e-9)
        call('document.units', {'units': 'mm'})

        overlap_feature = overlap['featureId']
        call('solid.box', {'featureId': overlap_feature, 'name': 'B overlap',
            'width': 4, 'depth': 4, 'height': 4, 'x': 7, 'y': 2, 'z': 3,
            'units': 'mm'})
        changed = pair(a, overlap)
        assert changed['kind'] == 'overlap' and math.isclose(
            changed['canonical']['overlapVolumeMm3'], 48, abs_tol=1e-6), changed
        call('history.undo')
        assert math.isclose(pair(a, overlap)['canonical']['overlapVolumeMm3'], 32, abs_tol=1e-6)
        call('history.redo')
        assert math.isclose(pair(a, overlap)['canonical']['overlapVolumeMm3'], 48, abs_tol=1e-6)
        call('document.save', {'path': str(model)})
        edited = inspect()
        call('document.open', {'path': str(model)})
        assert inspect()['authored'] == edited['authored']
        assert math.isclose(pair(a, overlap)['canonical']['overlapVolumeMm3'], 48, abs_tol=1e-6)
        call('view.home')
        call('capture.screenshot', {'path': str(out/'inspection-bodies.png')})
        gpu = call('app.state')['viewport']['gpu']
        (out/'report.json').write_text(json.dumps({'passed': True,
            'results': results, 'inches': inches, 'inchGap': inch_gap,
            'changed': changed, 'gpu': gpu}, indent=2)+'\n')
        (out/'transcript.json').write_text(json.dumps(transcript, indent=2)+'\n')


if __name__ == '__main__':
    main()
