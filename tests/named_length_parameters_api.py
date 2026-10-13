#!/usr/bin/env python3
"""Bounded named-length lifecycle through the actual Alloy process."""
import argparse
import json
from pathlib import Path
from app_harness import application


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    output = Path(args.output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    with application(args.app, output) as (call, transcript):
        for name, value in [('Length', 75), ('Width', 50), ('Thickness', 10)]:
            call('document.parameterAdd', {'name': name, 'value': value, 'units': 'mm'})
        rows = {row['name']: row for row in call('document.inspect')['userParameters']}
        call('sketch.begin', {'plane': 'XY', 'units': 'mm'})
        call('sketch.rectangle', {'x1': -30, 'y1': -20, 'x2': 30, 'y2': 20, 'units': 'mm'})
        sketch = call('app.state')['activeSession']['draftSketch']
        points, edges = sketch['parameters']['points'], sketch['parameters']['entities']
        call('sketch.constraint', {'type': 'fixed', 'pointIds': [points[0]['id']]})
        call('sketch.dimension', {'kind': 'horizontal', 'entityId': edges[0]['id'], 'parameterName': 'Length'})
        call('sketch.dimension', {'kind': 'vertical', 'entityId': edges[1]['id'], 'parameterName': 'Width'})
        call('sketch.finish')
        profile = call('document.inspect')['evaluation']['sketches'][0]['profiles'][0]
        call('solid.extrude', {'sketchId': sketch['id'], 'profileId': profile['profileId'],
                              'distanceParameter': 'Thickness', 'units': 'mm'})
        initial = call('document.inspect')
        assert abs(initial['evaluation']['bodies'][0]['volumeMm3'] - 37500) < 1e-5
        before = initial['authored']
        call('document.parameterUpdate', {'parameterId': rows['Length']['id'], 'value': 85})
        changed = call('document.inspect')
        assert abs(changed['evaluation']['bodies'][0]['volumeMm3'] - 42500) < 1e-5
        call('history.undo')
        assert call('document.inspect')['authored'] == before
        call('history.redo')
        assert call('document.inspect')['authored'] == changed['authored']
        call('document.parameterUpdate', {'parameterId': rows['Length']['id'], 'name': 'Width'}, 'invalid_parameter')
        assert call('document.inspect')['authored'] == changed['authored']
        path = output / 'named-lengths.alloy'
        call('document.save', {'path': str(path)})
        call('document.open', {'path': str(path), 'discard': True})
        reopened = call('document.inspect')
        assert reopened['authored'] == changed['authored']
        assert abs(reopened['evaluation']['bodies'][0]['volumeMm3'] - 42500) < 1e-5
        call('capture.screenshot', {'path': str(output / 'named-length-api.png')})
        (output / 'api-state.json').write_text(json.dumps(reopened, indent=2) + '\n')
    (output / 'api-transcript.json').write_text(json.dumps(transcript, indent=2) + '\n')
    print('PASS: real-app bound sketch and Extrude, update, undo/redo, refusal, save/reopen')


if __name__ == '__main__':
    main()
