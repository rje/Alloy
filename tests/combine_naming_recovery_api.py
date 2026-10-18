#!/usr/bin/env python3
"""SOL-071 named Combine results, transactional refusal and retained recovery."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application


def check_bodies(state, target, tool, volume, retained):
    bodies = {b['bodyId']: b for b in state['evaluation']['bodies']}
    assert set(bodies) == ({target, tool} if retained else {target}), bodies
    assert bodies[target]['name'] == 'Plate'
    assert math.isclose(bodies[target]['volumeMm3'], volume, abs_tol=1e-6)
    assert bodies[target]['valid'] and bodies[target]['closed']
    assert bodies[target]['topology']['solids'] == 1
    assert bodies[target]['visible']
    if retained:
        assert bodies[tool]['name'] == 'Cutter' and bodies[tool]['visible']
        assert math.isclose(bodies[tool]['volumeMm3'], 500, abs_tol=1e-6)
    assert state['authored']['bodyNames'] == {target: 'Plate', tool: 'Cutter'}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--app', required=True)
    p.add_argument('--output', required=True)
    a = p.parse_args()
    out = Path(a.output).resolve()
    with application(a.app, out) as (call, ledger):
        gpu = call('app.state')['viewport']['gpu']
        assert gpu['hardwareAccelerated']
        for operation, expected, edited_volume in [('join', 1250, 1250), ('cut', 750, 750), ('intersect', 250, 350)]:
            call('document.new', {'discard': True})
            call('solid.box', {'width': 20, 'depth': 10, 'height': 5, 'units': 'mm'})
            call('solid.box', {'width': 10, 'depth': 10, 'height': 5, 'x': 15, 'units': 'mm'})
            source = call('document.inspect')
            target, tool = [f['bodyId'] for f in source['authored']['features']]
            for body_id, name in [(target, 'Plate'), (tool, 'Cutter')]:
                call('body.rename', {'bodyId': body_id, 'name': name})
            named = call('document.inspect')
            inputs = {'targetBodyId': target, 'toolBodyIds': [tool], 'operation': operation, 'keepTools': False}
            session = call('command.begin', {'command': 'solid.combine', 'inputs': inputs})
            assert session['geometryEvaluated']
            assert call('document.inspect') == named
            call('command.commit', {'session': session['id']})
            result = call('document.inspect')
            check_bodies(result, target, tool, expected, False)
            call('history.undo')
            assert call('document.inspect')['authored'] == named['authored']
            call('history.redo')
            assert call('document.inspect')['authored'] == result['authored']
            combine = result['authored']['features'][-1]['id']
            edit = call('feature.editDialog', {'featureId': combine})
            assert edit['inputs']['targetBodyId'] == target and edit['inputs']['toolBodyIds'] == [tool]
            call('command.update', {'session': edit['id'], 'inputs': {'keepTools': True}})
            call('command.commit', {'session': edit['id']})
            check_bodies(call('document.inspect'), target, tool, expected, True)
            call('feature.edit', {'featureId': source['authored']['features'][0]['id'], 'parameters': {'widthMm': 22}})
            edited = call('document.inspect')
            check_bodies(edited, target, tool, edited_volume, True)
            call('history.undo')
            check_bodies(call('document.inspect'), target, tool, expected, True)
            call('history.redo')
            assert call('document.inspect')['authored'] == edited['authored']
            path = out / f'named-{operation}.alloy'
            call('document.save', {'path': str(path)})
            call('document.new', {'discard': True})
            call('document.open', {'path': str(path)})
            reopened = call('document.inspect')
            assert reopened['authored'] == edited['authored']
            check_bodies(reopened, target, tool, edited_volume, True)
            call('view.home')
            call('capture.screenshot', {'path': str(out / f'{operation}-reopened.png')})

        # Moving the retained cutter outside the intersect target is refused as
        # one candidate; accepted geometry, dirty/revision and journal survive.
        stable = call('document.inspect')
        call('feature.edit', {'featureId': source['authored']['features'][1]['id'],
                              'parameters': {'xMm': 40}}, error='evaluation_failed')
        assert call('document.inspect') == stable
        # Same unsupported empty result in an editable native command session.
        call('solid.box', {'width': 5, 'depth': 5, 'height': 5, 'x': 40, 'units': 'mm'})
        outside = call('document.inspect')['authored']['features'][-1]['bodyId']
        stable = call('document.inspect')
        session = call('command.begin', {'command': 'solid.combine', 'inputs': {
            'targetBodyId': target, 'toolBodyIds': [outside], 'operation': 'intersect', 'keepTools': False}})
        assert not session['geometryEvaluated']
        assert session['previewError']['diagnostics']['code'] == 'boolean_empty_result'
        assert 'change Operation' in session['previewError']['message']
        call('command.commit', {'session': session['id']}, error='evaluation_failed')
        assert call('document.inspect') == stable
        call('capture.screenshot', {'path': str(out / 'empty-recovery.png')})
        call('capture.dialog', {'path': str(out / 'empty-recovery-panel.png')})
        call('command.update', {'session': session['id'], 'inputs': {'operation': 'cut'}})
        call('command.commit', {'session': session['id']})
        retry = call('document.inspect')
        assert retry['bodies'] == 2
        assert retry['authored']['bodyNames'] == stable['authored']['bodyNames']
        call('history.undo')
        assert call('document.inspect')['authored'] == stable['authored']
        call('history.redo')
        assert call('document.inspect')['authored'] == retry['authored']
        (out / 'receipt.json').write_text(json.dumps({'result': 'passed', 'gpu': gpu, 'ledger': ledger}, indent=2) + '\n')
    print('PASS named Join/Cut/Intersect, source/edit/history/reopen and atomic empty-result retry')


if __name__ == '__main__':
    main()
