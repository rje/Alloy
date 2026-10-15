#!/usr/bin/env python3
"""Bounded real-app paired-ridge history and GPU inspection on the accepted source."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application

parser = argparse.ArgumentParser()
parser.add_argument('--app', required=True)
parser.add_argument('--source', required=True)
parser.add_argument('--output', required=True)
args = parser.parse_args()
out = Path(args.output).resolve()
with application(args.app, out, command_timeout=180) as (call, transcript):
    try:
        call('document.open', {'path': str(Path(args.source).resolve())})
        call('window.resize', {'width': 1400, 'height': 1000})
        initial = call('document.inspect')
        fillet = initial['authored']['features'][-1]
        assert fillet['type'] == 'fillet'
        target = fillet['parameters']['targetBodyId']
        seeds = fillet['parameters']['edgeIds']
        assert len(seeds) == 2 and fillet['parameters']['radiusMm'] == 0.5

        def verify(state):
            current = next(b for b in state['evaluation']['bodies'] if b['bodyId'] == target)
            assert current['valid'] and current['closed'] and current['topology']['solids'] == 1
            assert current['construction'] == 'pairedFullRoundCommon'
            assert current['selectedEdgeIds'] == seeds
            assert len(current['tangentContours']) == 2
            assert [len(c) for c in current['tangentContours']] == [7, 7]
            assert state['authored']['features'][-1] == fillet
            assert not any(e['provenance'].get('resolutionError', '').startswith('ambiguous')
                           for e in current['solidEdges'])
            return current

        baseline = verify(initial)
        stock = next(f for f in initial['authored']['features'] if f['name'] == 'Centered case stock')
        width = next(d for d in stock['parameters']['dimensions']
                     if d['kind'] == 'horizontal' and d['valueMm'] == 64)
        call('sketch.edit', {'sketchId': stock['id']})
        call('sketch.dimensionUpdate', {'dimensionId': width['id'], 'value': 65, 'units': 'mm'})
        call('sketch.finish')
        edited = call('document.inspect')
        widened = verify(edited)
        assert abs(widened['volumeMm3'] - baseline['volumeMm3']) > 1
        call('history.undo')
        restored = call('document.inspect')
        assert restored['authored'] == initial['authored']
        assert math.isclose(verify(restored)['volumeMm3'], baseline['volumeMm3'], abs_tol=1e-6)
        call('history.redo')
        assert call('document.inspect')['authored'] == edited['authored']
        path = out / 'paired-ridge-width65.alloy'
        call('document.save', {'path': str(path)})
        call('document.open', {'path': str(path)})
        reopened = call('document.inspect')
        assert reopened['authored'] == edited['authored']
        assert math.isclose(verify(reopened)['volumeMm3'], widened['volumeMm3'], abs_tol=1e-6)
        # Hide the separately produced positive half for unobscured rim captures.
        split = next(f for f in reopened['authored']['features'] if f['type'] == 'splitBody')
        call('feature.visibility', {'featureId': split['id'], 'visible': False})
        call('view.grid', {'visible': False})
        call('view.origin', {'visible': False})
        call('view.home')
        call('capture.screenshot', {'path': str(out / 'paired-ridge-isometric.png')})
        call('view.orientation', {'name': 'top'})
        call('capture.screenshot', {'path': str(out / 'paired-ridge-top.png')})
        call('view.orientation', {'name': 'rearIsometric'})
        call('capture.screenshot', {'path': str(out / 'paired-ridge-mouth.png')})
        app_state = call('app.state')
        (out / 'report.json').write_text(json.dumps({'passed': True, 'before': initial,
            'after': edited, 'reopened': reopened, 'appState': app_state,
            'scope': 'paired-ridge width edit/history/reopen and GPU captures; retained-tool Cut/export pending'}, indent=2))
    finally:
        (out / 'transcript.json').write_text(json.dumps(transcript, indent=2))
