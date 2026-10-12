#!/usr/bin/env python3
"""Focused real-entrypoint sketch draft history, separate from native key evidence."""
import argparse
import hashlib
import json
import math
from pathlib import Path
from app_harness import application


def near(actual, expected):
    assert math.isclose(actual, expected, rel_tol=1e-8, abs_tol=1e-6), (actual, expected)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--fixture', default=str(Path(__file__).parent / 'workflows/fixtures/sketch-history.json'))
    args = parser.parse_args()
    output = Path(args.output).resolve()
    fixture = json.loads(Path(args.fixture).read_text())
    width, height = fixture['widthMm'], fixture['heightMm']
    first_width, second_width = fixture['firstEditedWidthMm'], fixture['secondEditedWidthMm']
    branch_width, cancel_width = fixture['branchWidthMm'], fixture['cancelWidthMm']
    captures = []
    with application(args.app, output) as (call, transcript):
        initial = call('app.state')
        call('solid.box', {'width': 2, 'depth': 3, 'height': 4, 'x': -30, 'y': -30,
                           'units': 'mm', 'name': 'Unrelated committed body'})
        baseline = call('document.inspect')
        session = call('sketch.begin', {'plane': 'XY', 'name': 'Undoable snapped outline'})

        def active():
            return call('app.state')['activeSession']

        def journal():
            return active()['draftHistory']

        def preview():
            return call('command.preview', {'session': session['id']})

        def draft():
            return preview()['authoredDraft']

        def pixels(u, v):
            point = call('view.projectSketchPoint', {'plane': 'XY', 'uMm': u, 'vMm': v})
            return {'x': point['x'], 'y': point['y']}

        empty = draft()
        assert journal() == {'index': 0, 'count': 1, 'canUndo': False, 'canRedo': False}
        call('sketch.tool', {'tool': 'line'})
        call('sketch.pointerMove', pixels(0, 0))
        assert journal()['count'] == 1 and draft() == empty
        call('sketch.placePoint', pixels(0, 0))
        assert journal()['count'] == 1, 'First point is pending input, not an accepted entity'
        call('history.redo', error='pending_input_active')
        assert active()['firstPoint'], 'Rejected Redo discarded pending point'
        cancelled = call('history.undo')
        assert cancelled['action'] == 'cancelled_tool'
        assert active()['tool'] == 'select' and draft() == empty
        assert journal()['count'] == 1

        call('sketch.tool', {'tool': 'line'})
        call('sketch.placePoint', pixels(0, 0))
        for u, v in ((width, 0), (width, height), (0, height)):
            call('sketch.placePoint', pixels(u, v))
        open_outline = draft()
        assert journal()['count'] == 4
        call('sketch.pointerMove', pixels(0, 0))
        assert journal()['count'] == 4 and draft() == open_outline
        call('sketch.placePoint', pixels(0, 0))
        closed = draft()
        assert journal()['index'] == 4 and journal()['count'] == 5
        assert len(closed['parameters']['entities']) == 4
        assert closed['parameters']['originPointId']
        assert preview()['sketchGeometry']['profiles'], 'Snapped boundary must actually close'
        cancelled = call('history.undo')
        assert cancelled['action'] == 'cancelled_tool' and draft() == closed
        assert journal()['index'] == 4, 'Exiting Line traversed accepted history prematurely'
        undone = call('history.undo')
        assert undone['changed'] and draft() == open_outline
        assert not preview()['sketchGeometry']['profiles']
        assert call('document.inspect') == baseline, 'Draft Undo changed unrelated document history'
        redone = call('history.redo')
        assert redone['changed'] and draft() == closed
        assert preview()['sketchGeometry']['profiles']

        entities = closed['parameters']['entities']
        for index, entity in enumerate(entities):
            call('sketch.constraint', {'type': 'horizontal' if index % 2 == 0 else 'vertical',
                                       'entityIds': [entity['id']]})
        constrained = draft()
        accepted_constraints = journal()
        call('history.undo')
        assert len(draft()['parameters']['constraints']) == len(constrained['parameters']['constraints']) - 1
        call('history.redo')
        assert draft() == constrained and journal() == accepted_constraints
        call('sketch.dimension', {'kind': 'horizontal', 'entityId': entities[0]['id'], 'value': width, 'units': 'mm'})
        width_id = draft()['parameters']['dimensions'][-1]['id']
        call('sketch.dimension', {'kind': 'vertical', 'entityId': entities[1]['id'], 'value': height, 'units': 'mm'})
        assert preview()['sketchGeometry']['solver']['dof'] == 0
        call('sketch.dimensionUpdate', {'dimensionId': width_id, 'value': first_width, 'units': 'mm'})
        width30 = draft()
        history30 = journal()
        call('sketch.dimension', {'kind': 'horizontal', 'entityId': entities[0]['id'], 'value': first_width + 5}, 'sketch_solve_failed')
        assert draft() == width30 and journal() == history30, 'Rejected solve entered draft history'
        call('sketch.dimensionUpdate', {'dimensionId': width_id, 'value': second_width, 'units': 'mm'})
        width40 = draft()
        history40 = journal()
        call('sketch.tool', {'tool': 'dimension'})
        call('sketch.dimensionEdit', {'dimensionId': width_id})
        call('history.redo', error='pending_input_active')
        assert active()['pendingDimension']['phase'] == 'editing'
        cancelled = call('history.undo')
        assert cancelled['action'] == 'cancelled_input'
        assert active()['tool'] == 'dimension' and 'pendingDimension' not in active()
        assert draft() == width40 and journal() == history40
        cancelled = call('history.undo')
        assert cancelled['action'] == 'cancelled_tool'
        assert active()['tool'] == 'select' and draft() == width40
        call('history.undo')
        assert draft() == width30 and journal()['canRedo']
        call('history.redo')
        assert draft() == width40, 'Redo changed stable authored IDs or geometry'
        call('history.undo')
        assert draft() == width30
        call('sketch.dimensionUpdate', {'dimensionId': width_id, 'value': branch_width, 'units': 'mm'})
        width35 = draft()
        branch = journal()
        assert not branch['canRedo'] and branch['count'] == history40['count']
        assert not call('history.redo')['changed'] and draft() == width35
        near(preview()['sketchGeometry']['profiles'][0]['areaMm2'], branch_width * height)
        assert call('document.inspect') == baseline
        captures.append(call('capture.screenshot', {'path': str(output / 'draft-history-restored-profile.png')}))
        call('sketch.finish')
        finished = call('document.inspect')
        assert finished['undo']['count'] == baseline['undo']['count'] + 1, 'Finish must remain one document edit'
        sketch_id = width35['id']
        call('history.undo')
        assert call('document.inspect')['authored'] == baseline['authored']
        call('history.redo')
        committed = call('document.inspect')
        assert committed['authored'] == finished['authored']

        edited = call('sketch.edit', {'sketchId': sketch_id})
        assert journal()['count'] == 1 and journal()['index'] == 0
        call('sketch.dimensionUpdate', {'dimensionId': width_id, 'value': cancel_width, 'units': 'mm'})
        assert call('command.preview', {'session': edited['id']})['authoredDraft'] != width35
        call('sketch.cancel')
        assert call('document.inspect') == committed, 'Cancel changed original authored work or its document journal'
        path = output / 'sketch-history.alloy'
        call('document.save', {'path': str(path)})
        call('document.new', {'discard': False})
        call('document.open', {'path': str(path)})
        reopened = call('document.inspect')
        assert reopened['authored'] == committed['authored']
        saved_sketch = next(f for f in reopened['authored']['features'] if f['id'] == sketch_id)
        assert saved_sketch == width35
        near(reopened['evaluation']['sketches'][0]['profiles'][0]['areaMm2'], branch_width * height)
        final = call('app.state')
    result = {
        'result': 'passed', 'testSha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'fixtureSha256': hashlib.sha256(Path(args.fixture).read_bytes()).hexdigest(),
        'verifiedCapabilities': ['sketch.draft_history', 'sketch.undo_tool_precedence'],
        'covers': [],
        'initialState': initial, 'finalState': final, 'transcript': transcript, 'captures': captures,
        'evidenceDimensions': {'api': {'status': 'passed'}, 'geometry': {'status': 'passed'},
                               'save_reopen': {'status': 'passed'},
                               'visual': {'status': 'captured', 'reason': 'Real GPU profile capture; independent appearance review remains separate.'},
                               'physical_input': {'status': 'not_run', 'reason': 'API dispatch verifies pending precedence; it does not establish native shortcut input.'}}
    }
    (output / 'sketch-history-api.json').write_text(json.dumps(result, indent=2) + '\n')
    print('PASS: active sketch history, pending Undo precedence, branch truncation, Finish/Cancel and storage')


if __name__ == '__main__':
    main()
