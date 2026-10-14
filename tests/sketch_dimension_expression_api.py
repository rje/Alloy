#!/usr/bin/env python3
"""Real-app local sketch dN formula and dependent profile edit, via shared commands."""
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
    report = {'status': 'incomplete', 'scope': 'Shared API sketch formula; native inline input is separate'}
    transcript = []
    try:
        with application(args.app, out) as (call, transcript):
            def draft():
                return call('app.state')['activeSession']['draftSketch']
            def parameters():
                return draft()['parameters']
            def inspect():
                return call('document.inspect')
            def line(x1, y1, x2, y2):
                call('sketch.line', {'x1': x1, 'y1': y1, 'x2': x2, 'y2': y2, 'units': 'mm'})
                return parameters()['entities'][-1]
            call('sketch.begin', {'plane': 'XY', 'units': 'mm'})
            call('sketch.centerToCenterSlot', {'firstX': -90, 'firstY': -30,
                'secondX': -45, 'secondY': -30, 'width': 10, 'units': 'mm'})
            group = parameters()['slotGroups'][0]
            call('sketch.constraint', {'type': 'horizontal', 'entityIds': [group['centerlineId']]})
            call('sketch.dimension', {'kind': 'linear', 'entityId': group['centerlineId'], 'value': 45, 'units': 'mm'})
            call('sketch.dimension', {'kind': 'radius', 'entityId': group['arcIds'][0], 'value': 5, 'units': 'mm'})
            bottom = line(0, 0, 100, 0)
            right = line(100, 0, 100, 100)
            top = line(100, 100, -50, 110)
            line(-50, 110, 0, 0)
            call('sketch.constraint', {'type': 'fixed', 'pointIds': [bottom['startPointId']]})
            call('sketch.constraint', {'type': 'horizontal', 'entityIds': [bottom['id']]})
            call('sketch.constraint', {'type': 'vertical', 'entityIds': [right['id']]})
            call('sketch.constraint', {'type': 'perpendicular', 'entityIds': [top['id'], right['id']]})
            call('sketch.constraint', {'type': 'equal', 'entityIds': [bottom['id'], right['id']]})
            call('sketch.dimension', {'kind': 'vertical', 'entityId': right['id'], 'value': 100,
                'units': 'mm', 'annotation': {'uMm': 120, 'vMm': 50}})
            driver = parameters()['dimensions'][-1]
            assert driver['name'] == 'd3'
            call('sketch.dimension', {'kind': 'horizontal', 'entityId': top['id'],
                'expression': 'd3 + 50', 'units': 'mm'})
            dependent = parameters()['dimensions'][-1]
            assert dependent['name'] == 'd4' and dependent['referenceIds'] == [driver['id']]
            assert math.isclose(dependent['valueMm'], 150, abs_tol=1e-6)
            sketch_id = draft()['id']
            call('sketch.lookAt')
            call('view.orientation', {'name': 'top'})
            call('capture.screenshot', {'path': str(out / 'formula-100-150.png')})
            call('sketch.finish')
            original = inspect()['authored']
            original_sketch = inspect()['evaluation']['sketches'][0]
            assert len(original_sketch['profiles']) == 2
            assert math.isclose(max(p['areaMm2'] for p in original_sketch['profiles']), 12500, abs_tol=1e-5)
            saved = out / 'sketch-expression.alloy'
            call('document.save', {'path': str(saved)})
            call('sketch.edit', {'sketchId': sketch_id})
            call('sketch.dimensionUpdate', {'dimensionId': driver['id'], 'value': 120, 'units': 'mm'})
            updated = next(d for d in parameters()['dimensions'] if d['id'] == dependent['id'])
            assert math.isclose(updated['valueMm'], 170, abs_tol=1e-6)
            call('sketch.finish')
            edited = inspect()['authored']
            edited_sketch = inspect()['evaluation']['sketches'][0]
            assert len(edited_sketch['profiles']) == 2
            assert math.isclose(max(p['areaMm2'] for p in edited_sketch['profiles']), 17400, abs_tol=1e-5)
            call('history.undo')
            assert inspect()['authored'] == original
            call('history.redo')
            assert inspect()['authored'] == edited
            call('history.undo')
            call('document.open', {'path': str(saved)})
            assert inspect()['authored'] == original
            report.update({'status': 'api-pass', 'sketchId': sketch_id,
                'driverId': driver['id'], 'dependentId': dependent['id'],
                'originalProfiles': original_sketch['profiles'],
                'editedProfiles': edited_sketch['profiles']})
    finally:
        report['callCount'] = len(transcript)
        (out / 'result.json').write_text(json.dumps(report, indent=2) + '\n')
        (out / 'transcript.json').write_text(json.dumps(transcript, indent=2) + '\n')
    print('PASS: sketch dimension expression shared API and dependent profile edit')


if __name__ == '__main__':
    main()
