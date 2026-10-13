#!/usr/bin/env python3
"""Real-entrypoint Circle diameter lock, acceptance and cancellation contract."""
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
    with application(args.app, output) as (call, transcript):
        started = call('sketch.begin', {'plane': 'XZ', 'units': 'mm'})
        sketch_id = started['draftSketch']['id']
        call('sketch.lookAt')
        def pixel(u, v):
            p = call('view.projectSketchPoint', {'sketchId': sketch_id, 'uMm': u, 'vMm': v})
            return {'x': p['x'], 'y': p['y']}
        def active():
            return call('app.state')['activeSession']
        call('sketch.tool', {'tool': 'circle'})
        call('sketch.placePoint', pixel(0, 0))
        before = active()['draftSketch']
        call('sketch.circleDiameterPreview', {'text': '2'})
        locked = call('sketch.circleDiameterLock')
        assert locked['pendingCircleDiameter']['phase'] == 'locked'
        assert active()['draftSketch'] == before
        call('sketch.circleDiameterAccept')
        parameters = active()['draftSketch']['parameters']
        assert len(parameters['entities']) == 1 and parameters['entities'][0]['radiusMm'] == 1
        assert len(parameters['dimensions']) == 1 and parameters['dimensions'][0]['valueMm'] == 2
        call('sketch.placePoint', pixel(5, 0))
        call('sketch.circleDiameterPreview', {'text': '3'})
        call('sketch.circleDiameterLock')
        call('sketch.circleDiameterCancel')
        assert active()['draftSketch']['parameters'] == parameters
        call('sketch.tool', {'tool': 'select'})
        call('sketch.finish')
        authored = call('document.inspect')['authored']
        path = output / 'circle-tab-api.alloy'
        call('document.save', {'path': str(path)})
        call('document.open', {'path': str(path)})
        assert call('document.inspect')['authored'] == authored
        (output / 'result.json').write_text(json.dumps({'authored': authored, 'callCount': len(transcript)}, indent=2))


if __name__ == '__main__':
    main()
