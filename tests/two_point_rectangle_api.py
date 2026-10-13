#!/usr/bin/env python3
"""Real-entrypoint command-first typed two-point Rectangle contract."""
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
        pending = call('sketch.twoPointRectangle')
        assert pending['command'] == 'sketch.plane' and pending['pendingTool'] == 'rectangle'
        started = call('sketch.begin', {'plane': 'XY', 'units': 'mm'})
        assert started['tool'] == 'rectangle'
        sketch_id = started['draftSketch']['id']
        call('sketch.lookAt')
        def pixel(u, v):
            p = call('view.projectSketchPoint', {'sketchId': sketch_id, 'uMm': u, 'vMm': v})
            return {'x': p['x'], 'y': p['y']}
        def active():
            return call('app.state')['activeSession']
        call('sketch.placePoint', pixel(0, 0))
        call('sketch.pointerMove', pixel(20, -10))
        before = active()['draftSketch']
        preview = call('sketch.twoPointRectanglePreview', {'widthText': '60', 'heightText': '30'})
        assert preview['pendingTwoPointRectangle']['valid']
        assert (preview['pendingTwoPointRectangle']['uSign'], preview['pendingTwoPointRectangle']['vSign']) == (1, -1)
        assert active()['draftSketch'] == before
        call('capture.screenshot', {'path': str(output / 'rectangle-preview.png')})
        call('sketch.twoPointRectangleAccept')
        parameters = active()['draftSketch']['parameters']
        assert len(parameters['entities']) == 4 and len(parameters['dimensions']) == 2
        assert [d['valueMm'] for d in parameters['dimensions']] == [60, 30]
        assert sorted((p['uMm'], p['vMm']) for p in parameters['points'] if p['uMm'] in (0, 60) and p['vMm'] in (0, -30))
        assert {d['kind'] for d in parameters['dimensions']} == {'horizontal', 'vertical'}
        assert any(c['type'] == 'coincident' for c in parameters['constraints'])
        assert any(c['type'] == 'fixed' for c in parameters['constraints'])
        call('sketch.tool', {'tool': 'select'})
        call('history.undo')
        assert active()['draftSketch'] == before
        call('history.redo')
        assert active()['draftSketch']['parameters'] == parameters
        call('capture.screenshot', {'path': str(output / 'rectangle-committed.png')})
        call('sketch.finish')
        authored = call('document.inspect')['authored']
        path = output / 'two-point-rectangle.alloy'
        call('document.save', {'path': str(path)})
        call('document.open', {'path': str(path)})
        assert call('document.inspect')['authored'] == authored
        (output / 'result.json').write_text(json.dumps({'authored': authored, 'callCount': len(transcript)}, indent=2))


if __name__ == '__main__':
    main()
