#!/usr/bin/env python3
"""Two-corner fillet preview, retained references and shared radius through the real app."""
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
    parser.add_argument('--fixture', default=str(Path(__file__).parent / 'workflows/fixtures/sketch-fillet.json'))
    args = parser.parse_args()
    fixture = json.loads(Path(args.fixture).read_text())
    output = Path(args.output).resolve()
    captures = []
    with application(args.app, output) as (call, transcript):
        initial = call('app.state')
        session = call('sketch.begin', {'plane': 'XY', 'name': 'Two-corner fillet checkpoint'})

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
        call('sketch.dimension', {'kind': 'horizontal', 'pointIds': [edges[0]['startPointId'], edges[0]['endPointId']], 'value': 500, 'units': 'mm'})
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
        before = draft()
        original_dimensions = before['parameters']['dimensions']
        removed_id = original_dimensions[0]['id']
        retained_ids = {d['id'] for d in original_dimensions[1:]}
        corners = [{'entityIds': [ids[a], ids[b]]} for a, b in fixture['filletPairs']]

        def choose_corners():
            call('sketch.tool', {'tool': 'fillet'})
            for first, second in fixture['filletPairs']:
                # API seed reuses exact coincident point IDs; picks are resolved
                # by the same viewport-space corner command used by the UI.
                a = edges[first]
                b = edges[second]
                shared = set((a['startPointId'], a['endPointId'])) & set((b['startPointId'], b['endPointId']))
                assert len(shared) == 1
                call('sketch.placePoint', project(*points()[shared.pop()]))

        choose_corners()
        history = call('app.state')['activeSession']['draftHistory']
        candidate = call('sketch.filletPreview', {'value': fixture['radiusMm'], 'units': 'mm'})
        assert candidate['authoredChange'] is False
        assert draft() == before
        assert call('app.state')['activeSession']['draftHistory'] == history
        captures.append(call('capture.screenshot', {'path': str(output / 'fillet-preview.png')}))
        call('sketch.filletCancel')
        assert draft() == before
        choose_corners()
        accepted_result = call('sketch.filletAccept', {'value': fixture['radiusMm'], 'units': 'mm'})
        assert accepted_result['removedDimensionIds'] == [removed_id], accepted_result
        accepted = draft()
        params = accepted['parameters']
        arcs = [e for e in params['entities'] if e['type'] == 'arc']
        assert len(arcs) == 2
        assert all(math.isclose(a['radiusMm'], fixture['radiusMm'], abs_tol=1e-6) for a in arcs)
        assert {e['id'] for e in params['entities'] if e['type'] == 'line'} == set(ids)
        assert not any(d['id'] == removed_id for d in params['dimensions'])
        assert retained_ids <= {d['id'] for d in params['dimensions']}
        arc_ids = {a['id'] for a in arcs}
        radius = next(d for d in params['dimensions'] if d['kind'] == 'radius' and d['entityId'] in arc_ids)
        tangents = [r for r in params['constraints'] if r['type'] == 'arcTangent']
        assert len(tangents) == 4
        by_id = {e['id']: e for e in params['entities']}
        for relation in tangents:
            arc, line_entity = (by_id[x] for x in relation['entityIds'])
            contact = relation['pointIds'][0]
            assert contact in (arc['startPointId'], arc['endPointId'])
            assert contact in (line_entity['startPointId'], line_entity['endPointId'])
        call('sketch.tool', {'tool': 'select'})
        call('history.undo')
        assert draft() == before
        call('history.redo')
        assert draft() == accepted
        call('sketch.dimensionUpdate', {'dimensionId': radius['id'], 'value': fixture['editedRadiusMm'], 'units': 'mm'})
        edited = draft()
        assert edited['parameters']['constraints'] == params['constraints']
        for arc in edited['parameters']['entities']:
            if arc['id'] in arc_ids:
                assert math.isclose(arc['radiusMm'], fixture['editedRadiusMm'], abs_tol=1e-6)
        captures.append(call('capture.screenshot', {'path': str(output / 'fillet-edited.png')}))
        state = call('app.state')
        glyphs = state['viewport']['gpu']['sketchConstraintGlyphs']
        assert {r['id'] for r in tangents} <= {g['constraintId'] for g in glyphs}
        call('sketch.finish')
        committed = call('document.inspect')['authored']
        path = output / 'filleted-bracket.alloy'
        call('document.save', {'path': str(path)})
        call('document.new', {'discard': False})
        call('document.open', {'path': str(path)})
        assert call('document.inspect')['authored'] == committed
        final = call('app.state')
    result = {
        'result': 'passed', 'fixtureSha256': hashlib.sha256(Path(args.fixture).read_bytes()).hexdigest(),
        'verifiedCapabilities': ['sketch.shared_radius_corner_fillets'], 'covers': [],
        'initialState': initial, 'finalState': final, 'transcript': transcript, 'captures': captures,
        'evidenceDimensions': {'api': {'status': 'passed'}, 'geometry': {'status': 'passed'},
            'save_reopen': {'status': 'passed'}, 'visual': {'status': 'captured'},
            'physical_input': {'status': 'not_run', 'reason': 'API pointer dispatch is not native input.'}},
        'limits': ['Mixed profile-region extraction and multi-profile extrusion remain separate.']}
    (output / 'sketch-fillet-api.json').write_text(json.dumps(result, indent=2) + '\n')
    print('PASS: two-corner fillet preview/cancel, dimension fallout, shared radius, history and storage')


if __name__ == '__main__':
    main()
