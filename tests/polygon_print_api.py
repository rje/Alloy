#!/usr/bin/env python3
"""Real-app polygon modes, retained edit, downstream solid, and printable STL."""
import argparse
import json
import math
from pathlib import Path

from app_harness import application
from stl_export_api import inspect_stl


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    out = Path(args.output).resolve()
    out.mkdir(parents=True, exist_ok=True)

    with application(args.app, out) as (call, transcript):
        call('sketch.begin', {'plane': 'XY', 'name': 'Three regular polygons'})
        for mode, x, size in [('inscribed', -40, 10),
                              ('circumscribed', 0, 10),
                              ('edge', 40, 10)]:
            call('sketch.polygon', {'mode': mode, 'x': x, 'y': 0,
                                    'size': size, 'count': 6, 'angleDeg': 0,
                                    'side': 1, 'units': 'mm'})
        draft = call('app.state')['activeSession']['draftSketch']
        groups = draft['parameters']['polygonGroups']
        assert [g['mode'] for g in groups] == ['inscribed', 'circumscribed', 'edge']
        sketch_id = draft['id']
        call('view.orientation', {'name': 'top'})
        call('view.fit')
        call('view.zoom', {'factor': 3})
        annotations = [a for a in call('app.state')['viewport']['sketchAnnotations']
                       if a.get('polygonId')]
        assert len(annotations) == 3, annotations
        assert {a['sizeBasis'] for a in annotations} == {
            'circumradius', 'inradius', 'edgeLength'}
        assert all(a['editable'] and a['driving'] and a['units'] == 'mm'
                   and abs(a['valueMm'] - 10) < 1e-6 for a in annotations)
        call('capture.screenshot', {'path': str(out/'three-polygons.png')})
        call('sketch.finish')
        source = call('document.inspect')
        profiles = source['evaluation']['sketches'][0]['profiles']
        assert len(profiles) == 3, profiles
        expected = [150 * math.sqrt(3), 200 * math.sqrt(3), 150 * math.sqrt(3)]
        assert all(any(abs(p['areaMm2'] - area) < 1e-3 for p in profiles)
                   for area in expected)
        largest = max(profiles, key=lambda p: p['areaMm2'])
        call('solid.extrude', {'sketchId': sketch_id, 'profileId': largest['profileId'],
                               'distance': 5, 'units': 'mm'})
        solid = call('document.inspect')
        assert solid['bodies'] == 1
        assert abs(solid['evaluation']['bodies'][0]['volumeMm3'] - 1000 * math.sqrt(3)) < 1e-2
        body_id = solid['evaluation']['bodies'][0]['bodyId']

        call('sketch.edit', {'sketchId': sketch_id})
        before = call('app.state')['activeSession']['draftSketch']['parameters']
        call('sketch.polygonUpdate', {'polygonId': groups[1]['id'], 'count': 2},
             error='invalid_arguments')
        assert call('app.state')['activeSession']['draftSketch']['parameters'] == before
        call('sketch.polygonUpdate', {'polygonId': groups[1]['id'], 'size': 12, 'units': 'mm'})
        call('capture.screenshot', {'path': str(out/'edited-annotation.png')})
        edited_annotations = [a for a in call('app.state')['viewport']['sketchAnnotations']
                              if a.get('polygonId') == groups[1]['id']]
        assert len(edited_annotations) == 1
        assert edited_annotations[0]['sizeBasis'] == 'inradius'
        assert abs(edited_annotations[0]['valueMm'] - 12) < 1e-6
        call('sketch.finish')
        edited = call('document.inspect')
        assert edited['evaluation']['sketches'][0]['profiles']
        assert abs(edited['evaluation']['bodies'][0]['volumeMm3'] - 1440 * math.sqrt(3)) < 1e-2
        assert edited['evaluation']['bodies'][0]['bodyId'] == body_id
        call('history.undo')
        assert call('document.inspect')['authored'] == solid['authored']
        call('history.redo')
        assert call('document.inspect')['authored'] == edited['authored']

        path = out/'polygons.alloy'
        call('document.save', {'path': str(path)})
        stl = out/'polygon-solid.stl'
        call('document.exportStl', {'path': str(stl), 'bodyId': body_id,
                                    'units': 'mm', 'format': 'binary',
                                    'surfaceDeviationMm': 0.05, 'normalDeviationDeg': 8})
        mesh = inspect_stl(stl, True)
        assert len(mesh['components']) == 1 and mesh['triangles'] >= 20
        call('view.home')
        call('capture.screenshot', {'path': str(out/'extruded-polygon.png')})
        call('document.open', {'path': str(path)})
        reopened = call('document.inspect')
        assert reopened['authored'] == edited['authored']
        assert abs(reopened['evaluation']['bodies'][0]['volumeMm3'] - 1440 * math.sqrt(3)) < 1e-2
        (out/'receipt.json').write_text(json.dumps({'passed': True, 'mesh': mesh,
            'sketchId': sketch_id, 'bodyId': body_id, 'transcript': transcript}, indent=2)+'\n')
    print('PASS polygon modes, edit, downstream extrusion, history, reopen, STL')


if __name__ == '__main__':
    main()
