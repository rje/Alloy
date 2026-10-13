"""Saved wall-outlet edit/history checks, reusable without replaying creation."""
import argparse
import json
from pathlib import Path
from app_harness import application
from wall_outlet_oracle import near, verify


def verify_history(call, model):
    authored = call('document.inspect')['authored']
    assert len(authored['features']) == 1
    sketch = authored['features'][0]
    call('sketch.edit', {'sketchId': sketch['id']})
    initial = call('app.state')['activeSession']
    before = initial['draftSketch']
    verify(before, initial['solverDiagnostics'])
    dimensions = before['parameters']['dimensions']
    target = next(d for d in dimensions
                  if d['kind'] == 'parallelLineDistance' and d['valueMm'] == 11)
    call('sketch.dimensionUpdate', {'dimensionId': target['id'], 'value': 12, 'units': 'mm'})
    state = call('app.state')['activeSession']
    edited = state['draftSketch']
    assert edited != before and state['solverDiagnostics']['dof'] == 0
    assert edited['parameters']['mirrorGroups'] == before['parameters']['mirrorGroups']
    points = {p['id']: p for p in edited['parameters']['points']}
    entities = {e['id']: e for e in edited['parameters']['entities']}
    for pair in edited['parameters']['mirrorGroups'][0]['entityPairs']:
        source, reflected = entities[pair['sourceEntityId']], entities[pair['resultEntityId']]
        if source['type'] == 'arc':
            near(points[source['centerPointId']]['vMm'], 26)
            near(points[reflected['centerPointId']]['vMm'], -26)
            near(source['radiusMm'], 17)
            near(reflected['radiusMm'], 17)
    call('history.undo')
    # Edit entry reevaluates curve coordinates. Compare against that actual
    # history baseline, not a pre-Finish solver snapshot from an earlier session.
    assert call('app.state')['activeSession']['draftSketch'] == before
    call('history.redo')
    assert call('app.state')['activeSession']['draftSketch'] == edited
    call('sketch.cancel')
    call('document.open', {'path': str(model), 'discard': True})
    assert call('document.inspect')['authored'] == authored
    return {'result': 'passed', 'dimensionId': target['id'],
            'edit': '11 to 12 mm; mirrored arc centers move to +26 and -26 mm',
            'undoRedo': 'exact edit-entry and edited drafts', 'reopen': 'exact authored document'}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--model', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args()
    report, transcript = {'result': 'incomplete'}, []
    try:
        with application(args.app, args.output) as (call, transcript):
            call('document.open', {'path': str(args.model.resolve()), 'discard': True})
            report.update(verify_history(call, args.model.resolve()))
    finally:
        report['transcript'] = transcript
        (args.output / 'history-report.json').write_text(json.dumps(report, indent=2)+'\n')
    print('PASS: saved wall-outlet edit/history/reopen')
