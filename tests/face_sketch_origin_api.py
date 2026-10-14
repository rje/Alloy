#!/usr/bin/env python3
"""Fresh direct-face origin through the real local command entrypoint."""
import argparse
import json
from pathlib import Path
from app_harness import application

p = argparse.ArgumentParser()
p.add_argument('--app', required=True)
p.add_argument('--output', required=True)
a = p.parse_args()
output = Path(a.output).resolve()
with application(a.app, output) as (call, transcript):
    call('solid.box', {'width': 64, 'depth': 93, 'height': 30,
                       'x': -32, 'y': -46.5, 'z': 0, 'units': 'mm'})
    body = call('document.inspect')['evaluation']['bodies'][0]
    face = next(f for f in body['supportFaces'] if f['role'] == 'box.y0')
    support = {k: face[k] for k in ('producerFeatureId', 'bodyId', 'faceId')}
    session = call('sketch.begin', {'support': support, 'name': 'Projected owner origin'})
    frame = session['resolvedFrame']
    assert frame['origin'] == [0, -46.5, 0], frame
    assert frame['uAxis'] == face['frame']['uAxis']
    assert frame['vAxis'] == face['frame']['vAxis']
    assert session['draftSketch']['parameters']['originConvention'] == 'projectedOwnerV1'
    call('sketch.centerRectangle', {'centerX': 32, 'centerY': 0, 'width': 16, 'height': 12,
                                    'units': 'mm', 'centerSnapTarget': {'kind': 'origin'}})
    call('capture.screenshot', {'path': str(output / 'fresh-face-origin-sketch.png')})
    sketch_id = call('app.state')['activeSession']['draftSketch']['id']
    call('sketch.finish')
    snapshot = call('document.inspect')
    sketch = next(s for s in snapshot['evaluation']['sketches'] if s['sketchId'] == sketch_id)
    assert sketch['frame']['origin'] == frame['origin']
    assert any(point['positionMm'] == [0, -46.5, 0] for point in sketch['worldPoints'])
    path = output / 'projected-origin.alloy'
    call('document.save', {'path': str(path)})
    call('document.open', {'path': str(path)})
    reopened = call('document.inspect')
    assert reopened['authored'] == snapshot['authored']
    assert next(s for s in reopened['evaluation']['sketches'] if s['sketchId'] == sketch_id)['frame'] == sketch['frame']
    (output / 'result.json').write_text(json.dumps({'passed': True, 'scope': 'API fresh face origin, snap, save/reopen',
                                                    'originMm': frame['origin'], 'calls': len(transcript)}, indent=2))
