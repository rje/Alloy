#!/usr/bin/env python3
"""Source-order ornament prefix for retained Join diagnostics, not a full gate."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application


def create_seed(call, out):
    """Author the source through Intersect; reused by the future cumulative gate."""
    def inspect(): return call('document.inspect')
    def draft(): return call('app.state')['activeSession']['draftSketch']
    def entity(): return draft()['parameters']['entities'][-1]
    def capture(name): call('capture.screenshot', {'path': str(out/(name+'.png'))})
    assert not inspect()['authored']['features']
    call('window.resize', {'width': 1400, 'height': 1000})
    call('sketch.begin', {'plane': 'XZ', 'name': 'Radial profile'})
    call('sketch.line', {'x1': 0, 'y1': 0, 'x2': 0, 'y2': 80, 'units': 'mm'})
    axis = entity()
    call('sketch.line', {'x1': 0, 'y1': 0, 'x2': 5, 'y2': 0, 'units': 'mm',
         'startSnapTarget': {'kind': 'point', 'pointId': axis['startPointId']}})
    radial = entity()
    length = math.hypot(5, 80)
    offset = math.sqrt(50**2-length**2/4)
    cx, cy = 2.5-80/length*offset, 40-5/length*offset
    start, end = math.atan2(-cy, 5-cx), math.atan2(80-cy, -cx)
    call('sketch.addArc', {'mode': 'threePoint', 'startX': 5, 'startY': 0,
         'endX': 0, 'endY': 80, 'throughX': cx+50*math.cos((start+end)/2),
         'throughY': cy+50*math.sin((start+end)/2), 'units': 'mm'})
    arc = entity()
    for a, b in ((arc['startPointId'], radial['endPointId']), (arc['endPointId'], axis['endPointId'])):
        if a != b:
            call('sketch.constraint', {'type': 'coincident', 'pointIds': [a, b]})
    call('sketch.constraint', {'type': 'fixed', 'pointIds': [axis['startPointId']]})
    for kind, edge in (('vertical', axis), ('horizontal', radial)):
        call('sketch.constraint', {'type': kind, 'entityIds': [edge['id']]})
    for kind, edge, value in (('linear', axis, 80), ('linear', radial, 5), ('radius', arc, 50)):
        call('sketch.dimension', {'kind': kind, 'entityId': edge['id'], 'value': value, 'units': 'mm'})
    source_id = draft()['id']
    call('view.home'); capture('radial-profile')
    call('sketch.finish')
    profile = inspect()['evaluation']['sketches'][-1]['profiles'][0]['profileId']
    call('solid.revolve', {'sketchId': source_id, 'profileId': profile, 'axisEntityId': axis['id']})
    body = inspect()['evaluation']['bodies'][0]
    face = next(f for f in body['supportFaces'] if f['role'] == 'revolve.radial')
    call('solid.shell', {'targetBodyId': body['bodyId'], 'removedFace':
         {k: face[k] for k in ('producerFeatureId', 'bodyId', 'faceId')}, 'thicknessMm': 5})
    call('view.home'); capture('shell')
    before = inspect()['authored']['features']
    call('view.origin', {'visible': True})
    call('inspect.section', {'plane': 'XZ', 'hatch': True})
    analysis = inspect()['authored']['analyses'][-1]
    assert inspect()['authored']['features'] == before
    capture('section-hatched')
    call('analysis.visibility', {'analysisId': analysis['id'], 'visible': False})
    call('sketch.begin', {'plane': 'XZ', 'name': 'S shaped strip'})
    fixture = [(-4, -1), (6, 19), (-5, 45), (3, 89)]
    call('sketch.splineAdd', {'points': [{'uMm': x, 'vMm': y} for x, y in fixture]})
    spline = entity()
    call('sketch.splineOffset', {'sourceEntityId': spline['id'], 'distance': -5, 'units': 'mm'})
    derived = entity()
    points = {p['id']: p for p in draft()['parameters']['points']}
    for field in ('startPointId', 'endPointId'):
        a, b = points[spline[field]], points[derived[field]]
        call('sketch.line', {'x1': a['uMm'], 'y1': a['vMm'], 'x2': b['uMm'], 'y2': b['vMm'],
             'units': 'mm', 'startSnapTarget': {'kind': 'point', 'pointId': spline[field]},
             'endSnapTarget': {'kind': 'point', 'pointId': derived[field]}})
    strip_id = draft()['id']
    call('view.home'); capture('closed-strip')
    call('sketch.finish')
    strip = inspect()['evaluation']['sketches'][-1]
    assert len(strip['profiles']) == 1
    call('document.save', {'path': str(out/'ornament-pre-intersect.alloy')})
    call('solid.extrude', {'sketchId': strip_id, 'profileId': strip['profiles'][0]['profileId'],
         'distance': 60, 'units': 'mm', 'operation': 'intersect', 'targetBodyIds': [body['bodyId']]})
    state = inspect()
    assert len(state['evaluation']['bodies']) == 1
    seed = state['evaluation']['bodies'][0]
    assert seed['valid'] and seed['closed'] and seed['volumeMm3'] > 0
    call('view.home'); capture('intersect-seed')
    call('document.save', {'path': str(out/'ornament-seed.alloy')})
    return {'bodyId': seed['bodyId'], 'stripSketchId': strip_id, 'offsetEntityId': derived['id'],
            'fixturePointsMm': fixture, 'state': state}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    out = Path(args.output).resolve(); out.mkdir(parents=True, exist_ok=True)
    transcript = []; report = {'status': 'incomplete', 'fullTutorialAccepted': False}
    try:
        with application(args.app, out) as (call, transcript):
            report.update(create_seed(call, out))
            report['status'] = 'source-prefix-pass'
    finally:
        (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
        (out/'transcript.json').write_text(json.dumps(transcript, indent=2)+'\n')


if __name__ == '__main__':
    main()
