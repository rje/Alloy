#!/usr/bin/env python3
"""Real-app fit spline edits/profile/history; native handles have a separate gate."""
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
    report = {'status': 'incomplete', 'scope': 'Shared API fit spline ingredient; no native-input claim',
              'appSha256': hashlib.sha256(Path(args.app).read_bytes()).hexdigest()}
    transcript = []
    try:
        with application(args.app, out) as (call, transcript):
            def inspect():
                return call('document.inspect')
            def draft():
                return call('app.state')['activeSession']['draftSketch']
            def spline():
                return next(e for e in draft()['parameters']['entities'] if e['type'] == 'fitSpline')
            def tangent(point, du, dv, error=None):
                return call('sketch.splineTangent', {'entityId': entity_id, 'pointId': point, 'duMm': du, 'dvMm': dv}, error)
            call('sketch.begin', {'plane': 'XY'})
            added = call('sketch.splineAdd', {'points': [{'uMm': 0, 'vMm': 0}, {'uMm': 15, 'vMm': 30}, {'uMm': 100, 'vMm': 0}]})
            entity_id = spline()['id']
            first, middle, last = spline()['pointIds']
            tangent(first, 50, 130)
            call('sketch.splineRemove', {'entityId': entity_id, 'pointId': middle})
            assert spline()['pointIds'] == [first, last]
            assert spline()['tangentHandles'][first] == {'duMm': 50, 'dvMm': 130}
            call('sketch.splineInsert', {'entityId': entity_id, 'parameter': .25})
            current = spline()['pointIds']
            assert len(current) == 3 and current[0] == first and current[-1] == last and current[1] != middle
            inserted = current[1]
            tangent(inserted, 80, 40)
            changed = tangent(inserted, 160, 80)
            details = next(e for e in changed['sketchGeometry']['entities'] if e['id'] == entity_id)
            assert details['degree'] == 5 and details['continuity'] == 'C2'
            before_refusal = draft()
            tangent(inserted, 0, 0, 'sketch_solve_failed')
            assert draft() == before_refusal
            assert call('app.state')['activeSession']['solverDiagnostics']['splineHandleDof'] > 0
            call('view.home')
            call('view.orientation', {'name': 'top'})
            call('capture.screenshot', {'path': str(out/'edited-fit-spline.png')})
            report['editedState'] = call('app.state')
            call('sketch.line', {'x1': 100, 'y1': 0, 'x2': 0, 'y2': 0, 'units': 'mm'})
            sketch_id = draft()['id']
            call('sketch.finish')
            source = inspect()
            profiles = source['evaluation']['sketches'][0]['profiles']
            assert len(profiles) == 1 and math.isfinite(profiles[0]['areaMm2']) and profiles[0]['areaMm2'] > 0
            before = source['authored']
            path = out/'fit-point-spline.alloy'
            call('document.save', {'path': str(path)})
            call('sketch.edit', {'sketchId': sketch_id})
            tangent(inserted, 80, 40)
            call('sketch.finish')
            after = inspect()
            assert after['authored'] != before
            assert not math.isclose(after['evaluation']['sketches'][0]['profiles'][0]['areaMm2'], profiles[0]['areaMm2'], abs_tol=1e-4)
            call('history.undo')
            assert inspect()['authored'] == before
            call('history.redo')
            assert inspect()['authored'] == after['authored']
            call('history.undo')
            call('document.open', {'path': str(path)})
            assert inspect()['authored'] == before
            assert math.isclose(inspect()['evaluation']['sketches'][0]['profiles'][0]['areaMm2'], profiles[0]['areaMm2'], abs_tol=1e-6)
            report.update(status='api-pass', sourceState=source, editedGeometry=after['evaluation'])
    finally:
        report['callCount'] = len(transcript)
        (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
        (out/'transcript.json').write_text(json.dumps(transcript, indent=2)+'\n')
    print('PASS: fit spline API editing, real profile, history and reopen')


if __name__ == '__main__':
    main()
