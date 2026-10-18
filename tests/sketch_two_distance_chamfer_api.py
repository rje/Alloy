#!/usr/bin/env python3
"""Independent real-app gate for retained Two Distance sketch Chamfer."""
import argparse
import json
import math
from pathlib import Path

from app_harness import application


def near(actual, expected, tolerance=1e-4):
    assert math.isclose(actual, expected, abs_tol=tolerance), (actual, expected)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    out = Path(args.output).resolve()
    out.mkdir(parents=True, exist_ok=True)

    with application(args.app, out) as (call, transcript):
        call('sketch.begin', {'plane': 'XZ', 'name': 'Two distance corner', 'units': 'mm'})
        call('sketch.centerRectangle', {'centerX': 0, 'centerY': 0,
                                         'width': 150, 'height': 75, 'units': 'mm'})
        draft = call('app.state')['activeSession']['draftSketch']
        sketch_id = draft['id']
        lines = [e['id'] for e in draft['parameters']['entities']
                 if e['type'] == 'line' and not e.get('construction')]
        assert len(lines) == 4
        width_id = next(d['id'] for d in draft['parameters']['dimensions']
                        if d['kind'] == 'horizontal')
        height_id = next(d['id'] for d in draft['parameters']['dimensions']
                         if d['kind'] == 'vertical')
        call('sketch.chamfer', {'entityIds': [lines[1], lines[2]],
                                'type': 'twoDistance', 'value': 10, 'value2': 20,
                                'units': 'mm'})
        group = call('app.state')['activeSession']['draftSketch']['parameters']['chamferGroups'][0]
        assert group['type'] == 'twoDistance'
        assert group['sourceEntityIds'] == [lines[1], lines[2]]
        stable = (group['id'], group['lineId'], tuple(group['pointIds']))
        near(group['distanceMm'], 10)
        near(group['distance2Mm'], 20)
        call('capture.screenshot', {'path': str(out/'two-distance-sketch.png')})
        call('sketch.finish')
        before = call('document.inspect')
        profile = before['evaluation']['sketches'][0]['profiles'][0]
        near(profile['areaMm2'], 150*75-10*20/2)
        call('solid.extrude', {'sketchId': sketch_id, 'profileId': profile['profileId'],
                               'distance': 5, 'units': 'mm'})
        body_id = call('document.inspect')['evaluation']['bodies'][0]['bodyId']

        call('sketch.edit', {'sketchId': sketch_id})
        call('sketch.chamferUpdate', {'chamferId': group['id'],
                                      'value2': 18, 'units': 'mm'})
        intermediate = call('app.state')['activeSession']['draftSketch']['parameters']['chamferGroups'][0]
        near(intermediate['distanceMm'], 10)
        near(intermediate['distance2Mm'], 18)
        call('sketch.chamferUpdate', {'chamferId': group['id'],
                                      'value': 12, 'units': 'mm'})
        call('sketch.dimensionUpdate', {'dimensionId': width_id,
                                         'value': 160, 'units': 'mm'})
        call('sketch.dimensionUpdate', {'dimensionId': height_id,
                                         'value': 80, 'units': 'mm'})
        state = call('app.state')
        edited = state['activeSession']['draftSketch']['parameters']['chamferGroups'][0]
        assert (edited['id'], edited['lineId'], tuple(edited['pointIds'])) == stable
        near(edited['distanceMm'], 12)
        near(edited['distance2Mm'], 18)
        unchanged = state['activeSession']['draftSketch']
        call('sketch.chamferUpdate', {'chamferId': group['id'],
                                      'value2': 160, 'units': 'mm'},
             error='sketch_chamfer_failed')
        assert call('app.state')['activeSession']['draftSketch'] == unchanged
        call('capture.screenshot', {'path': str(out/'two-distance-edited.png')})
        call('sketch.finish')
        final = call('document.inspect')
        expected_area = 160*80-12*18/2
        near(final['evaluation']['sketches'][0]['profiles'][0]['areaMm2'], expected_area)
        assert final['evaluation']['bodies'][0]['bodyId'] == body_id
        near(final['evaluation']['bodies'][0]['volumeMm3'], expected_area*5)
        call('history.undo')
        call('history.redo')
        assert call('document.inspect')['authored'] == final['authored']
        model = out/'two-distance.alloy'
        call('document.save', {'path': str(model)})
        call('document.open', {'path': str(model)})
        reopened = call('document.inspect')
        assert reopened['authored'] == final['authored']
        near(reopened['evaluation']['bodies'][0]['volumeMm3'], expected_area*5)
        call('view.home')
        call('capture.screenshot', {'path': str(out/'two-distance-solid.png')})
        (out/'receipt.json').write_text(json.dumps({
            'passed': True, 'sketchId': sketch_id, 'bodyId': body_id,
            'chamferIdentity': stable, 'expectedAreaMm2': expected_area,
            'expectedVolumeMm3': expected_area*5, 'transcript': transcript}, indent=2)+'\n')
    print('PASS Two Distance sketch Chamfer, independent values and reopen')


if __name__ == '__main__':
    main()
