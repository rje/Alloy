#!/usr/bin/env python3
"""Real-app combined slot ingredient; native input and full lesson are separate gates."""
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
    report = {'status': 'incomplete', 'scope': 'Shared API straight and arc slots, not full lesson or physical input',
              'appSha256': hashlib.sha256(Path(args.app).read_bytes()).hexdigest()}
    transcript = []
    try:
        with application(args.app, out) as (call, transcript):
            def session():
                return call('app.state')['activeSession']
            def draft():
                return session()['draftSketch']
            def inspect():
                return call('document.inspect')
            def by_id(values):
                return {v['id']: v for v in values}
            def verify(radius):
                state = inspect()
                feature = state['authored']['features'][0]
                params = feature['parameters']
                assert params['arcSlotGroups'][0] == group
                assert params['slotGroups'][0] == straight
                entities = by_id(params['entities'])
                points = by_id(params['points'])
                path = entities[group['pathArcId']]
                center = points[path['centerPointId']]
                assert math.hypot(center['uMm'], center['vMm']) < 1e-6
                for key, sign in [('startPointId', -1), ('endPointId', 1)]:
                    p = points[path[key]]
                    assert math.isclose(p['uMm'], sign*radius/math.sqrt(2), abs_tol=1e-5)
                    assert math.isclose(p['vMm'], radius/math.sqrt(2), abs_tol=1e-5)
                assert math.isclose(path['sweepAngleDeg'], -90, abs_tol=1e-5)
                for key, expected in [('outerArcId', radius+15), ('innerArcId', radius-15)]:
                    assert math.isclose(entities[group[key]]['radiusMm'], expected, abs_tol=1e-5)
                for cap in group['capIds']:
                    assert math.isclose(entities[cap]['radiusMm'], 15, abs_tol=1e-5)
                info = state['evaluation']['sketches'][0]
                areas = sorted(p['areaMm2'] for p in info['profiles'])
                expected = sorted([7500+625*math.pi, (15*radius+225)*math.pi])
                assert len(areas) == 2, info
                assert all(math.isclose(a, b, abs_tol=1e-4) for a, b in zip(areas, expected)), areas
                assert info['solver']['dof'] == 0, info['solver']
                return info

            call('sketch.begin', {'plane': 'XY'})
            call('sketch.centerPointSlot', {'centerX': 0, 'centerY': 0, 'endX': 68, 'endY': 12,
                 'width': 42, 'units': 'mm', 'centerSnapTarget': {'kind': 'origin'}})
            straight = draft()['parameters']['slotGroups'][0]
            call('sketch.constraint', {'type': 'horizontal', 'entityIds': [straight['centerlineId']]})
            call('sketch.dimension', {'kind': 'linear', 'entityId': straight['centerlineId'], 'value': 150, 'units': 'mm'})
            call('sketch.dimension', {'kind': 'radius', 'entityId': straight['arcIds'][0], 'value': 25, 'units': 'mm'})
            call('sketch.centerPointArcSlot', {'centerX': 0, 'centerY': 0, 'firstX': -100, 'firstY': 110,
                 'lastX': 105, 'lastY': 98, 'width': 28, 'units': 'mm', 'centerSnapTarget': {'kind': 'origin'}})
            group = draft()['parameters']['arcSlotGroups'][0]
            call('sketch.dimension', {'kind': 'radius', 'entityId': group['pathArcId'], 'value': 150, 'units': 'mm'})
            radius_id = draft()['parameters']['dimensions'][-1]['id']
            call('sketch.dimension', {'kind': 'radius', 'entityId': group['capIds'][0], 'value': 15, 'units': 'mm'})
            path = by_id(draft()['parameters']['entities'])[group['pathArcId']]
            call('sketch.constraint', {'type': 'horizontal', 'pointIds': [path['startPointId'], path['endPointId']]})
            params = draft()['parameters']
            points = by_id(params['points'])
            left = points[path['startPointId']]
            call('sketch.line', {'x1': left['uMm'], 'y1': left['vMm'], 'x2': 0, 'y2': 0, 'units': 'mm'})
            radial = draft()['parameters']['entities'][-1]
            assert radial['startPointId'] == path['startPointId']
            params = draft()['parameters']
            equivalent = {path['centerPointId'], params['originPointId']}
            while True:
                expanded = set(equivalent)
                for constraint in params['constraints']:
                    ids = set(constraint.get('pointIds', []))
                    if constraint['type'] == 'coincident' and ids & equivalent:
                        expanded.update(ids)
                if expanded == equivalent:
                    break
                equivalent = expanded
            assert radial['endPointId'] in equivalent, 'Radial line must retain origin/center association'
            call('sketch.construction', {'entityIds': [radial['id']], 'construction': True})
            params = draft()['parameters']
            points = by_id(params['points'])
            guide = by_id(params['entities'])[straight['centerlineId']]
            guide_sign = 1 if points[guide['endPointId']]['uMm'] > points[guide['startPointId']]['uMm'] else -1
            call('sketch.dimension', {'kind': 'angle', 'entityIds': [guide['id'], radial['id']],
                 'raySigns': [guide_sign, -1], 'valueDeg': 135, 'annotation': {'uMm': -35, 'vMm': 55}})
            assert session()['solverDiagnostics']['dof'] == 0
            sketch_id = draft()['id']
            call('view.home')
            call('view.orientation', {'name': 'top'})
            call('capture.screenshot', {'path': str(out/'defined-two-slots.png')})
            report['definedState'] = call('app.state')
            call('sketch.finish')
            report['sourceGeometry'] = verify(150)
            before = inspect()['authored']
            path = out/'two-slots.alloy'
            call('document.save', {'path': str(path)})
            call('sketch.edit', {'sketchId': sketch_id})
            call('sketch.dimensionUpdate', {'dimensionId': radius_id, 'value': 170, 'units': 'mm'})
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
            report.update(status='api-pass', sourceAuthored=before)
    finally:
        report['callCount'] = len(transcript)
        (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
        (out/'transcript.json').write_text(json.dumps(transcript, indent=2)+'\n')
    print('PASS: combined straight/arc slot geometry, retained edit, history and reopen')


if __name__ == '__main__':
    main()
