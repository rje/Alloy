#!/usr/bin/env python3
"""Short real-app check that Mirror never counts a point without its source curve."""
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
        call('sketch.begin', {'plane': 'XY'})
        call('sketch.lookAt')
        draft = lambda: call('app.state')['activeSession']['draftSketch']
        call('sketch.line', {'x1': 20, 'y1': 20, 'x2': 40, 'y2': 20, 'units': 'mm'})
        source = draft()['parameters']['entities'][-1]
        call('sketch.addCircle', {'centerX': 0, 'centerY': 0, 'diameter': 8, 'units': 'mm'})
        circle = draft()['parameters']['entities'][-1]
        original = draft()
        call('sketch.selectionSet', {'entityIds': [source['id'], circle['id']],
                                     'pointIds': [source['startPointId'], circle['centerPointId']]})
        started = call('sketch.mirrorStart')['mirrorSelection']
        assert started['sourceEntityIds'] == [source['id']]
        assert started['pointIds'] == [source['startPointId']]
        assert call('app.state')['activeSession']['selection']['entityIds'] == [source['id']]
        def pixel(u, v):
            return call('view.projectSketchPoint', {'sketchId': original['id'], 'uMm': u, 'vMm': v})
        unrelated = pixel(0, 0)
        before = call('app.state')['activeSession']
        call('sketch.mirrorPick', {'x': unrelated['x'], 'y': unrelated['y']},
             error='mirror_point_requires_curve')
        after = call('app.state')['activeSession']
        assert after['mirrorSelection'] == before['mirrorSelection']
        assert after['selection'] == before['selection']
        assert draft() == original and 'pendingSketchMirror' not in after
        related = pixel(40, 20)
        picked = call('sketch.mirrorPick', {'x': related['x'], 'y': related['y']})
        assert picked['picked']['pointId'] == source['endPointId']
        assert picked['mirrorSelection']['pointIds'] == [source['startPointId'], source['endPointId']]
        (output/'report.json').write_text(json.dumps({'sourceEntityId': source['id'],
            'rejectedPointId': circle['centerPointId'], 'acceptedPointId': source['endPointId'],
            'callCount': len(transcript)}, indent=2)+'\n')
    print('PASS: Mirror filters unrelated preselection and refuses standalone point picks')


if __name__ == '__main__':
    main()
