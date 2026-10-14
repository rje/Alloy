#!/usr/bin/env python3
"""Actual centered rectangular prism, grouped blend, exact closed cavity and split."""
import argparse
import json
import math
import uuid
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl

p = argparse.ArgumentParser()
p.add_argument('--app', required=True)
p.add_argument('--output', required=True)
a = p.parse_args()
out = Path(a.output).resolve()

def ref(face):
    return {key: face[key] for key in ('producerFeatureId', 'bodyId', 'faceId')}

def group(edges, radius):
    return {'id': str(uuid.uuid4()), 'edgeIds': edges, 'radiusMm': radius, 'tangentChain': True}

with application(a.app, out) as (call, transcript):
    def inspect():
        return call('document.inspect')
    assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
    call('sketch.begin', {'plane': 'XY', 'name': 'Centered stock'})
    call('sketch.centerRectangle', {'centerX': 0, 'centerY': 0, 'width': 93, 'height': 64,
        'units': 'mm', 'centerSnapTarget': {'kind': 'origin'}})
    sketch = call('app.state')['activeSession']['draftSketch']
    sketch_id = sketch['id']
    call('sketch.finish')
    profile = inspect()['evaluation']['sketches'][0]['profiles'][0]['profileId']
    call('solid.extrude', {'sketchId': sketch_id, 'profileId': profile, 'distance': 30, 'units': 'mm'})
    stock = inspect()['evaluation']['bodies'][0]
    body_id = stock['bodyId']
    vertical = [edge['edgeId'] for edge in stock['solidEdges'] if edge['provenance']['role'] == 'sideCorner']
    rims = [edge['edgeId'] for edge in stock['solidEdges'] if edge['provenance']['role'] in ('baseRim', 'endRim')]
    assert len(vertical) == 4 and len(rims) == 8
    call('solid.fillet', {'targetBodyId': body_id, 'groups': [group(vertical, 5), group(rims, 2.5)]})
    rounded = inspect()
    outer = rounded['evaluation']['bodies'][0]
    assert outer['valid'] and len(outer['radiusGroups']) == 2
    session = call('solid.shellDialog', {'targetBodyId': body_id, 'thicknessMm': 3.5})
    assert session['previewed'] and session['geometry']['closedCavity']
    assert session['geometry']['topology']['shells'] == 2
    assert inspect()['authored'] == rounded['authored']
    call('capture.screenshot', {'path': str(out / 'rounded-shell-preview.png')})
    call('capture.dialog', {'path': str(out / 'rounded-shell-panel.png')})
    call('command.cancel', {'session': session['id']})
    assert inspect()['authored'] == rounded['authored']
    call('solid.shell', {'targetBodyId': body_id, 'thicknessMm': 3.5})
    shelled = inspect()
    material = shelled['evaluation']['bodies'][0]
    cavity = (86 * 57 - (4 - math.pi) * 1.5**2) * 23
    assert material['valid'] and material['closed'] and material['topology']['shells'] == 2
    assert math.isclose(material['volumeMm3'], outer['volumeMm3'] - cavity, abs_tol=1e-4)
    assert len([f for f in material['supportFaces'] if f['role'] == 'shell-inner']) == 6
    top = ref(next(f for f in material['supportFaces'] if f['role'] == 'extrude.endCap'))
    bottom = ref(next(f for f in material['supportFaces'] if f['role'] == 'extrude.base'))
    call('inspect.section', {'plane': 'XZ', 'hatch': True})
    call('view.home')
    call('capture.screenshot', {'path': str(out / 'rounded-shell-section.png')})
    plane_id = call('construction.midplane', {'faces': [top, bottom], 'name': 'Midplane'})['planeId']
    call('solid.splitBody', {'targetBodyId': body_id, 'planeId': plane_id})
    split = inspect()
    halves = split['evaluation']['bodies']
    assert len(halves) == 2 and all(b['valid'] and b['closed'] for b in halves)
    assert math.isclose(sum(b['volumeMm3'] for b in halves), material['volumeMm3'], abs_tol=1e-4)
    exports = []
    for i, body in enumerate(halves):
        path = out / f'rounded-half-{i}.stl'
        call('document.exportStl', {'path': str(path), 'bodyId': body['bodyId'], 'units': 'mm',
            'format': 'binary', 'surfaceDeviationMm': .1, 'normalDeviationDeg': 10})
        mesh = inspect_stl(path, True)
        assert len(mesh['components']) == 1
        exports.append({'bodyId': body['bodyId'], 'mesh': mesh})
    path = out / 'rounded-shell-split.alloy'
    call('document.save', {'path': str(path)})
    call('document.open', {'path': str(path), 'discard': True})
    reopened = inspect()
    assert reopened['authored'] == split['authored']
    assert len(reopened['evaluation']['bodies']) == 2
    call('capture.screenshot', {'path': str(out / 'rounded-shell-reopened.png')})
    (out / 'receipt.json').write_text(json.dumps({'rounded': rounded, 'shelled': shelled,
        'split': split, 'exports': exports, 'transcript': transcript}, indent=2) + '\n')
print('PASS: exact rounded cavity, preview/cancel, Midplane split, two STL exports and reopen')
