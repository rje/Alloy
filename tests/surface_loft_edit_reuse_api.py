#!/usr/bin/env python3
"""Bounded before/after Loft edit opening on the accepted saved iron bridge."""
import argparse
import json
from pathlib import Path

from app_harness import application

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--app', required=True)
p.add_argument('--source', required=True, type=Path)
p.add_argument('--output', required=True, type=Path)
p.add_argument('--baseline', action='store_true')
a = p.parse_args()
out = a.output.resolve()

with application(a.app, out, command_timeout=120) as (call, ledger):
    try:
        call('window.resize', {'width': 1500, 'height': 950})
        call('document.open', {'path': str(a.source.resolve()), 'discard': True})
        before = call('document.inspect')
        feature = before['authored']['features'][-1]
        assert feature['type'] == 'loft' and feature['parameters']['guideType'] == 'none'
        original = next(b for b in before['evaluation']['bodies'] if b['featureId'] == feature['id'])
        session = call('feature.editDialog', {'featureId': feature['id']})
        opening_session = session
        opening_ms = ledger[-1]['elapsedMs']
        assert session['previewed'] and session['geometryEvaluated']
        assert session['geometry']['topology'] == original['topology']
        assert abs(session['geometry']['areaMm2'] - original['areaMm2']) < 1e-6
        call('capture.dialog', {'path': str(out / 'edit-controls.png')})
        call('capture.screenshot', {'path': str(out / 'edit-preview.png')})
        if not a.baseline:
            assert session['sourcePrefixEvaluationCount'] == 1
            assert session.get('candidateEvaluationCount', 0) == 0
            assert session.get('featurePrefixEvaluationCount', 0) == 0
            assert session['previewEvaluationSource'] == 'accepted_tail'
            sid = session['id']
            repeated = call('command.preview', {'session': sid})
            assert repeated['previewEvaluationReused']
            assert repeated['sourcePrefixEvaluationCount'] == 1
            assert repeated.get('candidateEvaluationCount', 0) == 0
        call('command.cancel', {'session': session['id']})
        assert call('document.inspect')['authored'] == before['authored']
        if not a.baseline:
            # Replacement starts fresh and accepted tail reuse is still exact.
            call('component.activate', {'definitionId': feature['ownerId']})
            session = call('feature.editDialog', {'featureId': feature['id']})
            assert session['sourcePrefixEvaluationCount'] == 1
            assert session.get('candidateEvaluationCount', 0) == 0
            changed = call('command.update', {'session': session['id'], 'inputs': {'name': 'Checked retained bridge'}})
            assert changed['previewed'] and changed['sourcePrefixEvaluationCount'] == 1
            assert changed['candidateEvaluationCount'] == 1
            assert changed.get('featurePrefixEvaluationCount', 0) == 0
            assert not changed['previewEvaluationReused']
            repeated = call('command.preview', {'session': session['id']})
            assert repeated['previewEvaluationReused'] and repeated['candidateEvaluationCount'] == 1
            call('command.commit', {'session': session['id']})
            committed = call('document.inspect')
            current = next(b for b in committed['evaluation']['bodies'] if b['featureId'] == feature['id'])
            assert current['topology'] == original['topology']
            assert abs(current['areaMm2'] - original['areaMm2']) < 1e-6
            assert [f['faceId'] for f in current['supportFaces']] == [f['faceId'] for f in original['supportFaces']]
            assert [e['edgeId'] for e in current['solidEdges']] == [e['edgeId'] for e in original['solidEdges']]
            call('history.undo')
            assert call('document.inspect')['authored'] == before['authored']
            call('history.redo')
            assert call('document.inspect')['authored'] == committed['authored']
            saved = out / 'retained-bridge-edit.alloy'
            call('document.save', {'path': str(saved)})
            call('document.open', {'path': str(saved)})
            assert call('document.inspect')['authored'] == committed['authored']
            call('capture.screenshot', {'path': str(out / 'reopened-bridge.png')})
        gpu = call('app.state')['viewport']['gpu']
        (out / 'result.json').write_text(json.dumps({
            'passed': True, 'baseline': a.baseline, 'source': str(a.source.resolve()),
            'editOpeningMs': opening_ms, 'initialSession': opening_session, 'gpu': gpu,
        }, indent=2))
    finally:
        (out / 'transcript.json').write_text(json.dumps(ledger, indent=2))

print('PASS saved Loft edit opening' + (' baseline' if a.baseline else ' reuse'))
