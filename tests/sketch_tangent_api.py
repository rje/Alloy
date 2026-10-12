#!/usr/bin/env python3
"""Bounded end-circle midpoint/tangent inference; region extraction is a separate gate."""
import argparse
import hashlib
import json
import math
from pathlib import Path
from app_harness import application


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--fixture', default=str(Path(__file__).parent / 'workflows/fixtures/sketch-tangent.json'))
    args = parser.parse_args()
    fixture = json.loads(Path(args.fixture).read_text())
    output = Path(args.output).resolve()
    captures = []
    with application(args.app, output) as (call, transcript):
        initial = call('app.state')
        session = call('sketch.begin', {'plane': 'XY', 'name': 'Retained end-circle inference'})

        def preview():
            return call('command.preview', {'session': session['id']})

        def draft():
            return preview()['authoredDraft']

        def project(u, v):
            point = call('view.projectSketchPoint', {'plane': 'XY', 'uMm': u, 'vMm': v})
            return {'x': point['x'], 'y': point['y']}

        # Explicit setup is distinct from the pointer-inference assertions below.
        vertices = fixture['seedVerticesMm']
        for (x1, y1), (x2, y2) in zip(vertices, vertices[1:]):
            call('sketch.line', {'x1': x1, 'y1': y1, 'x2': x2, 'y2': y2, 'units': 'mm'})
        edges = draft()['parameters']['entities']
        ids = [e['id'] for e in edges]
        call('sketch.constraint', {'type': 'fixed', 'pointIds': [edges[0]['startPointId']]})
        for a, b in fixture['perpendicularPairs']:
            call('sketch.constraint', {'type': 'perpendicular', 'entityIds': [ids[a], ids[b]]})
        for index in fixture['horizontalTargets']:
            call('sketch.constraint', {'type': 'horizontal', 'entityIds': [ids[index]]})
        for a, b in fixture['equalPairs']:
            call('sketch.constraint', {'type': 'equal', 'entityIds': [ids[a], ids[b]]})
        call('sketch.dimension', {'kind': 'horizontal', 'entityId': ids[0], 'value': 500, 'units': 'mm'})
        call('sketch.dimension', {'kind': 'angle', 'entityIds': [ids[0], ids[1]], 'raySigns': [1, 1], 'valueDeg': 45})
        call('sketch.dimension', {'kind': 'linear', 'entityId': ids[2], 'value': 100, 'units': 'mm'})
        cap_dimension = draft()['parameters']['dimensions'][-1]['id']
        call('sketch.construction', {'entityIds': [ids[2], ids[5]], 'construction': True})
        call('view.zoom', {'factor': 0.1})
        origin = project(0, 0)
        viewport = call('app.state')['viewport']
        call('view.pan', {'x': 100 - origin['x'], 'y': viewport['height'] - 100 - origin['y']})

        def points():
            return {p['id']: (p['uMm'], p['vMm']) for p in draft()['parameters']['points']}

        def line(index):
            p = points()
            return p[edges[index]['startPointId']], p[edges[index]['endPointId']]

        circles = []
        for end_index, tangent_index in fixture['endTangentPairs']:
            a, b = line(end_index)
            center = ((a[0]+b[0])/2, (a[1]+b[1])/2)
            a, b = line(tangent_index)
            dx, dy = b[0]-a[0], b[1]-a[1]
            t = ((center[0]-a[0])*dx+(center[1]-a[1])*dy)/(dx*dx+dy*dy)
            foot = (a[0]+t*dx, a[1]+t*dy)
            call('sketch.tool', {'tool': 'circle'})
            before = draft()
            history = call('app.state')['activeSession']['draftHistory']
            snap = call('sketch.pointerMove', project(*center))['snap']
            assert snap['status'] == 'acquired' and snap['target'] == {'kind': 'midpoint', 'entityId': ids[end_index]}
            call('sketch.placePoint', project(*center))
            tangent = call('sketch.pointerMove', project(*foot))['snap']
            assert tangent['status'] == 'acquired' and tangent['target'] == {'kind': 'tangent', 'entityId': ids[tangent_index]}, tangent
            assert draft() == before and call('app.state')['activeSession']['draftHistory'] == history
            captures.append(call('capture.screenshot', {'path': str(output / ('tangent-cue-' + str(end_index) + '.png'))}))
            call('sketch.placePoint', project(*foot))
            circle = draft()['parameters']['entities'][-1]
            assert circle['type'] == 'circle' and math.isclose(circle['radiusMm'], 50, abs_tol=1e-6)
            relations = draft()['parameters']['constraints']
            midpoint = next(c for c in relations if c['type'] == 'midpoint' and c['pointIds'] == [circle['centerPointId']])
            tangent = next(c for c in relations if c['type'] == 'tangent' and c['entityIds'][0] == circle['id'])
            assert midpoint['entityIds'] == [ids[end_index]]
            assert tangent['entityIds'] == [circle['id'], ids[tangent_index]] and tangent['side'] in (-1, 1)
            circles.append((circle, midpoint, tangent, end_index))
        call('sketch.tool', {'tool': 'select'})
        accepted = draft()
        assert preview()['sketchGeometry']['solver']['dof'] == 0
        call('sketch.dimensionUpdate', {'dimensionId': cap_dimension, 'value': 120, 'units': 'mm'})
        edited = draft()
        assert edited['parameters']['constraints'] == accepted['parameters']['constraints']
        p = points()
        for original, midpoint, tangent, index in circles:
            circle = next(e for e in edited['parameters']['entities'] if e['id'] == original['id'])
            assert math.isclose(circle['radiusMm'], 60, abs_tol=1e-6)
            a, b = line(index)
            assert all(math.isclose(v, (a[k]+b[k])/2, abs_tol=1e-6) for k, v in enumerate(p[circle['centerPointId']]))
        call('history.undo')
        assert draft() == accepted
        call('history.redo')
        assert draft() == edited
        captures.append(call('capture.screenshot', {'path': str(output / 'retained-tangent-glyphs.png')}))
        glyphs = call('app.state')['viewport']['gpu']['sketchConstraintGlyphs']
        for circle, midpoint, tangent, index in circles:
            assert any(g['kind'] == 'midpoint' and g['constraintId'] == midpoint['id'] and g['pointIds'] == midpoint['pointIds'] for g in glyphs)
            assert any(g['kind'] == 'tangent' and g['constraintId'] == tangent['id'] and g['entityIds'] == tangent['entityIds'] for g in glyphs)
        # Mixed line/circle region extraction is intentionally not asserted as supported.
        call('sketch.finish')
        committed = call('document.inspect')
        path = output / 'tangent-circles.alloy'
        call('document.save', {'path': str(path)})
        call('document.new', {'discard': False})
        call('document.open', {'path': str(path)})
        assert call('document.inspect')['authored'] == committed['authored']
        final = call('app.state')
    result = {
        'result': 'passed', 'testSha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'fixtureSha256': hashlib.sha256(Path(args.fixture).read_bytes()).hexdigest(),
        'verifiedCapabilities': ['sketch.persistent_circle_line_tangent'], 'covers': [],
        'initialState': initial, 'finalState': final, 'transcript': transcript, 'captures': captures,
        'evidenceDimensions': {'api': {'status': 'passed'}, 'geometry': {'status': 'passed'},
                               'save_reopen': {'status': 'passed'},
                               'visual': {'status': 'captured', 'reason': 'Actual midpoint/tangent cues and retained glyphs; independent review remains separate.'},
                               'physical_input': {'status': 'not_run', 'reason': 'API setup and pointer command dispatch; no native mouse evidence.'}}
    }
    (output / 'sketch-tangent-api.json').write_text(json.dumps(result, indent=2) + '\n')
    print('PASS: two inferred tangent circles, retained midpoint/radius edits, history and storage; profiles remain separate')


if __name__ == '__main__':
    main()
