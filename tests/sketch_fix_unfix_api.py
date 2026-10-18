#!/usr/bin/env python3
"""Bounded real-entrypoint Fix/UnFix, retained driver edits and exact reopen."""
import argparse
import json
from pathlib import Path
from app_harness import application

p = argparse.ArgumentParser()
p.add_argument('--app', required=True)
p.add_argument('--output', required=True)
a = p.parse_args()
out = Path(a.output).resolve()
with application(a.app, out) as (call, ledger):
    def session(): return call('app.state')['activeSession']
    def draft(): return session()['draftSketch']
    def last(): return draft()['parameters']['entities'][-1]
    def role(entity): return next(r['role'] for r in session()['solverDiagnostics']['entityRoles'] if r['entityId'] == entity['id'])
    call('sketch.begin', {'plane': 'XY', 'units': 'mm', 'name': 'Fix UnFix driver edits'})
    call('sketch.line', {'x1': -35, 'y1': -10, 'x2': 5, 'y2': -10, 'units': 'mm'})
    line = last()
    call('sketch.constraint', {'type': 'horizontal', 'entityIds': [line['id']]})
    call('sketch.dimension', {'kind': 'linear', 'entityId': line['id'], 'value': 40, 'units': 'mm'})
    dimension_id = draft()['parameters']['dimensions'][-1]['id']
    call('sketch.addPoint', {'x': 30, 'y': 15, 'units': 'mm'})
    point = last()
    call('sketch.addCircle', {'centerX': 30, 'centerY': -15, 'diameter': 12, 'units': 'mm'})
    circle = last()
    call('sketch.tool', {'tool': 'select'})
    call('sketch.selectionSet')
    baseline = draft()
    call('sketch.selectionSet', {'entityIds': [line['id']], 'pointIds': [point['pointId']]})
    call('sketch.constraintTool', {'type': 'fixed'}, error='invalid_constraint_target')
    assert draft() == baseline
    call('sketch.tool', {'tool': 'select'})
    call('sketch.selectionSet')
    call('sketch.fixToggle', {'entityIds': [line['id']]})
    line_fixed = draft()
    relation = line_fixed['parameters']['constraints'][-1]
    assert relation['type'] == 'fixedLine'
    assert relation['entityIds'] == [line['id']]
    assert relation['pointIds'] == [line['startPointId'], line['endPointId']]
    assert role(line) == 'fixed'
    call('history.undo'); assert draft() == baseline
    call('history.redo'); assert draft() == line_fixed
    # The same command path used by legacy fixed-point API callers now toggles.
    call('sketch.constraint', {'type': 'fixed', 'pointIds': [point['pointId']]})
    fixed_both = draft()
    point_relation = fixed_both['parameters']['constraints'][-1]
    assert set(point_relation) == {'id', 'type', 'pointIds', 'uMm', 'vMm'}
    assert role(point) == 'fixed'
    call('sketch.dimensionUpdate', {'dimensionId': dimension_id, 'value': 45, 'units': 'mm'}, error='sketch_solve_failed')
    assert draft() == fixed_both
    for args in ({'entityIds': [circle['id']]}, {'pointIds': ['00000000-0000-0000-0000-000000000000']}, {'entityIds': [line['id']], 'pointIds': [point['pointId']]}):
        before = session()['draftHistory']
        call('sketch.fixToggle', args, error='sketch_solve_failed')
        assert draft() == fixed_both and session()['draftHistory'] == before
    # Shared explicit entity form on sketch.constraint unfixes the same relation.
    call('sketch.constraint', {'type': 'fixed', 'entityIds': [line['id']]})
    point_only = draft()
    assert relation not in point_only['parameters']['constraints'] and role(line) != 'fixed'
    call('history.undo'); assert draft() == fixed_both
    call('history.redo'); assert draft() == point_only
    call('sketch.dimensionUpdate', {'dimensionId': dimension_id, 'value': 45, 'units': 'mm'})
    updated = draft()
    points = {p['id']: p for p in updated['parameters']['points']}
    assert abs(points[line['endPointId']]['uMm'] - points[line['startPointId']]['uMm'] - 45) < 1e-7
    assert point_relation in updated['parameters']['constraints']
    call('sketch.fixToggle', {'entityIds': [line['id']]})
    final = draft()
    call('sketch.lookAt')
    call('capture.screenshot', {'path': str(out / 'fixed-driver-edit.png')})
    fixed_state = call('app.state')
    assert not {'fixed', 'fixedLine'} & {g['kind'] for g in fixed_state['viewport']['gpu']['sketchConstraintGlyphs']}
    sketch_id = final['id']
    call('sketch.finish')
    authored = call('document.inspect')['authored']
    call('document.save', {'path': str(out / 'fix-unfix-driver.alloy')})
    call('document.open', {'path': str(out / 'fix-unfix-driver.alloy')})
    assert call('document.inspect')['authored'] == authored
    call('sketch.edit', {'sketchId': sketch_id})
    assert draft() == final and role(line) == role(point) == 'fixed'
    call('capture.screenshot', {'path': str(out / 'reopened.png')})
    reopened_state = call('app.state')
    call('sketch.cancel')
    (out / 'receipt.json').write_text(json.dumps({'result': 'passed', 'fixedState': fixed_state, 'reopenedState': reopened_state, 'transcript': ledger}, indent=2) + '\n')
print('PASS: shared point/line toggle, retained driver lock/recovery, atomic refusal, exact history/reopen and GPU captures')
