#!/usr/bin/env python3
"""Inspect the same overlapping curve candidates through the real Alloy entrypoint."""
import argparse, hashlib, json
from pathlib import Path
from app_harness import application
p = argparse.ArgumentParser()
p.add_argument('--app', required=True)
p.add_argument('--fixture', required=True)
p.add_argument('--output', required=True)
a = p.parse_args()
out = Path(a.output).resolve()
with application(a.app, out) as (call, ledger):
    call('document.open', {'path': str(Path(a.fixture).resolve())})
    baseline = call('document.inspect')
    sketch = next(f for f in baseline['authored']['features'] if f['type'] == 'sketch')
    call('sketch.edit', {'sketchId': sketch['id']})
    call('sketch.lookAt')
    call('view.fit')
    state = call('app.state')['activeSession']
    context = state['projectionContext']
    original = state['draftSketch']
    line, circle = context['lines'][0], context['circles'][0]
    targets = [('line', (line['u1Mm'] + line['u2Mm']) / 2, (line['v1Mm'] + line['v2Mm']) / 2),
               ('circle', circle['uMm'], circle['vMm'] - circle['radiusMm'])]
    rows = []
    for kind, u, v in targets:
        pixel = call('view.projectSketchPoint', {'sketchId': sketch['id'], 'uMm': u, 'vMm': v})
        candidates = call('sketch.selectionCandidates', {'x': round(pixel['x']), 'y': round(pixel['y'])})['candidates']
        assert len(candidates) == 2, candidates
        assert {r['projected'] for r in candidates} == {False, True}
        assert all(r['type'] == kind for r in candidates)
        for target in candidates:
            before = call('app.state')['activeSession']
            assert before['draftSketch'] == original
            call('sketch.selectionSet', {'entityIds': [target['entityId']]})
            selected = call('app.state')['activeSession']
            assert selected['selection']['entityIds'] == [target['entityId']]
            assert selected['draftHistory'] == before['draftHistory']
            call('capture.screenshot', {'path': str(out / f"{kind}-{'projected' if target['projected'] else 'authored'}-selected.png")})
            call('sketch.deleteSelection')
            deleted = call('app.state')['activeSession']
            assert deleted['draftHistory']['index'] == before['draftHistory']['index'] + 1
            assert deleted['draftSketch'] != original
            call('history.undo')
            restored = call('app.state')['activeSession']
            assert restored['draftSketch'] == original
            assert restored['projectionContext'] == context
            call('history.redo')
            assert call('app.state')['activeSession']['draftSketch'] == deleted['draftSketch']
            call('history.undo')
        rows.append({'kind': kind, 'candidates': candidates})
    call('sketch.cancel')
    assert call('document.inspect') == baseline
    gpu = call('app.state')['viewport']['gpu']
    assert gpu['hardwareAccelerated']
    (out / 'receipt.json').write_text(json.dumps({'passed': True, 'candidatePairs': rows, 'gpu': gpu,
        'unchangedCommittedDocument': True, 'exactUndoRedo': True,
        'appSha256': hashlib.sha256(Path(a.app).read_bytes()).hexdigest()}, indent=2) + '\n')
    (out / 'transcript.json').write_text(json.dumps(ledger, indent=2) + '\n')
print('PASS: actual-app overlapping line/circle candidates, deliberate shared selection/Delete, exact undo/redo and GPU')
