#!/usr/bin/env python3
"""Independent real-app gate for two retained sketch Chamfers sharing a side."""
import argparse
import json
import math
from pathlib import Path

from app_harness import application


def near(actual, expected, tolerance=1e-4):
    assert math.isclose(actual, expected, abs_tol=tolerance), (actual, expected)


def groups(state):
    return state['activeSession']['draftSketch']['parameters']['chamferGroups']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    out = Path(args.output).resolve()
    out.mkdir(parents=True, exist_ok=True)

    with application(args.app, out) as (call, transcript):
        call('sketch.begin', {'plane': 'XZ', 'name': 'Two adjacent chamfers', 'units': 'mm'})
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
                                'value': 10, 'units': 'mm'})
        call('sketch.chamfer', {'entityIds': [lines[2], lines[3]],
                                'value': 8, 'units': 'mm'})
        placed = groups(call('app.state'))
        assert len(placed) == 2
        identities = [(g['id'], g['lineId'], tuple(g['pointIds'])) for g in placed]
        near(placed[0]['distanceMm'], 10)
        near(placed[1]['distanceMm'], 8)
        call('capture.screenshot', {'path': str(out/'two-chamfers-sketch.png')})
        call('sketch.finish')
        initial = call('document.inspect')
        profile = initial['evaluation']['sketches'][0]['profiles'][0]
        near(profile['areaMm2'], 150*75-(10**2+8**2)/2)
        call('solid.extrude', {'sketchId': sketch_id, 'profileId': profile['profileId'],
                               'distance': 5, 'units': 'mm'})
        body_id = call('document.inspect')['evaluation']['bodies'][0]['bodyId']

        call('sketch.edit', {'sketchId': sketch_id})
        call('sketch.chamferUpdate', {'chamferId': placed[0]['id'],
                                      'value': 12, 'units': 'mm'})
        call('sketch.chamferUpdate', {'chamferId': placed[1]['id'],
                                      'value': 9, 'units': 'mm'})
        call('sketch.dimensionUpdate', {'dimensionId': width_id,
                                         'value': 160, 'units': 'mm'})
        call('sketch.dimensionUpdate', {'dimensionId': height_id,
                                         'value': 80, 'units': 'mm'})
        state = call('app.state')
        edited = groups(state)
        assert len(edited) == 2
        assert [(g['id'], g['lineId'], tuple(g['pointIds'])) for g in edited] == identities
        near(edited[0]['distanceMm'], 12)
        near(edited[1]['distanceMm'], 9)
        authored_before_refusal = state['activeSession']['draftSketch']
        call('sketch.chamferUpdate', {'chamferId': edited[0]['id'],
                                      'value': 155, 'units': 'mm'},
             error='sketch_chamfer_failed')
        assert call('app.state')['activeSession']['draftSketch'] == authored_before_refusal
        call('capture.screenshot', {'path': str(out/'two-chamfers-edited.png')})
        call('sketch.finish')
        final = call('document.inspect')
        assert final['evaluation']['bodies'][0]['bodyId'] == body_id
        expected_area = 160*80-(12**2+9**2)/2
        near(final['evaluation']['sketches'][0]['profiles'][0]['areaMm2'], expected_area)
        near(final['evaluation']['bodies'][0]['volumeMm3'], expected_area*5)
        call('history.undo')
        call('history.redo')
        assert call('document.inspect')['authored'] == final['authored']
        model = out/'two-chamfers.alloy'
        call('document.save', {'path': str(model)})
        call('document.open', {'path': str(model)})
        reopened = call('document.inspect')
        assert reopened['authored'] == final['authored']
        near(reopened['evaluation']['bodies'][0]['volumeMm3'], expected_area*5)
        call('view.home')
        call('capture.screenshot', {'path': str(out/'two-chamfers-solid.png')})
        (out/'receipt.json').write_text(json.dumps({
            'passed': True, 'sketchId': sketch_id, 'bodyId': body_id,
            'chamferIdentities': identities, 'expectedAreaMm2': expected_area,
            'expectedVolumeMm3': expected_area*5, 'transcript': transcript}, indent=2)+'\n')
    print('PASS two adjacent sketch Chamfers, independent edits and reopen')


if __name__ == '__main__':
    main()
