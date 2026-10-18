#!/usr/bin/env python3
"""Independent real-app Equal Distance sketch Chamfer and printable STL gate."""
import argparse
import json
import math
from pathlib import Path

from app_harness import application
from stl_export_api import inspect_stl


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
        call('sketch.begin', {'plane': 'XZ', 'name': 'Equal chamfer plate', 'units': 'mm'})
        call('sketch.centerRectangle', {'centerX': 0, 'centerY': 0,
                                         'width': 150, 'height': 75, 'units': 'mm'})
        draft = call('app.state')['activeSession']['draftSketch']
        sketch_id = draft['id']
        lines = [e for e in draft['parameters']['entities'] if e['type'] == 'line'
                 and not e.get('construction')]
        assert len(lines) == 4
        width_id = next(d['id'] for d in draft['parameters']['dimensions']
                        if d['kind'] == 'horizontal')
        height_id = next(d['id'] for d in draft['parameters']['dimensions']
                         if d['kind'] == 'vertical')
        call('sketch.chamfer', {'entityIds': [lines[1]['id'], lines[2]['id']],
                                'value': 10, 'units': 'mm'})
        placed = call('app.state')['activeSession']['draftSketch']
        group = placed['parameters']['chamferGroups'][0]
        assert group['type'] == 'equalDistance'
        assert group['sourceEntityIds'] == [lines[1]['id'], lines[2]['id']]
        chamfer_id, diagonal_id = group['id'], group['lineId']
        near(group['distanceMm'], 10)
        call('capture.screenshot', {'path': str(out/'equal-chamfer-sketch.png')})
        call('sketch.finish')
        profile = call('document.inspect')['evaluation']['sketches'][0]['profiles'][0]
        near(profile['areaMm2'], 11200)
        call('solid.extrude', {'sketchId': sketch_id, 'profileId': profile['profileId'],
                               'distance': 5, 'units': 'mm'})
        before = call('document.inspect')
        body_id = before['evaluation']['bodies'][0]['bodyId']
        near(before['evaluation']['bodies'][0]['volumeMm3'], 56000)

        call('sketch.edit', {'sketchId': sketch_id})
        call('sketch.chamferUpdate', {'chamferId': chamfer_id, 'value': 12, 'units': 'mm'})
        call('sketch.dimensionUpdate', {'dimensionId': width_id, 'value': 160, 'units': 'mm'})
        call('sketch.dimensionUpdate', {'dimensionId': height_id, 'value': 80, 'units': 'mm'})
        edited = call('app.state')['activeSession']['draftSketch']
        group_edited = edited['parameters']['chamferGroups'][0]
        assert group_edited['id'] == chamfer_id and group_edited['lineId'] == diagonal_id
        near(group_edited['distanceMm'], 12)
        call('capture.screenshot', {'path': str(out/'equal-chamfer-edited.png')})
        call('sketch.finish')
        after = call('document.inspect')
        assert after['evaluation']['bodies'][0]['bodyId'] == body_id
        near(after['evaluation']['bodies'][0]['volumeMm3'], 63640)
        call('history.undo')
        assert call('document.inspect')['authored'] == before['authored']
        call('history.redo')
        assert call('document.inspect')['authored'] == after['authored']
        model = out/'equal-chamfer.alloy'
        stl = out/'equal-chamfer.stl'
        call('document.save', {'path': str(model)})
        call('document.exportStl', {'path': str(stl), 'bodyId': body_id,
                                    'units': 'mm', 'format': 'binary',
                                    'surfaceDeviationMm': 0.05, 'normalDeviationDeg': 8})
        mesh = inspect_stl(stl, True)
        assert len(mesh['components']) == 1 and mesh['triangles'] >= 16
        near(mesh['signedVolume'], 63640, 0.1)
        call('view.home')
        call('capture.screenshot', {'path': str(out/'equal-chamfer-solid.png')})
        call('document.open', {'path': str(model)})
        reopened = call('document.inspect')
        assert reopened['authored'] == after['authored']
        near(reopened['evaluation']['bodies'][0]['volumeMm3'], 63640)
        (out/'receipt.json').write_text(json.dumps({
            'passed': True, 'sketchId': sketch_id, 'chamferId': chamfer_id,
            'diagonalId': diagonal_id, 'bodyId': body_id, 'mesh': mesh,
            'transcript': transcript}, indent=2)+'\n')
    print('PASS Equal Distance sketch Chamfer, source edit, reopen and STL')


if __name__ == '__main__':
    main()
