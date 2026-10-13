#!/usr/bin/env python3
"""Real-entrypoint stacked-circle construction planes; launch only in the desktop slot."""
import argparse
import json
from pathlib import Path
from app_harness import application

parser = argparse.ArgumentParser()
parser.add_argument('--app', required=True)
parser.add_argument('--output', required=True)
args = parser.parse_args()
out = Path(args.output).resolve()
with application(args.app, out) as (call, transcript):
    initial = call('app.state')
    assert initial['viewport']['gpu']['hardwareAccelerated']
    source = {'plane': 'XY'}
    planes = []
    for index, (z, diameter, offset) in enumerate([(0, 38, 0), (18, 16, 18), (38, 30, 20), (58, 44, 20)]):
        if index:
            plane = call('construction.plane', {'source': source, 'offset': offset, 'units': 'mm', 'name': f'Plane z{z}'})['planeId']
            planes.append(plane)
            selection = call('selection.planeSet', {'planeId': plane})
            assert abs(selection['frame']['origin'][2] - z) < 1e-7
            session = call('command.begin', {'command': 'sketch.create', 'inputs': {'planeId': plane, 'name': f'Circle {diameter}'}})
        else:
            session = call('sketch.begin', {'plane': 'XY', 'name': 'Circle 38'})
        call('sketch.addCircle', {'centerX': 0, 'centerY': 0, 'diameter': diameter, 'units': 'mm'})
        call('sketch.finish')
        source = {'sketchId': call('document.inspect')['authored']['features'][-1]['id']}
    baseline = call('document.inspect')
    assert [p['frame']['origin'][2] for p in baseline['evaluation']['constructionPlanes']] == [18, 38, 58]
    call('view.orientation', {'name': 'isometric'})
    call('capture.screenshot', {'path': str(out / 'stacked-circles.png')})
    call('construction.plane', {'featureId': planes[0], 'source': {'sketchId': baseline['authored']['features'][0]['id']}, 'offset': 19, 'units': 'mm'})
    edited = call('document.inspect')
    assert [p['frame']['origin'][2] for p in edited['evaluation']['constructionPlanes']] == [19, 39, 59]
    call('history.undo')
    assert call('document.inspect')['authored'] == baseline['authored']
    call('history.redo')
    assert call('document.inspect')['authored'] == edited['authored']
    path = out / 'stacked-circles.alloy'
    call('document.save', {'path': str(path)})
    call('document.new', {'discard': True})
    call('document.open', {'path': str(path), 'discard': True})
    assert call('document.inspect')['authored'] == edited['authored']
    (out / 'construction-plane-ledger.json').write_text(json.dumps({'initial': initial, 'baseline': baseline, 'edited': edited, 'transcript': transcript}, indent=2))
