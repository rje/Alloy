#!/usr/bin/env python3
"""One cumulative local-component workflow through the real application.
Launch only with the serialized private-editor slot. Uses independent STL readback.
"""
import argparse
import json
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl

p = argparse.ArgumentParser()
p.add_argument('--app', required=True)
p.add_argument('--output', required=True)
a = p.parse_args()
out = Path(a.output).resolve()
with application(a.app, out) as (call, transcript):
    initial = call('document.inspect')
    root = initial['authored']['documentId']
    call('solid.box', {'width': 100, 'depth': 100, 'height': 5, 'units': 'mm', 'name': 'Root stock'})
    created = call('component.create', {'name': 'die'})
    definition, occurrence = created['definitionId'], created['occurrenceId']
    assert len({root, definition, occurrence}) == 3
    before = call('document.inspect')
    call('component.activate', {'definitionId': root})
    activated = call('document.inspect')
    assert activated['authored'] == before['authored'] and activated['undo'] == before['undo']
    assert activated['revision'] == before['revision']
    call('component.activate', {'definitionId': definition})
    call('sketch.begin', {'plane': 'XY', 'name': 'Die sketch'})
    call('sketch.centerRectangle', {'centerX': 0, 'centerY': 0, 'width': 20, 'height': 20, 'units': 'mm'})
    call('sketch.finish')
    state = call('document.inspect')
    sketch = state['evaluation']['sketches'][0]
    call('solid.extrude', {'sketchId': sketch['sketchId'], 'profileId': sketch['profiles'][0]['profileId'], 'distance': 20, 'units': 'mm'})
    state = call('document.inspect')
    body = next(b for b in state['evaluation']['bodies'] if b['ownerId'] == definition)
    edges = [e['edgeId'] for e in body['solidEdges'] if e['available']]
    assert len(edges) == 12
    call('solid.fillet', {'targetBodyId': body['bodyId'], 'edgeIds': edges, 'radiusMm': 1, 'tangentChain': True})
    modeled = call('document.inspect')
    assert all(f['ownerId'] == definition for f in modeled['authored']['features'][1:])
    call('component.activate', {'definitionId': root})
    feature = modeled['authored']['features'][-1]['id']
    call('solid.fillet', {'featureId': feature, 'targetBodyId': body['bodyId'], 'edgeIds': edges, 'radiusMm': 1.2, 'tangentChain': True})
    edited = call('document.inspect')
    assert edited['authored']['features'][-1]['ownerId'] == definition
    call('history.undo')
    assert call('document.inspect')['authored'] == modeled['authored']
    call('history.redo')
    assert call('document.inspect')['authored'] == edited['authored']
    call('solid.move', {'targetBodyId': body['bodyId'], 'xMm': 1}, error='evaluation_failed')
    assert call('document.inspect')['authored'] == edited['authored']
    # Activation is isolated per design, without moving any existing geometry.
    call('component.activate', {'definitionId': definition})
    tabs = call('app.state')['documentTabs']
    first_tab = tabs['activeTabId']
    second = call('document.tabNew')
    assert call('document.inspect')['activeOwnerId'] != definition
    call('document.tabActivate', {'tabId': first_tab})
    assert call('document.inspect')['activeOwnerId'] == definition
    call('document.tabClose', {'tabId': second['activeTabId'], 'discard': True})
    call('view.home')
    call('capture.screenshot', {'path': str(out / 'components-browser.png')})
    path = out / 'die-component.alloy'
    call('document.save', {'path': str(path)})
    call('document.open', {'path': str(path)})
    reopened = call('document.inspect')
    assert reopened['authored'] == edited['authored'] and reopened['activeOwnerId'] == root
    call('component.activate', {'definitionId': definition})
    stl = out / 'die-component-mm.stl'
    exported = call('document.exportStl', {'occurrenceId': occurrence, 'path': str(stl), 'units': 'mm', 'format': 'binary', 'surfaceDeviationMm': .1, 'normalDeviationDeg': 10})
    assert exported['bodyCount'] == 1 and exported['bodyIds'] == [body['bodyId']]
    mesh = inspect_stl(stl, True)
    assert len(mesh['components']) == 1
    assert all(abs(mesh['bounds'][1][i] - mesh['bounds'][0][i] - 20) < .01 for i in range(3))
    dialog = call('document.exportMeshDialog')
    assert dialog['occurrenceId'] == occurrence
    (out / 'report.json').write_text(json.dumps({'passed': True, 'sourceReferenceId': 'tutorial-library-die', 'created': created, 'modeled': modeled, 'edited': edited, 'reopened': reopened, 'export': exported, 'mesh': mesh}, indent=2))
    (out / 'transcript.json').write_text(json.dumps(transcript, indent=2))
