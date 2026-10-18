#!/usr/bin/env python3
"""Independent real-app gate for a circle tangent to three retained sketch lines."""
import argparse
import json
import math
from pathlib import Path

from app_harness import application


def near(actual, expected, tolerance=1e-4):
    assert math.isclose(actual, expected, abs_tol=tolerance), (actual, expected)


def circle_state(draft, circle_id):
    parameters = draft['parameters']
    circle = next(e for e in parameters['entities'] if e['id'] == circle_id)
    center = next(p for p in parameters['points'] if p['id'] == circle['centerPointId'])
    return circle, center


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    out = Path(args.output).resolve()
    out.mkdir(parents=True, exist_ok=True)

    with application(args.app, out) as (call, transcript):
        call('sketch.begin', {'plane': 'XY', 'name': 'Three-line tangent circle', 'units': 'mm'})
        call('sketch.rectangle', {'x1': 0, 'y1': 0, 'x2': 30, 'y2': 30, 'units': 'mm'})
        draft = call('app.state')['activeSession']['draftSketch']
        sketch_id = draft['id']
        lines = [e for e in draft['parameters']['entities'] if e['type'] == 'line']
        assert len(lines) == 4
        bottom, right, top = lines[:3]
        call('sketch.constraint', {'type': 'fixed', 'pointIds': [bottom['startPointId']]})
        call('sketch.dimension', {'kind': 'horizontal', 'entityId': bottom['id'],
                                  'value': 30, 'units': 'mm'})
        call('sketch.dimension', {'kind': 'vertical', 'entityId': right['id'],
                                  'value': 30, 'units': 'mm'})
        draft = call('app.state')['activeSession']['draftSketch']
        height_id = next(d['id'] for d in draft['parameters']['dimensions']
                         if d['kind'] == 'vertical')
        call('sketch.construction', {'entityIds': [line['id'] for line in lines],
                                     'construction': True})
        call('sketch.threeTangentCircle', {'lineIds': [bottom['id'], right['id'], top['id']]})
        placed = call('app.state')['activeSession']['draftSketch']
        circles = [e for e in placed['parameters']['entities'] if e['type'] == 'circle']
        assert len(circles) == 1
        circle_id = circles[0]['id']
        circle, center = circle_state(placed, circle_id)
        near(circle['radiusMm'], 15)
        near(center['uMm'], 15)
        near(center['vMm'], 15)
        assert not any(d.get('entityId') == circle_id for d in placed['parameters']['dimensions'])
        relations = [c for c in placed['parameters']['constraints']
                     if c['type'] == 'tangent' and c['entityIds'][0] == circle_id]
        assert {c['entityIds'][1] for c in relations} == {bottom['id'], right['id'], top['id']}
        call('capture.screenshot', {'path': str(out/'three-tangent-circle-sketch.png')})
        call('sketch.finish')
        profile = call('document.inspect')['evaluation']['sketches'][0]['profiles'][0]
        near(profile['areaMm2'], 225 * math.pi)
        call('solid.extrude', {'sketchId': sketch_id, 'profileId': profile['profileId'],
                               'distance': 5, 'units': 'mm'})
        before = call('document.inspect')
        body_id = before['evaluation']['bodies'][0]['bodyId']
        near(before['evaluation']['bodies'][0]['volumeMm3'], 1125 * math.pi)

        call('sketch.edit', {'sketchId': sketch_id})
        call('sketch.dimensionUpdate', {'dimensionId': height_id, 'value': 40, 'units': 'mm'})
        edited = call('app.state')['activeSession']['draftSketch']
        circle, center = circle_state(edited, circle_id)
        near(circle['radiusMm'], 20)
        near(center['uMm'], 10)
        near(center['vMm'], 20)
        call('capture.screenshot', {'path': str(out/'three-tangent-circle-edited.png')})
        call('sketch.finish')
        after = call('document.inspect')
        assert after['evaluation']['bodies'][0]['bodyId'] == body_id
        near(after['evaluation']['bodies'][0]['volumeMm3'], 2000 * math.pi)
        call('history.undo')
        assert call('document.inspect')['authored'] == before['authored']
        call('history.redo')
        assert call('document.inspect')['authored'] == after['authored']
        model = out/'three-tangent-circle.alloy'
        call('document.save', {'path': str(model)})
        call('view.home')
        call('capture.screenshot', {'path': str(out/'three-tangent-circle-solid.png')})
        call('document.open', {'path': str(model)})
        reopened = call('document.inspect')
        assert reopened['authored'] == after['authored']
        near(reopened['evaluation']['bodies'][0]['volumeMm3'], 2000 * math.pi)
        (out/'receipt.json').write_text(json.dumps({
            'passed': True, 'sketchId': sketch_id, 'circleId': circle_id,
            'bodyId': body_id, 'lineIds': [bottom['id'], right['id'], top['id']],
            'transcript': transcript}, indent=2)+'\n')
    print('PASS three-line tangent circle, source edit, reopen')


if __name__ == '__main__':
    main()
