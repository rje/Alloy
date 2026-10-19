#!/usr/bin/env python3
"""Reopen the approved enclosure variant through the real Alloy application."""
import argparse
import json
from pathlib import Path
from app_harness import application

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--app', required=True)
p.add_argument('--source', type=Path, required=True)
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
out = a.output.resolve()
out.mkdir(parents=True, exist_ok=True)
source = a.source.resolve()
saved = json.loads(source.read_text())
with application(a.app, out, command_timeout=180) as (call, transcript):
    try:
        call('document.open', {'path': str(source), 'discard': True})
        state = call('document.inspect')
        assert state['authored'] == saved
        features = saved['features']
        assert len(features) == 33
        port_sketch = next(f for f in features if f['type'] == 'sketch'
                           and f['name'] == 'Three linked port clearances')
        port = next(f for f in features if f['type'] == 'extrude'
                    and f['parameters'].get('sketchId') == port_sketch['id']
                    and f['parameters'].get('operation') == 'cut')
        assert port['parameters']['endOffsetMm'] == -1.5
        rim = next(f for f in features if f['type'] == 'fillet' and f['name'] == 'Paired rim round')
        assert rim['parameters']['radiusMm'] == .4
        split = next(f for f in features if f['type'] == 'splitBody')
        halves = []
        for identity in (split['parameters']['targetBodyId'], split['bodyId']):
            body = next(b for b in state['evaluation']['bodies'] if b['bodyId'] == identity)
            assert body['valid'] and body['closed'] and body['topology']['solids'] == 1
            halves.append({'bodyId': identity, 'volumeMm3': body['volumeMm3']})
        board = next(f for f in features if f['type'] == 'stepAssemblyImport')
        call('feature.visibility', {'featureId': board['id'], 'visible': not board['visible']})
        assert call('document.inspect')['authored'] != saved
        call('history.undo')
        assert call('document.inspect')['authored'] == saved
        call('history.redo')
        assert call('document.inspect')['authored'] != saved
        call('history.undo')
        assert call('document.inspect')['authored'] == saved
        call('view.orientation', {'name': 'rearIsometric'})
        call('view.fit')
        call('capture.screenshot', {'path': str(out / 'final-reopened.png')})
        (out / 'result.json').write_text(json.dumps({
            'source': str(source), 'featureCount': len(features), 'halfBodies': halves,
            'authoredExactOnOpen': True, 'visibilityUndoRedoRestore': True,
            'rimRadiusMm': .4, 'portEndOffsetMm': -1.5,
        }, indent=2) + '\n')
    finally:
        (out / 'transcript.json').write_text(json.dumps(transcript, indent=2) + '\n')
print('PASS final enclosure reopen, half validity and undo/redo restoration')
