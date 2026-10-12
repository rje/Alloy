#!/usr/bin/env python3
"""Bounded Activity 1 shape/constraint API checkpoint, not full tutorial or native acceptance."""
import argparse
import hashlib
import json
import math
from pathlib import Path
from app_harness import application


def near(actual, expected):
    assert math.isclose(actual, expected, rel_tol=1e-8, abs_tol=1e-5), (actual, expected)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--fixture', default=str(Path(__file__).parent / 'workflows/fixtures/bracket-activity1.json'))
    args = parser.parse_args()
    fixture = json.loads(Path(args.fixture).read_text())
    assert fixture['case'] == 'bracket-activity1'
    output = Path(args.output).resolve()
    captures, stages = [], []
    with application(args.app, output) as (call, transcript):
        call('window.resize', {'width': 1600, 'height': 900})
        initial = call('app.state')
        assert initial['viewport']['gpu']['hardwareAccelerated']
        session = call('sketch.begin', {'plane': 'XY', 'name': 'Activity 1 shape checkpoint'})
        call('view.zoom', {'factor': 0.1})

        def preview():
            return call('command.preview', {'session': session['id']})

        def params():
            return preview()['authoredDraft']['parameters']

        def project(u, v):
            point = call('view.projectSketchPoint', {'plane': 'XY', 'uMm': u, 'vMm': v})
            return {'x': point['x'], 'y': point['y']}

        origin = project(0, 0)
        viewport = call('app.state')['viewport']
        call('view.pan', {'x': 100 - origin['x'], 'y': viewport['height'] - 100 - origin['y']})
        call('sketch.tool', {'tool': 'line'})
        for u, v in fixture['seedVerticesMm']:
            call('sketch.placePoint', project(u, v))
        call('sketch.tool', {'tool': 'select'})  # Shared Escape outcome; native key input is separate.
        outline = preview()
        assert len(outline['authoredDraft']['parameters']['entities']) == 6
        assert outline['sketchGeometry']['profiles']
        edges = outline['authoredDraft']['parameters']['entities']
        edge_ids = [edge['id'] for edge in edges]

        def points_and_vectors():
            p = params()
            points = {point['id']: point for point in p['points']}
            vectors = [(points[edge['endPointId']]['uMm'] - points[edge['startPointId']]['uMm'],
                        points[edge['endPointId']]['vMm'] - points[edge['startPointId']]['vMm'])
                       for edge in p['entities']]
            return points, vectors

        def capture(stage):
            info = preview()
            p = info['authoredDraft']['parameters']
            captures.append(call('capture.screenshot', {'path': str(output / (stage + '.png'))}))
            state = call('app.state')
            relations = [r for r in p['constraints'] if r['type'] in ('perpendicular', 'horizontal', 'vertical', 'equal')]
            glyphs = state['viewport']['gpu']['sketchConstraintGlyphs']
            expected = {(r['id'], target) for r in relations for target in r['entityIds']}
            actual = {(g['constraintId'], g['anchorEntityId']) for g in glyphs}
            assert actual == expected and len(glyphs) == len(expected), {'stage': stage, 'expected': sorted(expected), 'actual': sorted(actual), 'glyphCount': len(glyphs)}
            by_id = {r['id']: r for r in relations}
            for glyph in glyphs:
                relation = by_id[glyph['constraintId']]
                assert glyph['kind'] == relation['type'] and glyph['entityIds'] == relation['entityIds']
                assert glyph['anchorEntityId'] in relation['entityIds']
                assert glyph['bounds']['width'] > 0 and glyph['bounds']['height'] > 0
            stages.append({'stage': stage, 'draft': info['authoredDraft'], 'geometry': info['sketchGeometry'], 'glyphs': glyphs})

        capture('01-outline')
        # Activate each constraint before selecting targets; repeated pair selection
        # must retain the active tool and reset completed target selection.
        def pairs(kind, indices):
            call('sketch.constraintTool', {'type': kind})
            for a, b in indices:
                call('sketch.selectionSet', {'entityIds': [edge_ids[a]]})
                applied = call('sketch.selectionSet', {'entityIds': [edge_ids[a], edge_ids[b]]})
                assert applied['constraintApplied'] and applied['constraintType'] == kind
                active = call('app.state')['activeSession']
                assert active['tool'] == 'constraint' and active['constraintType'] == kind
                assert not active['selection']['entityIds']

        pairs('perpendicular', fixture['perpendicularPairs'])
        _, vectors = points_and_vectors()
        for a, b in fixture['perpendicularPairs']:
            av, bv = vectors[a], vectors[b]
            near((av[0] * bv[0] + av[1] * bv[1]) / (math.hypot(*av) * math.hypot(*bv)), 0)
        capture('02-perpendicular')
        call('sketch.constraintTool', {'type': 'horizontalVertical'})
        for index in fixture['horizontalTargets']:
            applied = call('sketch.selectionSet', {'entityIds': [edge_ids[index]]})
            assert applied['constraintApplied']
        _, vectors = points_and_vectors()
        for index in fixture['horizontalTargets']:
            near(vectors[index][1], 0)
        capture('03-horizontal')
        pairs('equal', fixture['equalPairs'])
        _, vectors = points_and_vectors()
        for a, b in fixture['equalPairs']:
            near(math.hypot(*vectors[a]), math.hypot(*vectors[b]))
        capture('04-equal')
        call('sketch.tool', {'tool': 'select'})
        real_relations = [r for r in params()['constraints'] if r['type'] in ('perpendicular', 'horizontal', 'equal')]
        assert [(r['type'], r['entityIds']) for r in real_relations] == (
            [('perpendicular', [edge_ids[a], edge_ids[b]]) for a, b in fixture['perpendicularPairs']]
            + [('horizontal', [edge_ids[i]]) for i in fixture['horizontalTargets']]
            + [('equal', [edge_ids[a], edge_ids[b]]) for a, b in fixture['equalPairs']])

        def midpoint_pixel(index):
            points, _ = points_and_vectors()
            a, b = points[edges[index]['startPointId']], points[edges[index]['endPointId']]
            return project((a['uMm'] + b['uMm']) / 2, (a['vMm'] + b['vMm']) / 2)

        def place_dimension(targets, placement, value, expected_kind):
            call('sketch.tool', {'tool': 'dimension'})
            call('sketch.selectionSet', {'entityIds': targets})
            pending = call('sketch.pointerMove', placement)
            assert pending['kind'] == expected_kind, pending
            call('sketch.dimensionPlace', placement)
            if expected_kind == 'angle':
                diagnostic = {'pending': pending, 'placement': placement, 'valueDeg': value,
                              'stateBeforeAccept': call('app.state'), 'previewBeforeAccept': preview()}
                diagnostic['capture'] = call('capture.screenshot', {'path': str(output / 'angle-before-accept.png')})
                (output / 'angle-accept-diagnostic.json').write_text(json.dumps(diagnostic, indent=2) + '\n')
            try:
                call('sketch.dimensionAccept', {'valueDeg' if expected_kind == 'angle' else 'value': value})
            except AssertionError:
                if expected_kind == 'angle':
                    diagnostic['failedResponse'] = transcript[-1]['response']
                    diagnostic['stateAfterFailure'] = call('app.state')
                    (output / 'angle-accept-diagnostic.json').write_text(json.dumps(diagnostic, indent=2) + '\n')
                raise
            authored = params()['dimensions'][-1]
            assert authored['kind'] == expected_kind
            return authored['id']

        lower = midpoint_pixel(0)
        lower['y'] += 35
        lower_id = place_dimension([edge_ids[0]], lower, fixture['lowerLengthMm'], 'horizontal')
        points, _ = points_and_vectors()
        joint = points[edges[0]['endPointId']]
        angle_id = place_dimension([edge_ids[0], edge_ids[1]], project(joint['uMm'] + 150, joint['vMm'] + 50), fixture['angleDeg'], 'angle')
        points, _ = points_and_vectors()
        a, b = points[edges[2]['startPointId']], points[edges[2]['endPointId']]
        ap, bp = project(a['uMm'], a['vMm']), project(b['uMm'], b['vMm'])
        dx, dy = bp['x'] - ap['x'], bp['y'] - ap['y']
        length = math.hypot(dx, dy)
        # Offset normal to the cap by 35 logical pixels: auto placement must
        # choose aligned length, with no forced kind/axis or modifier.
        aligned = {'x': (ap['x'] + bp['x']) / 2 - dy * 35 / length,
                   'y': (ap['y'] + bp['y']) / 2 + dx * 35 / length}
        cap_id = place_dimension([edge_ids[2]], aligned, fixture['capLengthMm'], 'linear')
        call('sketch.tool', {'tool': 'select'})
        final_preview = preview()
        assert final_preview['sketchGeometry']['solver']['dof'] == 0
        points, vectors = points_and_vectors()
        for index in (0, 1):
            near(math.hypot(*vectors[index]), fixture['lowerLengthMm'])
        for index in (2, 5):
            near(math.hypot(*vectors[index]), fixture['capLengthMm'])
        near(abs(math.atan2(vectors[0][0] * vectors[1][1] - vectors[0][1] * vectors[1][0],
                            vectors[0][0] * vectors[1][0] + vectors[0][1] * vectors[1][1])) * 180 / math.pi, fixture['angleDeg'])
        origin_id = params()['originPointId']
        near(points[origin_id]['uMm'], 0)
        near(points[origin_id]['vMm'], 0)
        assert [d['id'] for d in params()['dimensions']] == [lower_id, angle_id, cap_id]
        base, cap, theta = fixture['lowerLengthMm'], fixture['capLengthMm'], math.radians(fixture['angleDeg'])
        v2 = (base + base * math.cos(theta), base * math.sin(theta))
        v3 = (v2[0] - cap * math.sin(theta), v2[1] + cap * math.cos(theta))
        expected_vertices = [(0, 0), (base, 0), v2, v3, (base - cap * math.tan(theta / 2), cap), (0, cap)]
        expected_area = abs(sum(a[0] * b[1] - a[1] * b[0] for a, b in zip(expected_vertices, expected_vertices[1:] + expected_vertices[:1]))) / 2
        near(final_preview['sketchGeometry']['profiles'][0]['areaMm2'], expected_area)
        capture('05-activity1-endpoint')
        final_draft = final_preview['authoredDraft']
        # Alloy-added persistence checks, not additional source tutorial steps.
        call('sketch.finish')
        completed = call('document.inspect')['authored']
        call('history.undo')
        assert not call('document.inspect')['authored']['features']
        call('history.redo')
        assert call('document.inspect')['authored'] == completed
        path = output / 'bracket-activity1.alloy'
        call('document.save', {'path': str(path)})
        call('document.new', {'discard': False})
        call('document.open', {'path': str(path)})
        assert call('document.inspect')['authored'] == completed
        final = call('app.state')
    (output / 'bracket-activity1-api.json').write_text(json.dumps({
        'result': 'passed', 'scope': 'Bounded six-edge Activity 1 modeling checkpoint; not full TUT-003 or native input acceptance',
        'fixtureSha256': hashlib.sha256(Path(args.fixture).read_bytes()).hexdigest(),
        'testSha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'verifiedCapabilities': ['tutorial.activity1_shape_constraints_dimensions'], 'covers': [],
        'initialState': initial, 'finalState': final, 'transcript': transcript,
        'stages': stages, 'captures': captures, 'finalDraft': final_draft,
        'evidenceDimensions': {'api': {'status': 'passed'}, 'hardware': {'status': 'passed'},
                               'geometry': {'status': 'passed'}, 'save_reopen': {'status': 'passed'},
                               'visual': {'status': 'captured', 'reason': 'Constraint-stage and endpoint captures need independent reference review.'},
                               'physical_input': {'status': 'not_run', 'reason': 'API pointers/selection/placement do not establish native controls; new document tab is unavailable.'}}
    }, indent=2) + '\n')
    print('PASS: bounded Activity 1 six-edge shape, real relations, auto dimensions and persistence')


if __name__ == '__main__':
    main()
