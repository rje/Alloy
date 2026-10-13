#!/usr/bin/env python3
"""Four actual side-pair sketch fillets on a typed two-point Rectangle."""
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
        call('sketch.twoPointRectangle')
        started = call('sketch.begin', {'plane': 'XY', 'units': 'mm'})
        sketch_id = started['draftSketch']['id']
        call('sketch.lookAt')

        def pixel(u, v):
            p = call('view.projectSketchPoint', {'sketchId': sketch_id, 'uMm': u, 'vMm': v})
            return {'x': p['x'], 'y': p['y']}

        def active():
            return call('app.state')['activeSession']

        def draft():
            return active()['draftSketch']

        call('sketch.placePoint', pixel(0, 0))
        call('sketch.pointerMove', pixel(20, -10))
        call('sketch.twoPointRectanglePreview', {'widthText': '60', 'heightText': '30'})
        call('sketch.twoPointRectangleAccept')
        call('sketch.tool', {'tool': 'select'})
        base = draft()
        lines = [e['id'] for e in base['parameters']['entities']]
        width_id, height_id = [d['id'] for d in base['parameters']['dimensions']]
        assert len(lines) == 4
        call('view.zoom', {'factor': 0.7})
        center = pixel(30, -15)
        viewport = call('app.state')['viewport']
        call('view.pan', {'x': viewport['width'] / 2 - center['x'],
                          'y': viewport['height'] / 2 - center['y']})

        side_pairs = [((30, 0), (60, -15)), ((60, -15), (30, -30)),
                      ((30, -30), (0, -15)), ((0, -15), (30, 0))]

        def choose(pair):
            call('sketch.tool', {'tool': 'fillet'})
            first, second = pair
            call('sketch.placePoint', pixel(*first))
            assert 'firstSideId' in active()['pendingFillet']
            call('sketch.placePoint', pixel(*second))
            pending = active()['pendingFillet']
            assert pending['sidePair'] and len(pending['corners']) == 1

        def span(sketch, dimension_id):
            params = sketch['parameters']
            points = {p['id']: p for p in params['points']}
            d = next(d for d in params['dimensions'] if d['id'] == dimension_id)
            a, b = [points[id] for id in d['pointIds']]
            axis = 'uMm' if d['kind'] == 'horizontal' else 'vMm'
            return d, abs(a[axis] - b[axis])

        for index, pair in enumerate(side_pairs):
            before = draft()
            choose(pair)
            preview = call('sketch.filletPreview', {'value': 6, 'units': 'mm'})
            assert draft() == before and preview['previewed']
            if index == 3:
                assert preview['diagnostics']['releasedOriginCoincidence']
                call('capture.screenshot', {'path': str(output / 'rounded-preview.png')})
                call('sketch.filletCancel')
                assert draft() == before
                call('sketch.tool', {'tool': 'select'})
                choose(pair)
            result = call('sketch.filletAccept', {'value': 6, 'units': 'mm'})
            assert len(result['createdArcIds']) == 1
            current = draft()
            assert {e['id'] for e in current['parameters']['entities'] if e['type'] == 'line'} == set(lines)
            for dim_id, expected in [(width_id, 60), (height_id, 30)]:
                dimension, measured = span(current, dim_id)
                assert not dimension['driving'] and 'valueMm' not in dimension
                assert math.isclose(measured, expected, abs_tol=1e-5)
        final = draft()
        geometry = call('command.preview', {'session': active()['id']})['sketchGeometry']
        assert len(geometry['profiles']) == 1
        area = geometry['profiles'][0]['areaMm2']
        assert math.isclose(area, 1800 - 36 * (4 - math.pi), abs_tol=1e-5)
        call('capture.screenshot', {'path': str(output / 'rounded-accepted.png')})
        last_radius = next(d for d in final['parameters']['dimensions'] if d['kind'] == 'radius')
        call('sketch.dimensionUpdate', {'dimensionId': last_radius['id'], 'value': 5.5, 'units': 'mm'})
        edited = draft()
        for dim_id in (width_id, height_id):
            d, measured = span(edited, dim_id)
            assert not d['driving'] and measured > 0
        call('history.undo')
        assert draft() == final
        call('history.redo')
        assert draft() == edited
        call('history.undo')
        assert draft() == final
        call('sketch.finish')
        authored = call('document.inspect')['authored']
        path = output / 'rounded-rectangle.alloy'
        call('document.save', {'path': str(path)})
        call('document.open', {'path': str(path)})
        assert call('document.inspect')['authored'] == authored
        (output / 'report.json').write_text(json.dumps({
            'scope': 'Real entrypoint command-first Rectangle authoring and viewport-space side-pair fillet picks; native Qt key path separate.',
            'areaMm2': area, 'widthDimensionId': width_id,
            'heightDimensionId': height_id, 'callCount': len(transcript),
            'authored': authored}, indent=2) + '\n')
    print('PASS: four side-pair R6 fillets, overall spans, Cancel, edit, history and exact reopen')


if __name__ == '__main__':
    main()
