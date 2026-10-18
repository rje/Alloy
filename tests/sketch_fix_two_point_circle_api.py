#!/usr/bin/env python3
"""Real-entrypoint two-point circle Fix: authored source, diameter and evaluated geometry."""
import argparse
import json
import math
import time
from pathlib import Path
from app_harness import application

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--app', required=True)
p.add_argument('--output', required=True)
a = p.parse_args()
out = Path(a.output).resolve()
with application(a.app, out) as (call, ledger):
    def session(): return call('app.state')['activeSession']
    def draft(): return session()['draftSketch']
    def point(d, identity): return next(p for p in d['parameters']['points'] if p['id'] == identity)
    def check_geometry(d, x, y, radius):
        e = next(e for e in d['parameters']['entities'] if e['id'] == circle['id'])
        center = point(d, circle['centerPointId'])
        start = point(d, circle['diameterStartPointId'])
        assert math.isclose(start['uMm'], x, abs_tol=1e-7) and math.isclose(start['vMm'], y, abs_tol=1e-7)
        assert math.isclose(center['uMm'], x + .6*radius, abs_tol=1e-7)
        assert math.isclose(center['vMm'], y + .8*radius, abs_tol=1e-7)
        assert math.isclose(e['radiusMm'], radius, abs_tol=1e-7)
        assert e['diameterDirectionU'] == .6 and e['diameterDirectionV'] == .8
        assert math.isclose(2*center['uMm']-start['uMm'], x+1.2*radius, abs_tol=1e-7)
        assert math.isclose(2*center['vMm']-start['vMm'], y+1.6*radius, abs_tol=1e-7)

    call('sketch.begin', {'plane': 'XY', 'units': 'mm', 'name': 'Two-Point Circle Fix'})
    call('sketch.addTwoPointCircle', {'startX': -26, 'startY': 2, 'endX': -14, 'endY': 18, 'diameter': 20, 'units': 'mm'})
    circle = draft()['parameters']['entities'][-1]
    dimension = draft()['parameters']['dimensions'][-1]
    call('sketch.addCircle', {'centerX': 20, 'centerY': -10, 'diameter': 14, 'units': 'mm'})
    other = draft()['parameters']['entities'][-1]
    call('sketch.fixToggle', {'entityIds': [other['id']]})
    other_relation = draft()['parameters']['constraints'][-1]
    call('sketch.tool', {'tool': 'select'})
    call('sketch.selectionSet')
    baseline = draft()
    call('sketch.constraint', {'type': 'fixed', 'entityIds': [circle['id']]})
    fixed = draft()
    relation = fixed['parameters']['constraints'][-1]
    assert relation['type'] == 'fixedCircle' and relation['pointIds'] == [circle['centerPointId'], circle['diameterStartPointId']]
    assert relation['startUMm'] == -26 and relation['startVMm'] == 2
    assert relation['diameterDirectionU'] == .6 and relation['diameterDirectionV'] == .8
    check_geometry(fixed, -26, 2, 10)
    history = session()['draftHistory']
    call('sketch.dimensionUpdate', {'dimensionId': dimension['id'], 'value': 24, 'units': 'mm'}, error='sketch_solve_failed')
    assert draft() == fixed and session()['draftHistory'] == history
    for identity in [circle['centerPointId'], circle['diameterStartPointId']]:
        history = session()['draftHistory']
        call('sketch.movePoint', {'pointId': identity, 'x': 50, 'y': 40, 'units': 'mm'}, error='sketch_solve_failed')
        assert draft() == fixed and session()['draftHistory'] == history
        check_geometry(draft(), -26, 2, 10)
        assert draft()['parameters']['constraints'] == fixed['parameters']['constraints']
    fixed = draft()
    for args in [{'entityIds': ['00000000-0000-0000-0000-000000000000']}, {'entityIds': [circle['id']], 'pointIds': [circle['diameterStartPointId']]}]:
        history = session()['draftHistory']
        call('sketch.fixToggle', args, error='sketch_solve_failed')
        assert draft() == fixed and session()['draftHistory'] == history
    call('sketch.fixToggle', {'entityIds': [circle['id']]})
    unfixed = draft()
    assert unfixed['parameters']['constraints'] == [other_relation]
    call('history.undo'); assert draft() == fixed
    call('history.redo'); assert draft() == unfixed
    call('sketch.dimensionUpdate', {'dimensionId': dimension['id'], 'value': 24, 'units': 'mm'})
    check_geometry(draft(), -26, 2, 12)
    call('sketch.movePoint', {'pointId': circle['diameterStartPointId'], 'x': -30, 'y': 0, 'units': 'mm'})
    check_geometry(draft(), -30, 0, 12)
    assert draft()['parameters']['dimensions'][0]['id'] == dimension['id']
    call('sketch.fixToggle', {'entityIds': [circle['id']]})
    final = draft()
    final_relation = final['parameters']['constraints'][-1]
    call('sketch.deleteSelection', {'constraintIds': [final_relation['id']]})
    assert draft()['parameters']['constraints'] == [other_relation]
    call('history.undo'); assert draft() == final
    call('sketch.lookAt'); time.sleep(.25)
    call('capture.screenshot', {'path': str(out/'fixed.png')})
    state = call('app.state')
    assert not {'fixed', 'fixedLine', 'fixedCircle'} & {g['kind'] for g in state['viewport']['gpu']['sketchConstraintGlyphs']}
    assert next(r['role'] for r in state['activeSession']['solverDiagnostics']['entityRoles'] if r['entityId'] == circle['id']) == 'fixed'
    call('sketch.finish')
    authored = call('document.inspect')['authored']
    call('document.save', {'path': str(out/'fixed-two-point-circle.alloy')})
    call('document.open', {'path': str(out/'fixed-two-point-circle.alloy')})
    assert call('document.inspect')['authored'] == authored
    call('sketch.edit', {'sketchId': final['id']}); assert draft() == final
    check_geometry(draft(), -30, 0, 12)
    time.sleep(.25); call('capture.screenshot', {'path': str(out/'reopened.png')})
    reopened = call('app.state'); call('sketch.cancel')
    (out/'receipt.json').write_text(json.dumps({'result': 'passed', 'fixedState': state, 'reopenedState': reopened, 'transcript': ledger}, indent=2)+'\n')
print('PASS: two-point Fix authored source/direction/endpoints, diameter conflict/recovery, stable-ID removal, history/reopen and GPU capture')
