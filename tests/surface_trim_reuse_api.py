#!/usr/bin/env python3
"""Measure region-only Trim reuse on an existing retained iron model."""
import argparse
import json
from pathlib import Path

from app_harness import application

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--app', required=True)
p.add_argument('--source', required=True, type=Path,
               help='Accepted retained exterior with a final paired-sketch Trim.')
p.add_argument('--output', required=True, type=Path)
p.add_argument('--max-pick-ms', type=float, default=1000)
a = p.parse_args()
out = a.output.resolve()

with application(a.app, out, command_timeout=120) as (call, ledger):
    try:
        call('window.resize', {'width': 1500, 'height': 950})
        call('document.open', {'path': str(a.source.resolve()), 'discard': True})
        before = call('document.inspect')
        feature = next(f for f in reversed(before['authored']['features']) if f['type'] == 'surfaceTrim')
        body_id = feature['parameters']['targetBodyId']

        def body(state):
            return next(b for b in state['evaluation']['bodies'] if b['bodyId'] == body_id)

        original = body(before)
        assert original['valid'] and original['topology']['faces'] == 2
        session = call('feature.editDialog', {'featureId': feature['id']})
        sid = session['id']
        assert session['partitionBuildCount'] == 1 and not session['partitionReused']
        inside = [r['regionId'] for r in session['regions'] if r['inside']]
        assert len(inside) == 2 and set(inside) == set(feature['parameters']['removedRegionIds'])
        timings = []

        def state():
            return call('app.state')['activeSession']

        def reused(command, args, *, error=None):
            result = call(command, args, error=error)
            elapsed = ledger[-1]['elapsedMs']
            current = state()
            assert current['partitionBuildCount'] == 1 and current['partitionReused']
            timings.append({'command': command, 'elapsedMs': elapsed})
            assert elapsed < a.max_pick_ms, (command, elapsed, a.max_pick_ms)
            return result

        # Deselect both, then restore both through the authoritative command.
        for region in inside:
            reused('surface.trimRegion', {'regionId': region})
        assert not state()['previewed']
        reused('command.preview', {'session': sid}, error='trim_region_required')
        for region in inside:
            reused('surface.trimRegion', {'regionId': region})
        reused('command.preview', {'session': sid})
        reused('command.update', {'session': sid, 'inputs': {'name': 'Responsive handle Trim'}})
        assert state()['geometry']['result']['valid']
        assert state()['geometry']['result']['topology']['faces'] == 2
        call('capture.dialog', {'path': str(out / 'trim-controls.png')})
        call('capture.screenshot', {'path': str(out / 'trim-red-preview.png')})
        call('command.cancel', {'session': sid})
        assert call('document.inspect')['authored'] == before['authored']

        # A new session starts fresh; a geometry input change rebuilds once.
        session = call('feature.editDialog', {'featureId': feature['id']})
        sid = session['id']
        assert session['partitionBuildCount'] == 1 and not session['partitionReused']
        margin = feature['parameters']['throughMarginMm'] + 1
        session = call('command.update', {'session': sid, 'inputs': {'throughMarginMm': margin}})
        assert session['partitionBuildCount'] == 2 and not session['partitionReused']
        session = call('command.preview', {'session': sid})
        assert session['partitionBuildCount'] == 2 and session['partitionReused']
        call('command.commit', {'session': sid})
        committed = call('document.inspect')
        current = body(committed)
        assert current['valid'] and current['topology'] == original['topology']
        assert abs(current['areaMm2'] - original['areaMm2']) < 1e-5
        assert [f['faceId'] for f in current['supportFaces']] == [f['faceId'] for f in original['supportFaces']]
        assert [e['edgeId'] for e in current['solidEdges']] == [e['edgeId'] for e in original['solidEdges']]
        call('history.undo')
        assert call('document.inspect')['authored'] == before['authored']
        call('history.redo')
        assert call('document.inspect')['authored'] == committed['authored']
        saved = out / 'responsive-trim.alloy'
        call('document.save', {'path': str(saved)})
        call('document.open', {'path': str(saved)})
        reopened = call('document.inspect')
        assert reopened['authored'] == committed['authored']
        assert body(reopened)['topology'] == current['topology']
        call('view.home')
        call('capture.screenshot', {'path': str(out / 'reopened-holes.png')})
        (out / 'result.json').write_text(json.dumps({
            'passed': True, 'source': str(a.source.resolve()), 'regionActions': timings,
            'maxPickMs': a.max_pick_ms, 'committed': current,
            'gpu': call('app.state')['viewport']['gpu'],
        }, indent=2))
    finally:
        (out / 'transcript.json').write_text(json.dumps(ledger, indent=2))

print('PASS session-local Trim partition reuse')
