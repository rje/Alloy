#!/usr/bin/env python3
"""Real-entrypoint shared-command replay of seven typed, pointer-directed Line segments."""
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
    source = [(0, 40, 0, 20, 40, 'vertical'),
              (14, 40, 10, 40, 14, 'horizontal'),
              (14, -20, 14, 0, 60, 'vertical'),
              (3, -20, 8, -20, 11, 'horizontal'),
              (3, 24, 3, 0, 44, 'vertical'),
              (11, 24, 8, 24, 8, 'horizontal'),
              (11, 0, 11, 10, 24, 'vertical')]
    with application(args.app, output) as (call, transcript):
        started = call('sketch.begin', {'plane': 'XY', 'units': 'mm'})
        sketch_id = started['draftSketch']['id']
        call('sketch.lookAt')
        call('sketch.tool', {'tool': 'line'})
        def pixel(u, v):
            p = call('view.projectSketchPoint', {'sketchId': sketch_id, 'uMm': u, 'vMm': v})
            return {'x': p['x'], 'y': p['y']}
        def draft():
            return call('app.state')['activeSession']['draftSketch']
        call('sketch.placePoint', pixel(0, 0))
        baseline = draft()
        call('sketch.lineLengthPreview', {'text': '0'})
        call('sketch.lineLengthLock', error='invalid_line_length')
        call('sketch.placePoint', pixel(0, 20), error='line_length_pending')
        assert draft() == baseline
        call('sketch.lineLengthCancel')
        for index, (u, v, pu, pv, length, direction) in enumerate(source):
            text = str(length) if index != 4 else '44 mm'
            preview = call('sketch.lineLengthPreview', {'text': text})
            assert preview['pendingLineLength']['valid']
            assert not preview['authoredChange']
            call('sketch.lineLengthLock')
            moved = call('sketch.pointerMove', pixel(pu, pv))
            assert moved['point']['direction'] == direction
            assert not moved['authoredChange']
            call('sketch.placePoint', pixel(pu, pv))
            parameters = draft()['parameters']
            assert len(parameters['entities']) == index + 1
            assert len(parameters['dimensions']) == index + 1
            assert parameters['dimensions'][-1]['valueMm'] == length
            assert parameters['dimensions'][-1]['kind'] == direction
            end_id = parameters['entities'][-1]['endPointId']
            end = next(p for p in parameters['points'] if p['id'] == end_id)
            assert abs(end['uMm'] - u) < 1e-6 and abs(end['vMm'] - v) < 1e-6
        accepted = draft()
        assert sum(c['type'] in ('horizontal', 'vertical') for c in accepted['parameters']['constraints']) == 7
        call('sketch.tool', {'tool': 'select'})
        call('history.undo')
        assert len(draft()['parameters']['entities']) == 6
        call('history.redo')
        assert draft() == accepted
        call('sketch.finish')
        original = call('document.inspect')['authored']
        path = output / 'typed-line-api.alloy'
        call('document.save', {'path': str(path)})
        call('document.open', {'path': str(path)})
        assert call('document.inspect')['authored'] == original
        (output / 'result.json').write_text(json.dumps({'source': source, 'authored': original,
            'callCount': len(transcript)}, indent=2))


if __name__ == '__main__':
    main()
