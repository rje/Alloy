#!/usr/bin/env python3
"""SKT-128: real-entrypoint inventory of the retained native mixed sketch."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--app', required=True)
p.add_argument('--fixture', required=True)
p.add_argument('--output', required=True)
a = p.parse_args()
out = Path(a.output).resolve()
with application(a.app, out) as (call, ledger):
    call('document.open', {'path': str(Path(a.fixture).resolve())})
    authored = call('document.inspect')['authored']
    sketch = next(f for f in authored['features'] if f['name'] == 'Mixed relation glyph inventory')
    call('sketch.edit', {'sketchId': sketch['id']})

    def state():
        return call('app.state')

    def draft():
        return state()['activeSession']['draftSketch']

    baseline = draft()
    relations = baseline['parameters']['constraints']
    expected = [r for r in relations if r['type'] not in ('fixed', 'coincident')]
    assert {r['type'] for r in expected} == {
        'pointOnCurve', 'horizontal', 'vertical', 'parallel', 'perpendicular',
        'equal', 'tangent', 'concentric', 'midpoint', 'collinear', 'symmetry'}

    def inventory():
        s = state()
        glyphs = s['viewport']['gpu']['sketchConstraintGlyphs']
        for relation in expected:
            badges = [g for g in glyphs if g['constraintId'] == relation['id']]
            assert badges, relation
            for g in badges:
                assert g['kind'] == relation['type']
                assert g.get('pointIds', []) == relation.get('pointIds', [])
                assert g.get('entityIds', []) == relation.get('entityIds', [])
                assert g['viewportLocal'] and g['units'] == 'logicalPixels'
                assert all(math.isfinite(g['bounds'][k]) for k in ('x', 'y', 'width', 'height'))
                assert g['bounds']['width'] > 0 and g['bounds']['height'] > 0
        assert {g['constraintId'] for g in glyphs} == {r['id'] for r in expected}
        assert not s['activeSession']['solverDiagnostics']['redundantConstraintIds']
        return s

    initial = inventory()
    # Shared selection exposes the exact retained targets. Native badge picking
    # and representative Backspace/Undo are separately checked by the Qt fixture.
    for relation in expected:
        call('sketch.constraintSelect', {'constraintId': relation['id']})
        s = state()
        assert s['activeSession']['selection']['constraintIds'] == [relation['id']]
        assert all(g['selected'] for g in s['viewport']['gpu']['sketchConstraintGlyphs']
                   if g['constraintId'] == relation['id'])
        assert draft() == baseline
    fixed = next(r for r in relations if r['type'] == 'fixed')
    call('sketch.deleteSelection', {'constraintIds': [fixed['id']]})
    removed = draft()
    assert fixed not in removed['parameters']['constraints']
    call('history.undo')
    assert draft() == baseline
    call('history.redo')
    assert draft() == removed
    call('history.undo')
    assert draft() == baseline
    call('sketch.selectionSet')
    inventory()
    call('sketch.finish')
    assert call('document.inspect')['authored'] == authored
    saved = out / 'mixed-glyph-inventory-api.alloy'
    call('document.save', {'path': str(saved)})
    call('document.open', {'path': str(saved)})
    assert call('document.inspect')['authored'] == authored
    call('sketch.edit', {'sketchId': sketch['id']})
    assert draft() == baseline
    reopened = inventory()
    gpu = reopened['viewport']['gpu']
    assert gpu['hardwareAccelerated'] and gpu['renderer'], gpu
    capture = call('capture.screenshot', {'path': str(out / 'reopened-glyph-inventory.png')})
    call('sketch.cancel')
    (out / 'receipt.json').write_text(json.dumps({
        'result': 'passed', 'initialState': initial, 'reopenedState': reopened,
        'capture': capture, 'transcript': ledger,
        'limits': ['No reference pixel parity', 'Native input is separate Qt fixture evidence',
                   'Geometry Fix/UnFix and native toggle remain SKT-133/#558']}, indent=2) + '\n')
print('PASS: existing glyph identities/targets, fixed-point stable deletion, Undo/Redo, authored save/reopen and GPU capture')
