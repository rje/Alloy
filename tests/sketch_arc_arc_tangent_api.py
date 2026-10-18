#!/usr/bin/env python3
"""Arc-arc finite contact, edits and refusal through Alloy's real entrypoint."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    output = Path(args.output).resolve()
    with application(args.app, output) as (call, transcript):
        call('sketch.begin', {'plane': 'XY', 'name': 'Finite arc-arc contact'})
        def draft():
            return call('app.state')['activeSession']['draftSketch']
        call('sketch.addArc', dict(mode='center', centerX=0, centerY=0, radius=10, startAngleDeg=-90, sweepAngleDeg=180, units='mm'))
        circle = draft()['parameters']['entities'][-1]
        call('sketch.addArc', dict(mode='center', centerX=math.sqrt(14**2-3**2), centerY=3, radius=4,
                                  startAngleDeg=90, sweepAngleDeg=180, units='mm'))
        arc = draft()['parameters']['entities'][-1]
        call('sketch.constraint', dict(type='fixed', pointIds=[circle['centerPointId']]))
        # A vertical center/start pair retains the authored start direction.
        call('sketch.constraint', dict(type='vertical', pointIds=[arc['centerPointId'], arc['startPointId']]))
        call('sketch.constraint', dict(type='vertical', pointIds=[circle['centerPointId'], circle['startPointId']]))
        def dimension(**kw):
            return call('sketch.dimension', {**kw, 'units': 'mm'})['draftSketch']['parameters']['dimensions'][-1]['id']
        circle_radius = dimension(kind='radius', entityId=circle['id'], value=10)
        radius = dimension(kind='radius', entityId=arc['id'], value=4)
        sweep = dimension(kind='arcSweep', entityId=arc['id'], valueDeg=180)
        other_sweep = dimension(kind='arcSweep', entityId=circle['id'], valueDeg=180)
        offset = dimension(kind='vertical', pointIds=[circle['centerPointId'], arc['centerPointId']], value=3)
        call('sketch.constraint', dict(type='tangent', entityIds=[arc['id'], circle['id']]))
        relation = draft()['parameters']['constraints'][-1]
        assert relation['type'] == 'arcArcTangent' and relation['side'] == 1
        def verify(r, y):
            p = draft()['parameters']
            points = {x['id']: x for x in p['points']}
            entities = {x['id']: x for x in p['entities']}
            a, c = points[arc['centerPointId']], points[circle['centerPointId']]
            e = entities[arc['id']]
            assert math.isclose(math.hypot(a['uMm']-c['uMm'], a['vMm']-c['vMm']), 10+r, abs_tol=1e-6)
            assert math.isclose(a['vMm']-c['vMm'], y, abs_tol=1e-6)
            assert math.isclose(e['radiusMm'], r, abs_tol=1e-6)
            assert math.isclose(e['sweepAngleDeg'], 180, abs_tol=1e-6)
            contacts = []
            assert math.isclose(entities[circle['id']]['radiusMm'], 10, abs_tol=1e-6)
            for own, other in [(arc, circle), (circle, arc)]:
                entity = entities[own['id']]
                center, target = points[own['centerPointId']], points[other['centerPointId']]
                assert math.isclose(entity['sweepAngleDeg'], 180, abs_tol=1e-6)
                angle = math.degrees(math.atan2(target['vMm']-center['vMm'], target['uMm']-center['uMm']))
                parameter = ((angle-entity['startAngleDeg']) % 360)/entity['sweepAngleDeg']
                assert 0 < parameter < 1
                contacts.append((center['uMm']+entity['radiusMm']*math.cos(math.radians(angle)),
                                 center['vMm']+entity['radiusMm']*math.sin(math.radians(angle))))
            assert math.dist(*contacts) < 1e-6
            assert next(x for x in p['constraints'] if x['id'] == relation['id']) == relation
        verify(4, 3)
        for dimension_id, u, v in [(circle_radius, -12, -12), (radius, 23, 9), (sweep, 22, -3), (other_sweep, 7, 0), (offset, 26, -4)]:
            call('sketch.annotationMove', dict(dimensionId=dimension_id, uMm=u, vMm=v))
        call('sketch.lookAt')
        call('capture.screenshot', {'path': str(output / 'initial.png')})
        sketch_id = draft()['id']
        call('sketch.finish')
        call('sketch.edit', {'sketchId': sketch_id})
        call('sketch.dimensionUpdate', dict(dimensionId=radius, value=6, units='mm'))
        call('sketch.dimensionUpdate', dict(dimensionId=offset, value=4, units='mm'))
        verify(6, 4)
        before, document = draft(), call('document.inspect')
        for sweep_id in [sweep, other_sweep]:
            call('sketch.dimensionUpdate', dict(dimensionId=sweep_id, valueDeg=30), 'sketch_solve_failed')
            assert draft() == before and call('document.inspect') == document
        call('capture.screenshot', {'path': str(output / 'edited.png')})
        gpu = call('app.state')['viewport']['gpu']
        call('sketch.finish')
        edited = call('document.inspect')['authored']
        call('history.undo')
        call('sketch.edit', {'sketchId': sketch_id})
        verify(4, 3)
        call('sketch.cancel')
        call('history.redo')
        assert call('document.inspect')['authored'] == edited
        saved = output / 'arc-arc-tangent.alloy'
        call('document.save', {'path': str(saved)})
        authored = call('document.inspect')['authored']
        call('document.new', {'discard': False})
        call('document.open', {'path': str(saved)})
        assert call('document.inspect')['authored'] == authored
        call('sketch.edit', {'sketchId': sketch_id})
        verify(6, 4)
        call('sketch.cancel')
        (output / 'result.json').write_text(json.dumps(dict(status='passed', gpu=gpu, authored=authored, transcript=transcript), indent=2)+'\n')
    print('PASS: finite arc-arc contact, radius/center edits, atomic sweep refusal, history/reopen')

if __name__ == '__main__':
    main()
