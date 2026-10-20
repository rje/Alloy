#!/usr/bin/env python3
"""Independent real-app U-channel Rib, retained edit and printable STL check."""
import argparse
import json
import math
from pathlib import Path

from app_harness import application
from alloyctl import request
from stl_export_api import inspect_stl


def near(actual, expected, tolerance=1e-6):
    assert math.isclose(actual, expected, abs_tol=tolerance), (actual, expected)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    out = Path(args.output).resolve()
    out.mkdir(parents=True, exist_ok=True)
    results = {}
    with application(args.app, out, command_timeout=90) as (call, transcript):
        def inspect():
            return call('document.inspect')

        def rib_body():
            state = inspect()
            body, = state['evaluation']['bodies']
            assert body['bodyId'] == target and body['valid'] and body['closed']
            assert body['topology']['solids'] == 1
            return body

        def refused(command, parameters):
            before = inspect()
            response = request(call.endpoint, command, parameters, timeout=90,
                               transport=call.transport)
            transcript.append({'command': command, 'args': parameters, 'response': response})
            assert not response['ok'], response
            assert inspect()['authored'] == before['authored']
            assert inspect()['undo'] == before['undo']
            return response['error']

        call('window.resize', {'width': 1280, 'height': 900})
        assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
        # The walls overlap the base by 1 mm. Their union is still the exact
        # U-channel x[0,40], y[0,30], base z[0,2], walls z[2,22].
        for name, depth, height, y, z in (
            ('U base', 30, 2, 0, 0),
            ('Near wall', 2, 21, 0, 1),
        ):
            call('solid.box', {'name': name, 'width': 40, 'depth': depth,
                'height': height, 'x': 0, 'y': y, 'z': z, 'units': 'mm'})
        base, near_wall = [f['bodyId'] for f in inspect()['authored']['features'][-2:]]
        target = base
        call('solid.combine', {'targetBodyId': target, 'toolBodyIds': [near_wall],
            'operation': 'join', 'keepTools': False})
        call('solid.box', {'name': 'Far wall', 'width': 40, 'depth': 2,
            'height': 21, 'x': 0, 'y': 28, 'z': 1, 'units': 'mm'})
        far_feature = inspect()['authored']['features'][-1]
        call('solid.combine', {'targetBodyId': target,
            'toolBodyIds': [far_feature['bodyId']], 'operation': 'join',
            'keepTools': False})
        near(rib_body()['volumeMm3'], 5600)
        plane = call('construction.plane', {'source': {'plane': 'XY'},
            'offset': 4, 'units': 'mm'})['planeId']
        call('sketch.create', {'planeId': plane, 'name': 'Rib centerline'})
        call('sketch.line', {'x1': 2, 'y1': 1, 'x2': 38, 'y2': 1, 'units': 'mm'})
        draft = call('app.state')['activeSession']['draftSketch']
        sketch_id = draft['id']
        entity_id = draft['parameters']['entities'][0]['id']
        call('sketch.finish')
        before = inspect()
        source_path = out/'u-channel-before-rib.alloy'
        call('document.save', {'path': str(source_path)})
        before = inspect()
        source_bytes = source_path.read_bytes()
        inputs = {'sketchId': sketch_id, 'sourceEntityId': entity_id,
                  'targetBodyId': target, 'thicknessMm': 2,
                  'flip': False, 'extent': 'toNext', 'direction': 'symmetric',
                  'operation': 'join'}
        session = call('command.begin', {'command': 'solid.rib', 'inputs': inputs})
        preview = call('command.preview', {'session': session['id']})
        near(preview['geometry']['volumeMm3'], 7472)
        assert inspect()['authored'] == before['authored']
        call('view.home')
        call('capture.screenshot', {'path': str(out/'rib-preview.png')})
        call('command.cancel', {'session': session['id']})
        assert inspect()['authored'] == before['authored']
        assert inspect()['undo'] == before['undo']
        assert source_path.read_bytes() == source_bytes
        session = call('command.begin', {'command': 'solid.rib', 'inputs': inputs})
        call('command.commit', {'session': session['id']})
        accepted = inspect()
        rib_feature = accepted['authored']['features'][-1]
        assert rib_feature['type'] == 'rib'
        near(rib_body()['volumeMm3'], 7472)
        results['initial'] = {'featureId': rib_feature['id'],
            'bodyId': target, 'volumeMm3': rib_body()['volumeMm3']}
        call('capture.screenshot', {'path': str(out/'rib-committed.png')})
        call('view.orientation', {'name': 'right'})
        call('view.fit')
        call('capture.screenshot', {'path': str(out/'rib-right-cross-section.png')})
        call('history.undo')
        assert inspect()['authored'] == before['authored']
        call('history.redo')
        assert inspect()['authored'] == accepted['authored']
        refused('solid.rib', {**inputs, 'featureId': rib_feature['id'], 'thicknessMm': -1})
        call('solid.rib', {**inputs, 'featureId': rib_feature['id'], 'thicknessMm': 3})
        near(rib_body()['volumeMm3'], 8408)
        call('history.undo')
        near(rib_body()['volumeMm3'], 7472)
        call('history.redo')
        near(rib_body()['volumeMm3'], 8408)
        call('solid.rib', {**inputs, 'featureId': rib_feature['id'], 'thicknessMm': 2})
        near(rib_body()['volumeMm3'], 7472)
        call('solid.box', {'featureId': far_feature['id'], 'name': 'Far wall',
            'width': 40, 'depth': 2, 'height': 21, 'x': 0, 'y': 26,
            'z': 1, 'units': 'mm'})
        near(rib_body()['volumeMm3'], 7328)
        results['far-wall-edited'] = {'volumeMm3': rib_body()['volumeMm3']}
        pinned = next(f for f in inspect()['authored']['features']
                      if f['id'] == rib_feature['id'])['parameters']['supportFaceIds']
        call('construction.plane', {'featureId': plane, 'source': {'plane': 'XY'},
            'offset': 5, 'units': 'mm'})
        near(rib_body()['volumeMm3'], 7328)
        moved = inspect()
        resolved = next(p for p in moved['evaluation']['constructionPlanes']
                        if p['featureId'] == plane)
        near(resolved['frame']['origin'][2], 5)
        assert next(f for f in moved['authored']['features']
                    if f['id'] == rib_feature['id'])['parameters']['supportFaceIds'] == pinned
        results['source-plane-edited'] = {'volumeMm3': rib_body()['volumeMm3'],
                                          'sketchHeightMm': 5,
                                          'supportFaceIds': pinned}
        stl = out/'rib-printable.stl'
        call('document.exportStl', {'bodyId': target, 'path': str(stl),
            'format': 'binary', 'units': 'mm', 'surfaceDeviationMm': .05,
            'normalDeviationDeg': 5})
        mesh = inspect_stl(stl, True)
        assert len(mesh['components']) == 1
        near(mesh['signedVolume'], 7328, .03)
        for actual, expected in zip(mesh['bounds'], [[0,0,0],[40,30,22]]):
            assert all(abs(x-y) < .001 for x,y in zip(actual,expected)), mesh
        results['stl'] = {k:v for k,v in mesh.items() if k!='facetNormals'}
        final = inspect()
        model = out/'rib-edited.alloy'
        call('document.save', {'path': str(model)})
        call('document.new', {'discard': True})
        call('document.open', {'path': str(model)})
        assert inspect()['authored'] == final['authored']
        near(rib_body()['volumeMm3'], 7328)
        call('view.orientation', {'name': 'right'})
        call('view.fit')
        call('capture.screenshot', {'path': str(out/'rib-reopened.png')})
        results['gpu'] = call('app.state')['viewport']['gpu']
        (out/'report.json').write_text(json.dumps(results, indent=2)+'\n')
        (out/'transcript.json').write_text(json.dumps(transcript, indent=2)+'\n')
    print('PASS retained Rib U-channel geometry, edit/refusal/history/reopen and independent watertight STL')


if __name__ == '__main__':
    main()
