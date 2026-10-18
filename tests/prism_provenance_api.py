#!/usr/bin/env python3
"""GEO-006 retained bracket opens, source edits and exact reopen through Alloy."""
import argparse
import hashlib
import json
from pathlib import Path
from app_harness import application

p = argparse.ArgumentParser()
p.add_argument('--app', required=True)
p.add_argument('--output', required=True)
p.add_argument('--fullsize', required=True)
p.add_argument('--angle', required=True)
a = p.parse_args()
out = Path(a.output).resolve()
fixtures = {'fullsize': Path(a.fullsize).resolve(), 'angle': Path(a.angle).resolve()}

def names(state):
    return {b['bodyId']: {'faces': sorted(f['faceId'] for f in b['supportFaces']),
                         'edges': sorted(e['edgeId'] for e in b['solidEdges'])}
            for b in state['evaluation']['bodies']}

receipts = []
expected_names = {}
for process in range(2):
    case = out / f'process-{process+1}'
    with application(a.app, case, command_timeout=90) as (call, ledger):
        try:
            for stem, path in fixtures.items():
                original = json.loads(path.read_text())
                for repeat in range(4):
                    call('document.open', {'path': str(path), 'discard': True})
                    state = call('document.inspect')
                    assert state['authored'] == original
                    assert len(state['evaluation']['bodies']) == 1
                    assert state['evaluation']['bodies'][0]['valid']
                    current = names(state)
                    if stem in expected_names:
                        assert current == expected_names[stem]
                    else:
                        expected_names[stem] = current
                # A retained outline radius edit exercises seam canonicalization
                # during downstream rebuild, with no replacement of source IDs.
                source = original['features'][0]
                radius = next(d for d in source['parameters']['dimensions'] if d['kind'] == 'radius')
                call('sketch.edit', {'sketchId': source['id']})
                call('sketch.dimensionUpdate', {'dimensionId': radius['id'], 'value': 90, 'units': 'mm'})
                call('sketch.finish')
                edited = call('document.inspect')
                assert edited['authored'] != original
                assert names(edited) == current
                assert edited['evaluation']['bodies'][0]['valid']
                assert edited['evaluation']['bodies'][0]['volumeMm3'] != state['evaluation']['bodies'][0]['volumeMm3']
                edited_path = case / f'{stem}-radius90.alloy'
                call('document.save', {'path': str(edited_path)})
                call('document.open', {'path': str(edited_path), 'discard': True})
                reopened = call('document.inspect')
                assert reopened['authored'] == edited['authored'] and names(reopened) == current
                call('window.resize', {'width': 1500, 'height': 950})
                call('view.fit')
                capture = call('capture.screenshot', {'path': str(case / f'{stem}-radius90.png')})
                assert capture['source'] == 'QWidget chrome + OCCT OpenGL framebuffer'
                (case / f'{stem}-edited-state.json').write_text(json.dumps(reopened, indent=2)+'\n')
                # On the original checkpoint, source edit Undo/Redo is exact.
                call('document.open', {'path': str(path), 'discard': True})
                call('sketch.edit', {'sketchId': source['id']})
                call('sketch.dimensionUpdate', {'dimensionId': radius['id'], 'value': 90, 'units': 'mm'})
                call('sketch.finish')
                call('history.undo')
                assert call('document.inspect')['authored'] == original
                call('history.redo')
                assert call('document.inspect')['authored'] == edited['authored']
                call('history.undo')
                final = call('document.inspect')
                assert final['authored'] == original and names(final) == current
                call('view.fit')
                call('capture.screenshot', {'path': str(case / f'{stem}-original.png')})
                gpu = call('app.state')['viewport']['gpu']
                assert gpu['hardwareAccelerated'] and gpu['vendor'] and gpu['renderer'] and gpu['version']
                receipts.append({'process': process+1, 'fixture': stem, 'path': str(path),
                                 'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                                 'originalOpens': 5, 'editedReopen': True, 'radiusEditMm': [100, 90],
                                 'originalVolumeMm3': final['evaluation']['bodies'][0]['volumeMm3'],
                                 'editedVolumeMm3': edited['evaluation']['bodies'][0]['volumeMm3'],
                                 'names': current, 'gpu': gpu, 'capture': capture})
        finally:
            (case / 'transcript.json').write_text(json.dumps(ledger, indent=2)+'\n')
(out / 'receipt.json').write_text(json.dumps({'status': 'pass', 'appSha256': hashlib.sha256(Path(a.app).read_bytes()).hexdigest(),
                                            'cases': receipts}, indent=2)+'\n')
print('PASS: both retained brackets, 20 original opens across two processes, radius edits, IDs, Undo/Redo, saved edited reopen and GPU captures')
