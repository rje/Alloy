#!/usr/bin/env python3
"""Bounded real-app tutorial constraint checkpoint; not a complete tutorial pass."""
import argparse
import hashlib
import json
import math
from pathlib import Path
from app_harness import application


def near(actual, expected):
    assert math.isclose(actual, expected, rel_tol=1e-8, abs_tol=1e-6), (actual, expected)


def check_glyphs(state, draft):
    """Inspect renderer-authored relation badges separately from solved geometry."""
    glyphs = state['viewport']['gpu']['sketchConstraintGlyphs']
    relations = draft['parameters']['constraints']
    expected = {(r['id'], entity): r for r in relations for entity in r.get('entityIds', [])}
    actual = {(g['constraintId'], g['anchorEntityId']): g for g in glyphs}
    assert actual.keys() == expected.keys(), (actual, expected)
    assert len(actual) == len(glyphs), 'Duplicate badge for a retained target'
    for key, relation in expected.items():
        glyph = actual[key]
        assert glyph['kind'] == relation['type']
        assert glyph['entityIds'] == relation['entityIds']
        assert glyph['viewportLocal'] and glyph['units'] == 'logicalPixels'
        assert glyph['bounds']['width'] > 0 and glyph['bounds']['height'] > 0
        assert all(math.isfinite(glyph['bounds'][field]) for field in ('x', 'y', 'width', 'height'))
    return glyphs


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--fixture', default=str(Path(__file__).parent / 'workflows/fixtures/constraint-tools.json'))
    args = parser.parse_args()
    fixture = json.loads(Path(args.fixture).read_text())
    assert fixture['case'] == 'constraint-tools'
    output = Path(args.output).resolve()
    captures, glyph_evidence = [], []
    with application(args.app, output) as (call, transcript):
        initial = call('app.state')
        assert initial['viewport']['gpu']['hardwareAccelerated']
        before = call('document.inspect')['authored']
        circles_session = call('sketch.begin', {'plane': 'XY', 'name': 'Equal circle checkpoint'})

        def preview(session):
            return call('command.preview', {'session': session['id']})

        def draft(session):
            return preview(session)['authoredDraft']

        def active():
            return call('app.state')['activeSession']

        for index in range(5):
            call('sketch.addCircle', {'centerX': (index - 2) * 20, 'centerY': 0,
                                     'diameter': 8 + index, 'units': 'mm'})
        original = draft(circles_session)
        circle_ids = [entity['id'] for entity in original['parameters']['entities']]
        sketch_id = original['id']
        call('sketch.selectionSet', {'entityIds': [circle_ids[0]]})
        call('sketch.constraintTool', {'type': 'perpendicular'}, 'invalid_constraint_target')
        failed = active()
        assert failed['tool'] == 'constraint' and failed['constraintType'] == 'perpendicular'
        assert draft(circles_session) == original, 'Incompatible target changed authored geometry'
        call('sketch.selectionSet', {'entityIds': []})
        call('sketch.tool', {'tool': 'select'})
        # Activate using compatible preselection, then reuse the same active tool.
        call('sketch.selectionSet', {'entityIds': circle_ids[:2]})
        call('sketch.constraintTool', {'type': 'equal'})
        assert len(draft(circles_session)['parameters']['constraints']) == 1
        for index in range(2, 5):
            call('sketch.selectionSet', {'entityIds': [circle_ids[index - 1]]})
            assert len(draft(circles_session)['parameters']['constraints']) == index - 1
            response = call('sketch.selectionSet', {'entityIds': circle_ids[index - 1:index + 1]})
            assert response['constraintApplied'] and response['constraintType'] == 'equal'
            session = active()
            assert session['tool'] == 'constraint' and session['constraintType'] == 'equal'
            assert not session['selection']['entityIds'], 'Completed pair must reset selection'
        equal_draft = draft(circles_session)
        pairs = [r['entityIds'] for r in equal_draft['parameters']['constraints']]
        assert pairs == [circle_ids[index:index + 2] for index in range(4)]
        assert preview(circles_session)['sketchGeometry']['solver']['dof'] == 11
        # Shared Select command is the Escape outcome; physical key input is separate.
        call('sketch.tool', {'tool': 'select'})
        assert active()['tool'] == 'select' and 'constraintType' not in active()
        call('sketch.dimension', {'kind': 'diameter', 'entityId': circle_ids[0],
                                  'value': 10, 'units': 'mm', 'annotation': {'uMm': -48, 'vMm': 10}})
        dimension_id = draft(circles_session)['parameters']['dimensions'][-1]['id']
        call('sketch.dimensionUpdate', {'dimensionId': dimension_id, 'value': 12, 'units': 'mm'})
        circle_draft = draft(circles_session)
        assert circle_draft['parameters']['constraints'] == equal_draft['parameters']['constraints']
        assert [e['id'] for e in circle_draft['parameters']['entities']] == circle_ids
        for entity in circle_draft['parameters']['entities']:
            near(entity['radiusMm'], 6)
        assert preview(circles_session)['sketchGeometry']['solver']['dof'] == 10
        call('sketch.dimension', {'kind': 'diameter', 'entityId': circle_ids[-1], 'value': 10}, 'sketch_solve_failed')
        assert draft(circles_session) == circle_draft, 'Failed solve replaced usable draft'
        glyph_evidence.append(check_glyphs(call('app.state'), circle_draft))
        captures.append(call('capture.screenshot', {'path': str(output / 'equal-circles.png')}))
        assert call('document.inspect')['authored'] == before, 'Draft was prematurely committed'
        call('sketch.finish')
        circle_document = call('document.inspect')['authored']
        call('history.undo')
        assert call('document.inspect')['authored'] == before
        call('history.redo')
        assert call('document.inspect')['authored'] == circle_document

        line_session = call('sketch.begin', {'plane': 'XY', 'name': 'Line relation checkpoint'})
        for coords in ((-30, 15, -10, 15.2), (0, 15, 0.2, 35), (20, 15, 21, 35)):
            call('sketch.line', dict(zip(('x1', 'y1', 'x2', 'y2'), coords)))
        lines = draft(line_session)
        line_ids = [entity['id'] for entity in lines['parameters']['entities']]
        call('sketch.selectionSet', {'entityIds': [line_ids[0]]})
        call('sketch.constraintTool', {'type': 'horizontalVertical'})
        call('sketch.selectionSet', {'entityIds': [line_ids[1]]})
        call('sketch.constraintTool', {'type': 'perpendicular'})
        applied = call('sketch.selectionSet', {'entityIds': [line_ids[0], line_ids[2]]})
        assert applied['constraintApplied'] and applied['constraintType'] == 'perpendicular'
        lines = draft(line_session)
        assert [r['type'] for r in lines['parameters']['constraints']] == ['horizontal', 'vertical', 'perpendicular']
        points = {p['id']: p for p in lines['parameters']['points']}
        directions = [(points[e['endPointId']]['uMm'] - points[e['startPointId']]['uMm'],
                       points[e['endPointId']]['vMm'] - points[e['startPointId']]['vMm'])
                      for e in lines['parameters']['entities']]
        near(directions[0][1], 0)
        near(directions[1][0], 0)
        a, b = directions[0], directions[2]
        near((a[0] * b[0] + a[1] * b[1]) / (math.hypot(*a) * math.hypot(*b)), 0)
        glyph_evidence.append(check_glyphs(call('app.state'), lines))
        captures.append(call('capture.screenshot', {'path': str(output / 'line-relations.png')}))
        retained = lines['parameters']['constraints']
        call('sketch.tool', {'tool': 'select'})
        assert draft(line_session)['parameters']['constraints'] == retained
        call('sketch.finish')
        completed = call('document.inspect')['authored']
        call('history.undo')
        assert call('document.inspect')['authored'] == circle_document
        call('history.redo')
        assert call('document.inspect')['authored'] == completed

        path = output / 'constraint-tools.alloy'
        call('document.save', {'path': str(path)})
        call('document.new', {'discard': False})
        call('document.open', {'path': str(path)})
        assert call('document.inspect')['authored'] == completed
        edited = call('sketch.edit', {'sketchId': sketch_id})
        reopened = draft(edited)
        assert reopened['parameters']['constraints'] == circle_draft['parameters']['constraints']
        glyph_evidence.append(check_glyphs(call('app.state'), reopened))
        call('sketch.dimensionUpdate', {'dimensionId': dimension_id, 'value': 14, 'units': 'mm'})
        for entity in draft(edited)['parameters']['entities']:
            near(entity['radiusMm'], 7)
        call('sketch.finish')
        changed = call('document.inspect')['authored']
        call('history.undo')
        assert call('document.inspect')['authored'] == completed
        call('history.redo')
        assert call('document.inspect')['authored'] == changed
        call('document.save', {'path': str(path)})
        call('document.new', {'discard': False})
        call('document.open', {'path': str(path)})
        assert call('document.inspect')['authored'] == changed
        final = call('app.state')
    result = {
        'result': 'passed', 'scope': 'Bounded tutorial constraint checkpoint; not a complete source tutorial pass',
        'fixtureSha256': hashlib.sha256(Path(args.fixture).read_bytes()).hexdigest(),
        'testSha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'verifiedCapabilities': ['sketch.constraint_tool_repeat', 'sketch.constraint_failure_recovery',
                                 'sketch.equal_circle_chain', 'sketch.constraint_glyph_targets',
                                 'sketch.constraint_history_storage'],
        'initialState': initial, 'finalState': final, 'transcript': transcript,
        'captures': captures, 'constraintGlyphs': glyph_evidence,
        'evidenceDimensions': {'api': {'status': 'passed'}, 'geometry': {'status': 'passed'},
                               'history_save_reopen': {'status': 'passed'},
                               'visual': {'status': 'captured', 'reason': 'Badge IDs and targets inspected; appearance needs independent capture review.'},
                               'native_input': {'status': 'not_run', 'reason': 'API selection and Select command do not establish physical picking or Escape input.'}}
    }
    (output / 'constraint-tools-api.json').write_text(json.dumps(result, indent=2) + '\n')
    print('PASS: bounded repeated constraint tools, circle diameter propagation, retained glyph targets, history and storage')


if __name__ == '__main__':
    main()
