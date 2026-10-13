#!/usr/bin/env python3
"""Real-app slot ingredient; native gestures and full tutorial have separate gates."""
import argparse
import hashlib
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
    report = {'status': 'incomplete', 'scope': 'Shared API slot ingredient, not physical input or full lesson',
              'appSha256': hashlib.sha256(Path(args.app).read_bytes()).hexdigest()}
    transcript = []
    try:
        with application(args.app, out) as (call, transcript):
            def inspect():
                return call('document.inspect')
            def session():
                return call('app.state')['activeSession']
            def draft():
                return session()['draftSketch']
            def verify(length):
                info = inspect()['evaluation']['sketches'][0]
                assert len(info['profiles']) == 1
                assert math.isclose(info['profiles'][0]['areaMm2'], 50*length+625*math.pi, abs_tol=1e-5)
                p = inspect()['authored']['features'][0]['parameters']
                assert p['slotGroups'][0] == group
                pts = {v['id']: v for v in p['points']}
                c = pts[group['centerPointId']]
                assert math.hypot(c['uMm'], c['vMm']) < 1e-6
                return info
            call('sketch.begin', {'plane': 'XY'})
            call('sketch.centerPointSlot', {'centerX': 0, 'centerY': 0, 'endX': 68, 'endY': 12,
                                           'width': 42, 'units': 'mm', 'centerSnapTarget': {'kind': 'origin'}})
            assert session()['solverDiagnostics']['dof'] == 3
            group = draft()['parameters']['slotGroups'][0]
            call('sketch.constraint', {'type': 'horizontal', 'entityIds': [group['centerlineId']]})
            assert session()['solverDiagnostics']['dof'] == 2
            call('sketch.dimension', {'kind': 'linear', 'entityId': group['centerlineId'], 'value': 150, 'units': 'mm'})
            length_id = draft()['parameters']['dimensions'][-1]['id']
            assert session()['solverDiagnostics']['dof'] == 1
            call('sketch.dimension', {'kind': 'radius', 'entityId': group['arcIds'][0], 'value': 25, 'units': 'mm'})
            assert session()['solverDiagnostics']['dof'] == 0
            sketch_id = draft()['id']
            call('view.home')
            call('view.orientation', {'name': 'top'})
            call('capture.screenshot', {'path': str(out/'defined-slot.png')})
            report['definedState'] = call('app.state')
            call('sketch.finish')
            report['sourceGeometry'] = verify(150)
            before = inspect()['authored']
            path = out/'center-point-slot.alloy'
            call('document.save', {'path': str(path)})
            call('sketch.edit', {'sketchId': sketch_id})
            call('sketch.dimensionUpdate', {'dimensionId': length_id, 'value': 170, 'units': 'mm'})
            call('sketch.finish')
            report['editedGeometry'] = verify(170)
            after = inspect()['authored']
            call('history.undo')
            assert inspect()['authored'] == before
            call('history.redo')
            assert inspect()['authored'] == after
            call('history.undo')
            call('document.open', {'path': str(path)})
            assert inspect()['authored'] == before
            verify(150)
            report['status'] = 'api-pass'
            report['sourceAuthored'] = before
    finally:
        report['callCount'] = len(transcript)
        (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
        (out/'transcript.json').write_text(json.dumps(transcript, indent=2)+'\n')
    print('PASS: slot API geometry, retained edit, history and reopen')


if __name__ == '__main__':
    main()
