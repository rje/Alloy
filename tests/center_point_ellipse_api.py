#!/usr/bin/env python3
"""Shared real-app ellipse geometry/edit acceptance; native input is separate."""
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
    report = {'status': 'incomplete', 'scope': 'Center Point Ellipse shared API, not native input or full lesson',
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
            def dof(expected):
                actual = session()['solverDiagnostics']['dof']
                assert actual == expected, (expected, actual)
                report.setdefault('dofSequence', []).append(actual)
            def verify(parameters, geometry, first_full):
                entities, points = by_id(parameters['entities']), by_id(parameters['points'])
                assert entities[ellipse['id']] == ellipse, 'Retain authored ellipse/axis identifiers'
                first, second = entities[ellipse['firstAxisId']], entities[ellipse['secondAxisId']]
                assert first['construction'] and second['construction']
                center = points[ellipse['centerPointId']]
                assert math.hypot(center['uMm'], center['vMm']) < 1e-6
                for axis, length, horizontal in [(first, first_full, True), (second, 100, False)]:
                    a, b = points[axis['startPointId']], points[axis['endPointId']]
                    assert math.isclose(math.hypot(b['uMm']-a['uMm'], b['vMm']-a['vMm']), length, abs_tol=1e-5)
                    assert math.hypot(a['uMm']+b['uMm'], a['vMm']+b['vMm']) < 1e-5
                    assert abs(b['vMm'] if horizontal else b['uMm']) < 1e-5
                info = next(e for e in geometry['entities'] if e['id'] == ellipse['id'])
                assert info['type'] == 'ellipse' and math.dist(info['centerMm'], [0, 0, 0]) < 1e-6
                assert math.isclose(info['firstRadiusMm'], first_full/2, abs_tol=1e-5)
                assert math.isclose(info['secondRadiusMm'], 50, abs_tol=1e-5)
                assert math.isclose(info['majorRadiusMm'], max(first_full/2, 50), abs_tol=1e-5)
                assert math.isclose(info['minorRadiusMm'], min(first_full/2, 50), abs_tol=1e-5)
                assert len(geometry['profiles']) == 1, geometry['profiles']
                assert math.isclose(geometry['profiles'][0]['areaMm2'], first_full*25*math.pi, abs_tol=1e-4)
                assert geometry['solver']['dof'] == 0
                return info

            call('sketch.begin', {'plane': 'XY'})
            call('sketch.centerPointEllipse', {'centerX': 0, 'centerY': 0, 'firstX': 130, 'firstY': 0,
                 'contourX': 78, 'contourY': 40, 'units': 'mm', 'centerSnapTarget': {'kind': 'origin'}})
            parameters = draft()['parameters']
            ellipse = next(e for e in parameters['entities'] if e['type'] == 'ellipse')
            equivalent = {parameters['originPointId']}
            while True:
                expanded = set(equivalent)
                for constraint in parameters['constraints']:
                    ids = set(constraint.get('pointIds', []))
                    if constraint['type'] == 'coincident' and ids & equivalent:
                        expanded.update(ids)
                if expanded == equivalent:
                    break
                equivalent = expanded
            assert ellipse['centerPointId'] in equivalent, 'Retained center must be associated with origin'
            dof(3)
            call('sketch.constraint', {'type': 'horizontal', 'entityIds': [ellipse['firstAxisId']]})
            dof(2)
            call('sketch.dimension', {'kind': 'linear', 'entityId': ellipse['firstAxisId'], 'value': 275, 'units': 'mm'})
            first_dimension = draft()['parameters']['dimensions'][-1]['id']
            dof(1)
            source = call('sketch.dimension', {'kind': 'linear', 'entityId': ellipse['secondAxisId'], 'value': 100, 'units': 'mm'})
            dof(0)
            verify(draft()['parameters'], source['sketchGeometry'], 275)
            sketch_id = draft()['id']
            call('view.home')
            call('view.orientation', {'name': 'top'})
            call('capture.screenshot', {'path': str(out/'defined-ellipse.png')})
            report['definedState'] = call('app.state')
            call('sketch.finish')
            source_state = inspect()
            verify(source_state['authored']['features'][0]['parameters'], source_state['evaluation']['sketches'][0], 275)
            before = source_state['authored']
            path = out/'center-point-ellipse.alloy'
            call('document.save', {'path': str(path)})
            call('sketch.edit', {'sketchId': sketch_id})
            # Equality and crossing exercise OCCT conic canonicalization without changing retained intent.
            for full in (100, 80):
                edited = call('sketch.dimensionUpdate', {'dimensionId': first_dimension, 'value': full, 'units': 'mm'})
                verify(draft()['parameters'], edited['sketchGeometry'], full)
            call('sketch.finish')
            after = inspect()
            verify(after['authored']['features'][0]['parameters'], after['evaluation']['sketches'][0], 80)
            assert after['authored'] != before
            call('history.undo')
            assert inspect()['authored'] == before
            call('history.redo')
            assert inspect()['authored'] == after['authored']
            call('history.undo')
            call('document.open', {'path': str(path)})
            reopened = inspect()
            assert reopened['authored'] == before
            verify(reopened['authored']['features'][0]['parameters'], reopened['evaluation']['sketches'][0], 275)
            report.update(status='api-pass', sourceState=source_state, editedState=after)
    finally:
        report['callCount'] = len(transcript)
        (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
        (out/'transcript.json').write_text(json.dumps(transcript, indent=2)+'\n')
    print('PASS: ellipse origin/full axes/area/DOF, retained edit, history and reopen')


if __name__ == '__main__':
    main()
