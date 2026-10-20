#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Bounded Activity 2 retained end-circle lifecycle through the real executable.

This API fixture is separate from bracket_midpoint_controls' actual-hit input.
"""
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
    fixture = Path(__file__).parent / 'workflows/fixtures/bracket-activity1.alloy'
    with application(args.app, output) as (call, transcript):
        call('document.open', {'path': str(fixture.resolve()), 'discard': True})
        source = call('document.inspect')['authored']['features'][0]
        session = call('feature.editDialog', {'featureId': source['id']})
        call('sketch.lookAt')
        call('view.fit')
        call('view.zoom', {'factor': 0.8})

        def draft():
            return call('app.state')['activeSession']['draftSketch']

        def geometry():
            return call('command.preview', {'session': call('app.state')['activeSession']['id']})['sketchGeometry']

        def line(index):
            p = draft()['parameters']
            points = {r['id']: (r['uMm'], r['vMm']) for r in p['points']}
            edge = p['entities'][index]
            return points[edge['startPointId']], points[edge['endPointId']]

        def project(point):
            result = call('view.projectSketchPoint', {'sketchId': source['id'], 'uMm': point[0], 'vMm': point[1]})
            return {key: result[key] for key in ('x', 'y')}

        ids = [e['id'] for e in source['parameters']['entities']]
        call('sketch.selectionSet', {'entityIds': [ids[2], ids[5]]})
        call('sketch.construction', {'construction': True})
        assert not geometry()['profiles']
        for end, tangent in ((5, 0), (2, 1)):
            call('sketch.tool', {'tool': 'circle'})
            a, b = line(end)
            center = tuple((x+y)/2 for x, y in zip(a, b))
            call('sketch.placePoint', project(center))
            a, b = line(tangent)
            dx, dy = b[0]-a[0], b[1]-a[1]
            t = ((center[0]-a[0])*dx+(center[1]-a[1])*dy)/(dx*dx+dy*dy)
            contact = (a[0]+t*dx, a[1]+t*dy)
            snap = call('sketch.pointerMove', project(contact))['snap']
            assert snap['target'] == {'kind': 'tangent', 'entityId': ids[tangent]}
            call('sketch.placePoint', project(contact))
        call('sketch.tool', {'tool': 'select'})

        def verify(radius):
            p = draft()['parameters']
            assert [e['id'] for e in p['entities'][:6]] == ids
            assert all(p['entities'][i]['construction'] for i in (2, 5))
            points = {r['id']: (r['uMm'], r['vMm']) for r in p['points']}
            for j, (end, tangent) in enumerate(((5, 0), (2, 1))):
                circle = p['entities'][6+j]
                a, b = line(end)
                actual = points[circle['centerPointId']]
                assert math.dist(actual, tuple((x+y)/2 for x, y in zip(a, b))) < 1e-6
                assert math.isclose(circle['radiusMm'], radius, abs_tol=1e-6)
                assert any(c['type'] == 'midpoint' and c.get('entityIds') == [ids[end]] and c.get('pointIds') == [circle['centerPointId']] for c in p['constraints'])
                assert any(c['type'] == 'tangent' and set(c.get('entityIds', [])) == {circle['id'], ids[tangent]} for c in p['constraints'])
                a, b = line(tangent)
                distance = abs((b[0]-a[0])*(actual[1]-a[1])-(b[1]-a[1])*(actual[0]-a[0]))/math.dist(a, b)
                assert math.isclose(distance, radius, abs_tol=1e-6)
            return geometry()

        before = draft()
        initial_geometry = verify(50)
        call('capture.screenshot', {'path': str(output/'circles100.png')})
        call('sketch.dimensionEdit', {'dimensionId': source['parameters']['dimensions'][-1]['id']})
        call('sketch.dimensionAccept', {'value': 120})
        after = draft()
        edited_geometry = verify(60)
        assert before != after
        call('history.undo')
        assert draft() == before
        call('history.redo')
        assert draft() == after
        verify(60)
        call('capture.screenshot', {'path': str(output/'circles120.png')})
        call('sketch.finish')
        authored = call('document.inspect')['authored']
        call('document.save', {'path': str(output/'bracket-midpoint.alloy')})
        call('document.new', {'discard': False})
        call('document.open', {'path': str(output/'bracket-midpoint.alloy')})
        assert call('document.inspect')['authored'] == authored
        state = call('app.state')
        assert state['viewport']['gpu']['hardwareAccelerated']
        call('window.resize', {'width': 1400, 'height': 850})
        capture = call('capture.screenshot', {'path': str(output/'reopened-resized.png')})
        resized = call('app.state')
        dpr = resized['ui']['devicePixelRatio']
        assert capture['viewportPixelWidth'] == round(resized['viewport']['width']*dpr)
        assert capture['viewportPixelHeight'] == round(resized['viewport']['height']*dpr)
        report = {'status': 'passed', 'scope': 'real executable API, separate from native picks',
                  'before': before, 'after': after, 'initialGeometry': initial_geometry,
                  'editedGeometry': edited_geometry, 'finalState': resized, 'capture': capture,
                  'transcript': transcript}
    (output/'receipt.json').write_text(json.dumps(report, indent=2)+'\n')


if __name__ == '__main__':
    main()
