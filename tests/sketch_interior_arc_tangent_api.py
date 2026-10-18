#!/usr/bin/env python3
"""Retained interior arc/segment contact through the real Alloy entrypoint."""
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
        call('sketch.begin', {'plane': 'XY', 'name': 'Interior contact and rounded profile'})
        def draft():
            return call('app.state')['activeSession']['draftSketch']
        def line(x1, y1, x2, y2):
            call('sketch.line', dict(x1=x1, y1=y1, x2=x2, y2=y2, units='mm'))
            return draft()['parameters']['entities'][-1]
        reference = line(-30, -20, -25, -20)
        call('sketch.construction', {'entityIds': [reference['id']], 'construction': True})
        call('sketch.constraint', {'type': 'fixed', 'pointIds': [reference['startPointId']]})
        call('sketch.addArc', dict(mode='center', centerX=0, centerY=0, radius=10,
                                   startAngleDeg=0, sweepAngleDeg=180, units='mm'))
        arc = draft()['parameters']['entities'][-1]
        chord = line(-10, 0, 10, 0)
        edge = line(-20, 11, 20, 11)
        def dimension(**kw):
            result = call('sketch.dimension', {**kw, 'units': 'mm'})
            return result['draftSketch']['parameters']['dimensions'][-1]['id']
        call('sketch.constraint', {'type': 'horizontal', 'entityIds': [chord['id']]})
        call('sketch.constraint', {'type': 'horizontal', 'entityIds': [edge['id']]})
        radius = dimension(kind='radius', entityId=arc['id'], value=10)
        sweep = dimension(kind='arcSweep', entityId=arc['id'], valueDeg=180)
        dimension(kind='horizontal', pointIds=[reference['startPointId'], arc['centerPointId']], value=30)
        dimension(kind='horizontal', pointIds=[reference['startPointId'], edge['startPointId']], value=10)
        location = dimension(kind='vertical', pointIds=[reference['startPointId'], edge['startPointId']], value=31)
        length = dimension(kind='linear', pointIds=[edge['startPointId'], edge['endPointId']], value=40)
        for dimension_id, u, v in [(radius, -16, 18), (sweep, 0, 34), (length, 0, 24)]:
            call('sketch.annotationMove', {'dimensionId': dimension_id, 'uMm': u, 'vMm': v})
        call('sketch.constraint', {'type': 'tangent', 'entityIds': [edge['id'], arc['id']]})
        relation = next(c for c in draft()['parameters']['constraints'] if c['type'] == 'arcTangent')
        assert relation['contact']['sweepSign'] == 1
        def contact(r, y):
            p = draft()['parameters']
            points = {v['id']: v for v in p['points']}
            entities = {v['id']: v for v in p['entities']}
            a, b, center = [points[x] for x in (edge['startPointId'], edge['endPointId'], arc['centerPointId'])]
            assert math.isclose(a['vMm'], y, abs_tol=1e-6)
            assert math.isclose(center['vMm']+r, y, abs_tol=1e-6)
            assert math.isclose(entities[arc['id']]['radiusMm'], r, abs_tol=1e-6)
            assert a['uMm'] < center['uMm'] < b['uMm']
            assert next(c for c in p['constraints'] if c['type'] == 'arcTangent') == relation
        contact(10, 11)
        call('sketch.lookAt')
        call('capture.screenshot', {'path': str(output / 'initial.png')})
        sketch_id = draft()['id']
        call('sketch.finish')
        # The arc/chord bound the rounded region. The tangent segment touches
        # its interior and is deliberately not claimed to be a simple wire edge.
        profiles = call('document.inspect')['evaluation']['sketches'][0]['profiles']
        assert len(profiles) == 1, profiles
        call('solid.extrude', {'sketchId': sketch_id, 'profileId': profiles[0]['profileId'], 'distance': 5, 'units': 'mm'})
        call('sketch.edit', {'sketchId': sketch_id})
        call('sketch.dimensionUpdate', {'dimensionId': radius, 'value': 12, 'units': 'mm'})
        call('sketch.dimensionUpdate', {'dimensionId': location, 'value': 34, 'units': 'mm'})
        contact(12, 14)
        before = draft()
        # Moving the finite segment far to the right must not rematch contact.
        horizontal = next(d['id'] for d in before['parameters']['dimensions'] if d.get('pointIds') == [reference['startPointId'], edge['startPointId']])
        call('sketch.dimensionUpdate', {'dimensionId': horizontal, 'value': 60, 'units': 'mm'}, 'sketch_solve_failed')
        assert draft() == before
        call('capture.screenshot', {'path': str(output / 'edited.png')})
        gpu = call('app.state')['viewport']['gpu']
        call('sketch.finish')
        edited = call('document.inspect')
        assert math.isclose(edited['evaluation']['bodies'][0]['volumeMm3'], 5*math.pi*12**2/2, abs_tol=1e-3)
        call('history.undo')
        call('sketch.edit', {'sketchId': sketch_id})
        contact(10, 11)
        call('sketch.cancel')
        call('history.redo')
        assert call('document.inspect')['authored'] == edited['authored']
        call('view.home')
        call('capture.screenshot', {'path': str(output / 'solid.png')})
        saved = output / 'interior-arc-tangent.alloy'
        call('document.save', {'path': str(saved)})
        authored = call('document.inspect')['authored']
        call('document.new', {'discard': False})
        call('document.open', {'path': str(saved)})
        assert call('document.inspect')['authored'] == authored
        call('sketch.edit', {'sketchId': sketch_id})
        contact(12, 14)
        call('sketch.cancel')
        (output / 'result.json').write_text(json.dumps(dict(status='passed', gpu=gpu, final=edited, transcript=transcript), indent=2)+'\n')
    print('PASS: interior contact, source edits, atomic branch loss, rounded-region solid, history/reopen')

if __name__ == '__main__':
    main()
