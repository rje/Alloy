#!/usr/bin/env python3
"""Retained Top-minus-Bottom rim fit through the real application authority."""
import argparse
import json
import math
from pathlib import Path
import subprocess
from app_harness import application
from stl_export_api import inspect_stl
from step_exchange_api import independent

p = argparse.ArgumentParser()
for name in ('app', 'source', 'output', 'step-tool', 'fit-tool'):
    p.add_argument('--' + name, required=True)
a = p.parse_args()
out = Path(a.output).resolve()
with application(a.app, out, command_timeout=180) as (call, ledger):
    try:
        call('document.open', {'path': str(Path(a.source).resolve())})
        call('window.resize', {'width': 1400, 'height': 1000})
        before = call('document.inspect')
        split = next(f for f in before['authored']['features'] if f['type'] == 'splitBody')
        bottom_id, top_id = split['parameters']['targetBodyId'], split['bodyId']
        owner = split['ownerId']
        def body(state, identity):
            b = next(b for b in state['evaluation']['bodies'] if b['bodyId'] == identity)
            assert b['valid'] and b['closed'] and b['topology']['solids'] == 1
            assert b['ownerId'] == owner
            # A Cut may genuinely split an old edge. Keep those old references
            # unavailable rather than demanding that topology never changes.
            assert all(not e['available'] for e in b['solidEdges']
                       if e['provenance'].get('resolutionError', '').startswith('ambiguous'))
            return b
        call('component.activate', {'definitionId': owner})
        call('solid.combine', {'targetBodyId': top_id, 'toolBodyIds': [bottom_id],
            'operation': 'cut', 'keepTools': True, 'name': 'Ridge mating groove'})
        cut = call('document.inspect')
        feature = cut['authored']['features'][-1]
        assert feature['type'] == 'boolean'
        assert feature['parameters']['targetBodyId'] == top_id
        assert feature['parameters']['toolBodyIds'] == [bottom_id]
        assert feature['parameters']['keepTools']
        assert body(cut, bottom_id)['volumeMm3'] == body(before, bottom_id)['volumeMm3']
        assert body(cut, top_id)['volumeMm3'] < body(before, top_id)['volumeMm3']
        call('history.undo')
        assert call('document.inspect')['authored'] == before['authored']
        call('history.redo')
        assert call('document.inspect')['authored'] == cut['authored']
        stock = next(f for f in cut['authored']['features'] if f['name'] == 'Centered case stock')
        width = next(d for d in stock['parameters']['dimensions']
                     if d['kind'] == 'horizontal' and d['valueMm'] == 64)
        call('sketch.edit', {'sketchId': stock['id']})
        call('sketch.dimensionUpdate', {'dimensionId': width['id'], 'value': 65, 'units': 'mm'})
        call('sketch.finish')
        edited = call('document.inspect')
        assert edited['authored']['features'][-1] == feature
        for identity in (bottom_id, top_id):
            assert abs(body(edited, identity)['volumeMm3'] - body(cut, identity)['volumeMm3']) > 1
        call('history.undo')
        assert call('document.inspect')['authored'] == cut['authored']
        call('history.redo')
        assert call('document.inspect')['authored'] == edited['authored']
        model = out / 'enclosure-ridge-fit-width65.alloy'
        call('document.save', {'path': str(model)})
        call('document.open', {'path': str(model)})
        reopened = call('document.inspect')
        assert reopened['authored'] == edited['authored']
        original_supports = {f['faceId'] for f in body(before, top_id)['supportFaces']}
        retained_support = next(f for f in body(reopened, top_id)['supportFaces']
                                if f['faceId'] in original_supports)
        call('sketch.begin', {'support': {k: retained_support[k]
            for k in ('producerFeatureId', 'bodyId', 'faceId')}, 'name': 'Retained support check'})
        call('sketch.cancel')
        assert call('document.inspect')['authored'] == edited['authored']
        exports = []
        for name, identity in (('Bottom', bottom_id), ('Top', top_id)):
            current = body(reopened, identity)
            assert math.isclose(current['volumeMm3'], body(edited, identity)['volumeMm3'], abs_tol=1e-6)
            stl, step = out / (name.lower()+'.stl'), out / (name.lower()+'.step')
            stl_result = call('document.exportStl', {'path': str(stl), 'bodyId': identity,
                'units': 'mm', 'format': 'binary', 'surfaceDeviationMm': .1, 'normalDeviationDeg': 10})
            mesh = inspect_stl(stl, True, allow_cavities=True)
            assert len(mesh['components']) == 1
            step_result = call('document.exportStep', {'path': str(step), 'bodyId': identity, 'units': 'mm'})
            read = independent(Path(a.step_tool).resolve(), 'read', step)
            assert read['valid'] and read['solidCount'] == 1
            assert math.isclose(read['volumeMm3'], current['volumeMm3'], abs_tol=1e-4)
            exports.append({'name': name, 'stl': stl_result, 'meshReadback': mesh,
                            'step': step_result, 'stepReadback': read})
        # Bottom's producer visibility is independent of the positive split half.
        call('feature.visibility', {'featureId': body(reopened, bottom_id)['producerFeatureId'], 'visible': False})
        call('view.orientation', {'name': 'bottom'})
        call('capture.screenshot', {'path': str(out / 'top-complementary-groove.png')})
        (out / 'result.json').write_text(json.dumps({'passed': True, 'before': before,
            'cut': cut, 'edited': edited, 'reopened': reopened, 'exports': exports,
            'gpu': call('app.state')['viewport']['gpu']['details']}, indent=2))
    finally:
        (out / 'transcript.json').write_text(json.dumps(ledger, indent=2))
completed = subprocess.run([str(Path(a.fit_tool).resolve()), str(model), str(out/'fit.json'), 'fit'],
                           capture_output=True, text=True)
(out / 'fit.log').write_text(completed.stdout + completed.stderr)
assert completed.returncode == 0, completed.stderr
fit = json.loads((out/'fit.json').read_text())
assert not fit['intersectionHasSolids'] and abs(fit['intersectionVolumeMm3']) < 1e-7
