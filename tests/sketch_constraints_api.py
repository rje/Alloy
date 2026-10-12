#!/usr/bin/env python3
"""Real-app constrained rectangle, driving/driven dimensions and journal workflow."""
import argparse
import hashlib
import json
import math
from pathlib import Path
from app_harness import application


def near(value, expected):
    assert math.isclose(value, expected, rel_tol=1e-8, abs_tol=1e-6), (value, expected)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('app', 'output', 'fixture'):
        parser.add_argument('--' + name, required=True)
    args = parser.parse_args()
    output = Path(args.output).resolve()
    fixture = json.loads(Path(args.fixture).read_text())
    captures = []
    with application(args.app, output) as (call, transcript):
        initial = call('app.state')
        assert initial['viewport']['gpu']['hardwareAccelerated']
        before = call('document.inspect')
        session = call('sketch.begin', {'plane': 'XY', 'name': 'Dimensioned rectangle'})
        call('sketch.rectangle', {'x1': 0, 'y1': 0, 'x2': fixture['widthMm'], 'y2': fixture['heightMm'], 'units': 'mm'})
        draft = call('command.preview', {'session': session['id']})['authoredDraft']
        points = [p['id'] for p in draft['parameters']['points']]
        entities = [e['id'] for e in draft['parameters']['entities']]
        assert [c['type'] for c in draft['parameters']['constraints']] == ['horizontal', 'vertical', 'horizontal', 'vertical']
        call('sketch.constraint', {'type': 'fixed', 'pointIds': [points[0]]})
        preview = call('command.preview', {'session': session['id']})
        assert preview['sketchGeometry']['solverIntegrated']
        assert preview['sketchGeometry']['solver']['dof'] == 2
        # An explicit stale line must not silently use unrelated selected points.
        call('sketch.selectionSet', {'pointIds': [points[0], points[2]], 'entityIds': []})
        usable = preview['authoredDraft']
        call('sketch.dimension', {'kind': 'horizontal',
             'entityId': '00000000-0000-4000-8000-000000000001',
             'value': fixture['widthMm'], 'units': 'mm'}, 'command_failed')
        assert call('command.preview', {'session': session['id']})['authoredDraft'] == usable
        # Agent selection uses the same selected line as native dimension entry.
        call('sketch.selectionSet', {'pointIds': [], 'entityIds': [entities[0]]})
        call('sketch.dimension', {'kind': 'horizontal', 'value': fixture['widthMm'], 'units': 'mm'})
        call('sketch.dimension', {'kind': 'vertical', 'pointIds': [points[1], points[2]], 'value': fixture['heightMm'], 'units': 'mm'})
        call('sketch.dimension', {'kind': 'linear', 'pointIds': [points[0], points[2]], 'driving': False})
        preview = call('command.preview', {'session': session['id']})
        dimensions = preview['authoredDraft']['parameters']['dimensions']
        width_id = next(d['id'] for d in dimensions if d['kind'] == 'horizontal')
        driven_id = next(d['id'] for d in dimensions if not d['driving'])
        assert preview['sketchGeometry']['solver']['dof'] == 0
        call('sketch.dimensionUpdate', {'dimensionId': width_id, 'value': fixture['editedWidthMm'] / 25.4, 'units': 'in'})
        call('sketch.movePoint', {'pointId': points[2], 'x': 48, 'y': 29, 'units': 'mm'})
        preview = call('command.preview', {'session': session['id']})
        solved = {p['id']: p for p in preview['authoredDraft']['parameters']['points']}
        near(solved[points[0]]['uMm'], 0)
        near(solved[points[0]]['vMm'], 0)
        near(solved[points[2]]['uMm'], fixture['editedWidthMm'])
        near(solved[points[2]]['vMm'], fixture['heightMm'])
        diagnostics = preview['sketchGeometry']['solver']
        measured = next(d['valueMm'] for d in diagnostics['dimensionValues'] if d['id'] == driven_id)
        near(measured, math.hypot(fixture['editedWidthMm'], fixture['heightMm']))
        assert diagnostics['dof'] == 0 and diagnostics['maxResidual'] <= 1e-6
        assert call('document.inspect') == before, 'Draft solves must not edit the document'
        # An incompatible second driving width must identify conflicts and keep the usable draft.
        usable = preview['authoredDraft']
        call('sketch.dimension', {'kind': 'horizontal', 'pointIds': [points[0], points[1]], 'value': 41, 'units': 'mm'}, 'sketch_solve_failed')
        failure = transcript[-1]['response']['error']['diagnostics']
        assert failure['conflictingConstraintIds'], failure
        assert call('command.preview', {'session': session['id']})['authoredDraft'] == usable
        call('sketch.dimensionUpdate', {'dimensionId': width_id, 'value': 50, 'units': 'cm'}, 'invalid_arguments')
        assert call('command.preview', {'session': session['id']})['authoredDraft'] == usable
        captures.append(call('capture.screenshot', {'path': str(output / 'dimensioned-draft.png')}))
        call('sketch.finish')
        committed = call('document.inspect')
        assert committed['sketches'] == 1 and committed['bodies'] == 0 and committed['undo']['count'] == 1
        sketch_id = committed['authored']['features'][0]['id']
        call('history.undo')
        assert call('document.inspect')['authored'] == before['authored']
        call('history.redo')
        assert call('document.inspect')['authored'] == committed['authored']
        before_edit = call('document.inspect')
        call('sketch.edit', {'sketchId': sketch_id})
        call('sketch.dimensionUpdate', {'dimensionId': width_id, 'value': 60, 'units': 'mm'})
        call('sketch.cancel')
        assert call('document.inspect') == before_edit
        saved_path = output / 'dimensioned-rectangle.alloy'
        call('document.save', {'path': str(saved_path)})
        saved = call('document.inspect')
        call('document.new', {'discard': False})
        call('document.open', {'path': str(saved_path)})
        reopened = call('document.inspect')
        assert reopened['authored'] == saved['authored'] and reopened['undo']['count'] == 0
        assert reopened['evaluation']['sketches'][0]['solver']['dof'] == 0
        call('sketch.edit', {'sketchId': sketch_id})
        call('sketch.dimensionUpdate', {'dimensionId': width_id, 'value': 55, 'units': 'mm'})
        call('sketch.finish')
        assert call('document.inspect')['undo']['count'] == 1
        call('history.undo')
        assert call('document.inspect')['authored'] == saved['authored']
    result = {
        'result': 'passed', 'fixtureSha256': hashlib.sha256(Path(args.fixture).read_bytes()).hexdigest(),
        'verifiedCapabilities': ['sketch.constraints', 'sketch.solver_diagnostics', 'sketch.driving_dimensions',
                                 'sketch.driven_measurements', 'sketch.constraint_storage', 'sketch.entity_selection'],
        'initialState': initial, 'transcript': transcript, 'captures': captures,
        'evidenceDimensions': {'api': {'status': 'passed'}, 'hardware': {'status': 'passed'},
            'geometry': {'status': 'passed'}, 'save_reopen': {'status': 'passed'},
            'visual': {'status': 'captured', 'reason': 'Real GPU sketch and controls; physical interaction and reference compatibility remain separate.'}}
    }
    (output / 'sketch-constraints-api.json').write_text(json.dumps(result, indent=2) + '\n')
    print('PASS: constrained rectangle, driving/driven dimensions, conflict preservation and undo/save workflow')


if __name__ == '__main__':
    main()
