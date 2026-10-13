#!/usr/bin/env python3
"""Real-app control spline/comb ingredient; native gestures and full lesson are separate."""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path
from app_harness import application


def check_quadratic(details, height):
    assert details['degree'] == 2 and details['maximumDegree'] == 5
    comb = details['curvatureComb']
    assert comb['enabled'] and len(comb['samples']) >= 4
    assert math.isclose(comb['gainMmSquared'], comb['lengthMm']**2*comb['scale']/4, rel_tol=1e-8)
    defined = [s for s in comb['samples'] if s['defined']]
    assert len(defined) == len(comb['samples'])
    for sample in defined:
        t = sample['parameter']
        dx, dy = 80, 2*height*(1-2*t)
        speed = math.hypot(dx, dy)
        curvature = -320*height/speed**3
        assert math.isclose(sample['uMm'], 80*t, abs_tol=1e-6)
        assert math.isclose(sample['vMm'], 2*height*t*(1-t), abs_tol=1e-6)
        assert math.isclose(sample['signedCurvaturePerMm'], curvature, abs_tol=1e-8)
        if not sample['clipped']:
            # Reviewed display convention is outward, opposite the curvature vector.
            gain = curvature*comb['gainMmSquared']
            assert math.isclose(sample['tipUMm'], sample['uMm']+dy/speed*gain, abs_tol=1e-5)
            assert math.isclose(sample['tipVMm'], sample['vMm']-dx/speed*gain, abs_tol=1e-5)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    out = Path(args.output).resolve()
    out.mkdir(parents=True, exist_ok=True)
    report = {'status': 'incomplete', 'scope': 'Shared API mixed fit/control spline and comb ingredient',
              'appSha256': hashlib.sha256(Path(args.app).read_bytes()).hexdigest()}
    transcript = []
    try:
        with application(args.app, out) as (call, transcript):
            def state():
                return call('app.state')['activeSession']
            def draft():
                return state()['draftSketch']
            def inspect():
                return call('document.inspect')
            def control(parameters):
                return next(e for e in parameters['entities'] if e['type'] == 'controlSpline')
            def details(result):
                return next(e for e in result['sketchGeometry']['entities'] if e['id'] == control_id)
            def without_comb(parameters):
                result = copy.deepcopy(parameters)
                for entity in result['entities']:
                    entity.pop('curvatureComb', None)
                return result
            call('sketch.begin', {'plane': 'XY'})
            call('sketch.splineAdd', {'points': [{'uMm': -100, 'vMm': -70},
                 {'uMm': -80, 'vMm': -35}, {'uMm': -50, 'vMm': -70}]})
            fit_id = draft()['parameters']['entities'][0]['id']
            added = call('sketch.controlSplineAdd', {'points': [{'uMm': 0, 'vMm': 0},
                 {'uMm': 40, 'vMm': 60}, {'uMm': 80, 'vMm': 0}]})
            entity = control(draft()['parameters'])
            control_id = entity['id']
            point_ids = entity['pointIds']
            assert entity['maximumDegree'] == 5
            before_comb = state()
            call('sketch.curvatureComb', {'entityId': fit_id, 'enabled': True, 'density': 24, 'scale': .1})
            comb = call('sketch.curvatureComb', {'entityId': control_id, 'enabled': True, 'density': 24, 'scale': .1})
            check_quadratic(details(comb), 60)
            after_comb = state()
            assert without_comb(after_comb['draftSketch']['parameters']) == without_comb(before_comb['draftSketch']['parameters'])
            assert after_comb['solverDiagnostics']['dof'] == before_comb['solverDiagnostics']['dof']
            call('sketch.selectionSet', {'entityIds': [control_id]})
            call('view.home')
            call('view.orientation', {'name': 'top'})
            call('capture.screenshot', {'path': str(out/'control-cage-comb.png')})
            report['combGeometry'] = comb['sketchGeometry']
            # Close the control curve for an independent exact area oracle.
            call('sketch.line', {'x1': 80, 'y1': 0, 'x2': 0, 'y2': 0, 'units': 'mm'})
            sketch_id = draft()['id']
            call('sketch.finish')
            source = inspect()
            profiles = source['evaluation']['sketches'][0]['profiles']
            assert len(profiles) == 1 and math.isclose(profiles[0]['areaMm2'], 1600, abs_tol=1e-5)
            before = source['authored']
            path = out/'fit-control-splines.alloy'
            call('document.save', {'path': str(path)})
            call('sketch.edit', {'sketchId': sketch_id})
            moved = call('sketch.movePoint', {'pointId': point_ids[1], 'x': 40, 'y': 80, 'units': 'mm'})
            assert control(draft()['parameters'])['pointIds'] == point_ids
            check_quadratic(details(moved), 80)
            call('sketch.finish')
            after = inspect()
            assert math.isclose(after['evaluation']['sketches'][0]['profiles'][0]['areaMm2'], 6400/3, abs_tol=1e-5)
            call('history.undo')
            assert inspect()['authored'] == before
            call('history.redo')
            assert inspect()['authored'] == after['authored']
            call('history.undo')
            call('document.open', {'path': str(path)})
            assert inspect()['authored'] == before
            reopened = inspect()['evaluation']['sketches'][0]
            check_quadratic(next(e for e in reopened['entities'] if e['id'] == control_id), 60)
            report.update(status='api-pass', sourceState=source, editedGeometry=after['evaluation'])
    finally:
        report['callCount'] = len(transcript)
        (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
        (out/'transcript.json').write_text(json.dumps(transcript, indent=2)+'\n')
    print('PASS: control spline/comb geometry, retained edit, history and reopen')


if __name__ == '__main__':
    main()
