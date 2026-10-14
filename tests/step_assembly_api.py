#!/usr/bin/env python3
"""Real-entrypoint retained STEP assembly import, identity, history and world export."""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path
from app_harness import application

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--app', required=True)
parser.add_argument('--fixture', required=True)
parser.add_argument('--output', required=True)
parser.add_argument('--step-tool')
args = parser.parse_args()
fixture = Path(args.fixture).resolve()
output = Path(args.output).resolve()
output.mkdir(parents=True, exist_ok=True)

with application(args.app, output, command_timeout=90) as (call, transcript):
    assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
    before = call('document.inspect')
    state = call('document.importStep', {'path': str(fixture)})
    assert state['bodies'] == 12 and state['undo']['count'] == before['undo']['count'] + 1
    feature = state['authored']['features'][-1]
    assert feature['type'] == 'stepAssemblyImport'
    assert feature['parameters']['source']['sha256'] == hashlib.sha256(fixture.read_bytes()).hexdigest()
    graph = feature['parameters']['assembly']
    assert len(graph['definitions']) == 18 and len(graph['occurrences']) == 18
    assert sum(row['kind'] == 'solid' for row in graph['occurrences']) == 12
    registry = call('component.inspect')['importedAssemblies']
    assert len(registry) == 1 and registry[0]['assembly'] == graph and registry[0]['readOnly']
    bodies = state['evaluation']['bodies']
    assert len({row['bodyId'] for row in bodies}) == 12
    assert len({row['occurrenceId'] for row in bodies}) == 12
    assert all(row['valid'] and row['closed'] and row['volumeMm3'] > 0 and row['supportFaces']
               and row['solidEdges'] for row in bodies)
    names = {row['name'] for row in bodies}
    assert 'PCB-001' in names and 'SIDE-USB-A-001' in names
    for body in bodies:
        selected = call('selection.set', {'bodyId': body['bodyId']})
        assert selected['occurrenceId'] == body['occurrenceId']
    call('view.home')
    call('capture.screenshot', {'path': str(output / 'assembly-import.png')})
    exported = output / 'leaf-world.step'
    leaf = next(row for row in bodies if row['name'] == 'SIDE-USB-A-001')
    result = call('document.exportStep', {'path': str(exported), 'bodyId': leaf['bodyId'], 'units': 'mm'})
    assert result['authoredChanged'] is False and exported.stat().st_size > 0
    if args.step_tool:
        readback = subprocess.run([str(Path(args.step_tool).resolve()), 'read', str(exported)],
                                  check=True, capture_output=True, text=True)
        independent = json.loads(readback.stdout.splitlines()[-1])
        assert independent['valid'] and independent['solidCount'] == 1
        assert all(abs(a-b) < 1e-4 for a, b in zip(independent['boundsMm'],
                                                    [70, 19, 1.6, 88, 35, 17.6])), independent
    assert call('document.inspect')['authored'] == state['authored']
    call('history.undo')
    assert call('document.inspect')['bodies'] == before['bodies']
    call('history.redo')
    assert call('document.inspect')['authored'] == state['authored']
    saved = output / 'retained-assembly.alloy'
    call('document.save', {'path': str(saved)})
    call('document.new', {'discard': True})
    call('document.open', {'path': str(saved), 'discard': True})
    reopened = call('document.inspect')
    assert reopened['authored'] == state['authored']
    assert {row['bodyId'] for row in reopened['evaluation']['bodies']} == {row['bodyId'] for row in bodies}
    assert call('component.inspect')['importedAssemblies'][0]['assembly'] == graph

(output / 'step-assembly-api.json').write_text(json.dumps({
    'result': 'passed', 'fixtureSha256': hashlib.sha256(fixture.read_bytes()).hexdigest(),
    'graph': graph, 'transcript': transcript,
}, indent=2) + '\n')
print('PASS: real-entrypoint retained XDE board assembly, 12 solids, identities, export, undo and reopen')
