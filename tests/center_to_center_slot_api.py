#!/usr/bin/env python3
"""Real-app shared-command slot geometry; native three-click input is separate."""
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
    report = {'status': 'incomplete', 'scope': 'Shared API cap-center slot, geometry, history and storage'}
    transcript = []
    try:
        with application(args.app, out) as (call, transcript):
            def authored():
                return call('document.inspect')['authored']
            def draft():
                return call('app.state')['activeSession']['draftSketch']
            call('sketch.begin', {'plane': 'XY', 'units': 'mm'})
            call('sketch.centerToCenterSlot', {'firstX': 0.2, 'firstY': -0.3,
                'secondX': 42, 'secondY': 8, 'width': 10, 'units': 'mm',
                'firstSnapTarget': {'kind': 'origin'}})
            p = draft()['parameters']
            assert len(p['slotGroups']) == 1 and not p.get('dimensions')
            group = p['slotGroups'][0]
            guide = next(e for e in p['entities'] if e['id'] == group['centerlineId'])
            points = {point['id']: point for point in p['points']}
            assert math.hypot(points[guide['startPointId']]['uMm'], points[guide['startPointId']]['vMm']) < 1e-6
            assert math.hypot(points[guide['endPointId']]['uMm'] - 42,
                              points[guide['endPointId']]['vMm'] - 8) < 1e-6
            call('sketch.constraint', {'type': 'horizontal', 'entityIds': [group['centerlineId']]})
            call('sketch.dimension', {'kind': 'linear', 'entityId': group['centerlineId'],
                'value': 45, 'units': 'mm'})
            length_id = draft()['parameters']['dimensions'][-1]['id']
            call('sketch.dimension', {'kind': 'radius', 'entityId': group['arcIds'][0],
                'value': 5, 'units': 'mm'})
            sketch_id = draft()['id']
            call('view.home')
            call('view.orientation', {'name': 'top'})
            call('capture.screenshot', {'path': str(out / 'defined-center-to-center-slot.png')})
            call('sketch.finish')
            info = call('document.inspect')['evaluation']['sketches'][0]
            assert len(info['profiles']) == 1
            assert math.isclose(info['profiles'][0]['areaMm2'], 450 + 25*math.pi, abs_tol=1e-5)
            before = authored()
            saved = out / 'center-to-center-slot.alloy'
            call('document.save', {'path': str(saved)})
            call('sketch.edit', {'sketchId': sketch_id})
            call('sketch.dimensionUpdate', {'dimensionId': length_id, 'value': 50, 'units': 'mm'})
            call('sketch.finish')
            edited = call('document.inspect')['evaluation']['sketches'][0]
            assert math.isclose(edited['profiles'][0]['areaMm2'], 500 + 25*math.pi, abs_tol=1e-5)
            after = authored()
            call('history.undo')
            assert authored() == before
            call('history.redo')
            assert authored() == after
            call('history.undo')
            call('document.open', {'path': str(saved)})
            assert authored() == before
            report.update({'status': 'api-pass', 'areaMm2': info['profiles'][0]['areaMm2'],
                           'editedAreaMm2': edited['profiles'][0]['areaMm2'], 'sketchId': sketch_id,
                           'group': group, 'sourceAuthored': before})
    finally:
        report['callCount'] = len(transcript)
        (out / 'result.json').write_text(json.dumps(report, indent=2) + '\n')
        (out / 'transcript.json').write_text(json.dumps(transcript, indent=2) + '\n')
    print('PASS: center-to-center slot shared API, geometry, history and reopen')


if __name__ == '__main__':
    main()
