#!/usr/bin/env python3
"""Real-app retained fit spline Offset, explicit Line closure and Intersect."""
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
    report = {'status': 'incomplete', 'scope': 'Shared API exact open fit Offset and Intersect; native input separately checked',
              'appSha256': hashlib.sha256(Path(args.app).read_bytes()).hexdigest()}
    transcript = []
    try:
        with application(args.app, out) as (call, transcript):
            def inspect(): return call('document.inspect')
            def draft(): return call('app.state')['activeSession']['draftSketch']
            def by_id(values): return {item['id']: item for item in values}
            call('sketch.begin', {'plane': 'XY'})
            call('sketch.rectangle', {'x1': -15, 'y1': -30, 'x2': 8, 'y2': 110, 'units': 'mm'})
            call('sketch.finish')
            stock_sketch = inspect()['evaluation']['sketches'][-1]
            call('solid.extrude', {'sketchId': stock_sketch['sketchId'],
                 'profileId': stock_sketch['profiles'][0]['profileId'], 'distance': 8, 'units': 'mm'})
            stock = inspect()['evaluation']['bodies'][0]
            call('sketch.begin', {'plane': 'XY'})
            call('sketch.splineAdd', {'points': [
                {'uMm': 0, 'vMm': -20}, {'uMm': 8, 'vMm': 20},
                {'uMm': -6, 'vMm': 60}, {'uMm': 4, 'vMm': 100}]})
            source = next(e for e in draft()['parameters']['entities'] if e['type'] == 'fitSpline')
            before = draft()
            call('sketch.tool', {'tool': 'offset'})
            pending = call('sketch.offsetPreview', {'sourceEntityId': source['id'], 'distance': -5, 'units': 'mm'})
            assert pending['pendingSplineOffset']['valid'] and draft() == before
            call('view.home')
            call('capture.screenshot', {'path': str(out/'api-preview.png')})
            call('sketch.offsetAccept')
            offset = next(e for e in draft()['parameters']['entities'] if e['type'] == 'fitSplineOffset')
            assert offset['sourceEntityId'] == source['id'] and offset['signedDistanceMm'] == -5
            assert len([e for e in draft()['parameters']['entities'] if e['type'] == 'fitSplineOffset']) == 1
            points = by_id(draft()['parameters']['points'])
            for source_id, derived_id in ((source['startPointId'], offset['startPointId']),
                                          (source['endPointId'], offset['endPointId'])):
                a, b = points[source_id], points[derived_id]
                call('sketch.line', {'x1': a['uMm'], 'y1': a['vMm'], 'x2': b['uMm'], 'y2': b['vMm'],
                     'units': 'mm', 'startSnapTarget': {'kind': 'point', 'pointId': source_id},
                     'endSnapTarget': {'kind': 'point', 'pointId': derived_id}})
            parameters = draft()['parameters']
            connectors = [e for e in parameters['entities'] if e['type'] == 'line']
            assert len(connectors) == 2 and {e['endPointId'] for e in connectors} == {
                offset['startPointId'], offset['endPointId']}
            sketch_id = draft()['id']
            call('sketch.finish')
            sketch = inspect()['evaluation']['sketches'][-1]
            assert len(sketch['profiles']) == 1 and sketch['profiles'][0]['areaMm2'] > 400
            profile_id = sketch['profiles'][0]['profileId']
            call('solid.extrude', {'sketchId': sketch_id, 'profileId': profile_id, 'distance': 8,
                 'units': 'mm', 'operation': 'intersect', 'targetBodyIds': [stock['bodyId']]})
            original = inspect()
            assert len(original['evaluation']['bodies']) == 1
            assert original['evaluation']['bodies'][0]['bodyId'] == stock['bodyId']
            assert original['evaluation']['bodies'][0]['valid'] and original['evaluation']['bodies'][0]['closed']
            call('view.home')
            call('capture.screenshot', {'path': str(out/'api-intersect.png')})
            call('sketch.edit', {'sketchId': sketch_id})
            call('sketch.splineOffsetUpdate', {'offsetEntityId': offset['id'], 'distance': -4, 'units': 'mm'})
            assert next(e for e in draft()['parameters']['entities'] if e['id'] == offset['id'])['signedDistanceMm'] == -4
            call('sketch.finish')
            edited = inspect()
            assert edited['evaluation']['sketches'][-1]['profiles'][0]['profileId'] == profile_id
            assert not math.isclose(edited['evaluation']['bodies'][0]['volumeMm3'],
                                    original['evaluation']['bodies'][0]['volumeMm3'], abs_tol=1e-4)
            call('history.undo')
            assert inspect()['authored'] == original['authored']
            call('history.redo')
            assert inspect()['authored'] == edited['authored']
            path = out/'retained-fit-offset.alloy'
            call('document.save', {'path': str(path)})
            call('document.open', {'path': str(path)})
            assert inspect()['authored'] == edited['authored']
            report.update(status='api-pass', original=original, edited=edited,
                          profileId=profile_id, sourceEntityId=source['id'], offsetEntityId=offset['id'])
    finally:
        report['callCount'] = len(transcript)
        (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
        (out/'transcript.json').write_text(json.dumps(transcript, indent=2)+'\n')
    print('PASS: exact retained fit Offset, Line closure, Intersect, edit/history/reopen')


if __name__ == '__main__':
    main()
