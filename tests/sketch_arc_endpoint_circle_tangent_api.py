#!/usr/bin/env python3
"""Explicit start/end arc-circle contact through the real application."""
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
        call('sketch.begin', {'plane': 'XY', 'name': 'Explicit arc endpoint Tangent'})
        def draft():
            return call('app.state')['activeSession']['draftSketch']
        call('sketch.addCircle', dict(centerX=0, centerY=0, diameter=20, units='mm'))
        circle = draft()['parameters']['entities'][-1]
        call('sketch.addArc', dict(mode='center', centerX=18, centerY=0, radius=8,
                                  startAngleDeg=180, sweepAngleDeg=100, units='mm'))
        arc = draft()['parameters']['entities'][-1]
        call('sketch.constraint', dict(type='fixed', pointIds=[circle['centerPointId']]))
        call('sketch.constraint', dict(type='horizontal', pointIds=[arc['centerPointId'], arc['startPointId']]))
        def dimension(**kw):
            return call('sketch.dimension', {**kw, 'units': 'mm'})['draftSketch']['parameters']['dimensions'][-1]['id']
        circle_radius = dimension(kind='radius', entityId=circle['id'], value=10)
        radius = dimension(kind='radius', entityId=arc['id'], value=8)
        sweep = dimension(kind='arcSweep', entityId=arc['id'], valueDeg=100)
        call('sketch.constraint', dict(type='tangent', endpoint='start', entityIds=[arc['id'], circle['id']]))
        relation = draft()['parameters']['constraints'][-1]
        assert relation['type'] == 'arcEndpointCircleTangent' and relation['side'] == 1
        assert relation['contact'] == dict(endpoint='start', pointId=arc['startPointId'], sweepSign=1)
        def verify(r):
            p = draft()['parameters']
            points = {x['id']: x for x in p['points']}
            entities = {x['id']: x for x in p['entities']}
            a, c = points[arc['centerPointId']], points[circle['centerPointId']]
            contact, opposite = points[arc['startPointId']], points[arc['endPointId']]
            e = entities[arc['id']]
            assert math.isclose(math.hypot(a['uMm']-c['uMm'], a['vMm']-c['vMm']), 10+r, abs_tol=1e-6)
            assert math.isclose(math.hypot(contact['uMm']-c['uMm'], contact['vMm']-c['vMm']), 10, abs_tol=1e-6)
            radial_a = (contact['uMm']-a['uMm'], contact['vMm']-a['vMm'])
            radial_c = (contact['uMm']-c['uMm'], contact['vMm']-c['vMm'])
            assert abs(radial_a[0]*radial_c[1]-radial_a[1]*radial_c[0]) < 1e-6
            assert math.hypot(opposite['uMm']-contact['uMm'], opposite['vMm']-contact['vMm']) > 1
            assert math.isclose(e['radiusMm'], r, abs_tol=1e-6)
            assert math.isclose(e['sweepAngleDeg'], 100, abs_tol=1e-6)
            assert next(x for x in p['constraints'] if x['id'] == relation['id']) == relation
        verify(8)
        for dimension_id, u, v in [(circle_radius, -15, -13), (radius, 32, 8), (sweep, 32, -13)]:
            call('sketch.annotationMove', dict(dimensionId=dimension_id, uMm=u, vMm=v))
        call('sketch.lookAt')
        call('capture.screenshot', {'path': str(output / 'initial.png')})
        sketch_id = draft()['id']
        call('sketch.finish')
        call('sketch.edit', {'sketchId': sketch_id})
        call('sketch.dimensionUpdate', dict(dimensionId=radius, value=10, units='mm'))
        verify(10)
        before, document = draft(), call('document.inspect')
        call('sketch.dimension', dict(kind='horizontal', pointIds=[circle['centerPointId'], arc['centerPointId']], value=30, units='mm'), 'sketch_solve_failed')
        assert draft() == before and call('document.inspect') == document
        # Badge selection is independent of geometry; native pointer/deletion is a separate gate.
        glyphs = call('app.state')['viewport']['gpu']['sketchConstraintGlyphs']
        assert next(g for g in glyphs if g['constraintId'] == relation['id'])
        call('sketch.constraintSelect', {'constraintId': relation['id']})
        call('capture.screenshot', {'path': str(output / 'edited.png')})
        gpu = call('app.state')['viewport']['gpu']
        call('sketch.finish')
        edited = call('document.inspect')['authored']
        call('history.undo')
        call('sketch.edit', {'sketchId': sketch_id})
        verify(8)
        call('sketch.cancel')
        call('history.redo')
        assert call('document.inspect')['authored'] == edited
        saved = output / 'arc-endpoint-circle-tangent.alloy'
        call('document.save', {'path': str(saved)})
        authored = call('document.inspect')['authored']
        call('document.new', {'discard': False})
        call('document.open', {'path': str(saved)})
        assert call('document.inspect')['authored'] == authored
        call('sketch.edit', {'sketchId': sketch_id})
        verify(10)
        call('sketch.cancel')
        (output / 'result.json').write_text(json.dumps(dict(status='passed', gpu=gpu, authored=authored, transcript=transcript), indent=2)+'\n')
    print('PASS: explicit endpoint-circle contact, radius edit, atomic impossible edit, history/reopen')

if __name__ == '__main__':
    main()
