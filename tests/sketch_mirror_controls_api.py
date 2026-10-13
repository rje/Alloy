#!/usr/bin/env python3
"""Real-entrypoint Mirror roles and enclosed-window collection, without native-event claims."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    output = Path(args.output).resolve()
    with application(args.app, output) as (call, transcript):
        call('sketch.begin', {'plane': 'XY', 'units': 'mm'})
        call('sketch.lookAt')
        draft = lambda: call('app.state')['activeSession']['draftSketch']
        def line(x1, y1, x2, y2):
            call('sketch.line', {'x1': x1, 'y1': y1, 'x2': x2, 'y2': y2, 'units': 'mm'})
            return draft()['parameters']['entities'][-1]['id']
        for segment in [(-50,-50,50,-50),(50,-50,50,50),(50,50,-50,50),(-50,50,-50,-50)]:
            line(*segment)
        diagonals = [line(-45,0,45,45), line(-45,45,45,0)]
        call('sketch.construction', {'entityIds': diagonals, 'construction': True})
        call('sketch.addCircle', {'centerX': 0, 'centerY': 0, 'diameter': 12, 'units': 'mm'})
        source = [line(*segment) for segment in [(-14,10,12,10),(12,10,12,36),(12,36,-14,36),(-14,36,-14,10)]]
        axis = line(-25,-3,25,-3)
        call('sketch.construction', {'entityIds': [source[1],source[3],axis], 'construction': True})
        for x, start_y, end_y, side in [(-14,36,10,-1),(12,10,36,1)]:
            call('sketch.addArc', {'mode': 'threePoint', 'startX': x, 'startY': start_y,
                                  'endX': x, 'endY': end_y, 'throughX': x+side*math.sqrt(56),
                                  'throughY': 23, 'units': 'mm'})
            source.append(draft()['parameters']['entities'][-1]['id'])
        original = draft()
        def pixel(u, v):
            return call('view.projectSketchPoint', {'sketchId': original['id'], 'uMm': u, 'vMm': v})
        a, b = pixel(-24,39), pixel(23,7)
        call('sketch.mirrorStart')
        call('sketch.mirrorStagePreview', error='mirror_source_required')
        selected = call('sketch.mirrorWindow', {'x1': a['x'], 'y1': a['y'], 'x2': b['x'], 'y2': b['y']})
        assert selected['curveCount'] == 6 and selected['pointCount'] == 6 and selected['objectCount'] == 12
        assert set(selected['mirrorSelection']['sourceEntityIds']) == set(source)
        call('sketch.mirrorStagePreview', error='mirror_axis_required')
        cleared = call('sketch.mirrorClear')
        assert not cleared['mirrorSelection']['sourceEntityIds'] and not cleared['mirrorSelection']['pointIds']
        selected = call('sketch.mirrorWindow', {'x1': a['x'], 'y1': a['y'], 'x2': b['x'], 'y2': b['y']})
        assert selected['objectCount'] == 12
        call('sketch.mirrorRole', {'role': 'axis'})
        center = pixel(0,-3)
        chosen = call('sketch.mirrorPick', {'x': center['x'], 'y': center['y']})
        assert chosen['mirrorSelection']['axisEntityId'] == axis
        preview = call('sketch.mirrorStagePreview')
        assert preview['pending'] and len(preview['mirrorGroup']['entityPairs']) == 6
        assert draft() == original
        call('capture.screenshot', {'path': str(output/'mirror-api-preview.png')})
        call('sketch.mirrorExit')
        assert draft() == original and 'pendingSketchMirror' not in call('app.state')['activeSession']
        call('sketch.mirrorAccept', error='mirror_preview_required')
        call('sketch.mirrorStart')
        call('sketch.mirrorWindow', {'x1': a['x'], 'y1': a['y'], 'x2': b['x'], 'y2': b['y']})
        call('sketch.mirrorRole', {'role': 'axis'})
        call('sketch.mirrorPick', {'x': center['x'], 'y': center['y']})
        call('sketch.mirrorStagePreview')
        call('sketch.mirrorAccept')
        authored = draft()
        assert len(authored['parameters']['mirrorGroups']) == 1
        assert len(authored['parameters']['mirrorGroups'][0]['entityPairs']) == 6
        call('capture.screenshot', {'path': str(output/'mirror-api-accepted.png')})
        (output/'report.json').write_text(json.dumps({'curveCount': 6, 'pointCount': 6,
            'objectCount': 12, 'axisId': axis, 'callCount': len(transcript)}, indent=2)+'\n')
    print('PASS: shared Mirror collection roles, true enclosure, preview, Cancel and Accept')


if __name__ == '__main__':
    main()
