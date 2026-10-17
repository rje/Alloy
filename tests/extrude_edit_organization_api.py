#!/usr/bin/env python3
"""Edit an early Extrude while preserving organization of a later body."""
import argparse
import copy
import json
from pathlib import Path
from app_harness import application

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--app', required=True)
parser.add_argument('--output', required=True, type=Path)
args = parser.parse_args()
out = args.output.resolve()
with application(args.app, out) as (call, transcript):
    try:
        for name, x in [('Early', 0), ('Later', 30)]:
            call('sketch.begin', {'plane': 'XY', 'name': name})
            call('sketch.centerRectangle', {'centerX': x, 'centerY': 0,
                 'width': 20, 'height': 10, 'units': 'mm'})
            call('sketch.finish')
            sketch = call('document.inspect')['evaluation']['sketches'][-1]
            call('solid.extrude', {'sketchId': sketch['sketchId'],
                 'profileId': sketch['profiles'][0]['profileId'], 'distance': 5, 'units': 'mm'})
        initial = call('document.inspect')
        early = next(f for f in initial['authored']['features'] if f['type'] == 'extrude')
        later_id = initial['evaluation']['bodies'][-1]['bodyId']
        owner = call('component.create', {'name': 'Finished part', 'activate': False})['definitionId']
        call('body.rename', {'bodyId': later_id, 'name': 'Later finished body'})
        call('component.reparent', {'destinationId': owner, 'bodyId': later_id})
        before = call('document.inspect')
        authored = before['authored']
        assert authored['bodyNames'][later_id] == 'Later finished body'
        assert authored['objectOwners']['bodies'][later_id] == owner
        expected = copy.deepcopy(authored)
        next(f for f in expected['features'] if f['id'] == early['id'])['parameters']['distanceMm'] = 8

        def preview_edit():
            session = call('feature.editDialog', {'featureId': early['id']})
            call('command.update', {'session': session['id'], 'inputs': {'distance': 8}})
            preview = call('command.preview', {'session': session['id']})
            assert preview['previewed'] and preview['geometryEvaluated']
            assert preview['documentDraft'] == expected
            assert call('document.inspect')['authored'] == authored
            return session

        session = preview_edit()
        call('command.cancel', {'session': session['id']})
        assert call('document.inspect')['authored'] == authored
        session = preview_edit()
        call('command.commit', {'session': session['id']})
        after = call('document.inspect')
        assert after['authored'] == expected
        assert [b['bodyId'] for b in after['evaluation']['bodies']] == [b['bodyId'] for b in before['evaluation']['bodies']]
        later = next(b for b in after['evaluation']['bodies'] if b['bodyId'] == later_id)
        assert later == next(b for b in before['evaluation']['bodies'] if b['bodyId'] == later_id)
        changed = next(b for b in after['evaluation']['bodies'] if b['featureId'] == early['id'])
        assert abs(changed['volumeMm3'] - 1600) < 1e-5
        call('history.undo')
        assert call('document.inspect')['authored'] == authored
        call('history.redo')
        assert call('document.inspect')['authored'] == expected
        saved = out / 'organized-extrude-edit.alloy'
        call('document.save', {'path': str(saved)})
        call('document.open', {'path': str(saved), 'discard': True})
        assert call('document.inspect')['authored'] == expected
        call('view.fit')
        call('capture.screenshot', {'path': str(out / 'organized-extrude-edit.png')})
    finally:
        (out / 'transcript.json').write_text(json.dumps(transcript, indent=2) + '\n')
print('PASS early Extrude preview/Cancel/commit, later body identity and organization, undo/redo/reopen')
