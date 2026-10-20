#!/usr/bin/env python3
"""Independent real-app retained hollow Pipe and printable STL check."""
import argparse
import json
import math
from pathlib import Path

from app_harness import application
from stl_export_api import inspect_stl


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    out = Path(args.output).resolve()
    out.mkdir(parents=True, exist_ok=True)
    length = 30 + 5 * math.pi

    with application(args.app, out, command_timeout=90) as (call, transcript):
        def inspect():
            return call('document.inspect')

        def geometry(radius, inner, run_length=length):
            body = inspect()['evaluation']['bodies'][0]
            expected = math.pi * (radius * radius - inner * inner) * run_length
            assert body['valid'] and body['closed'] and body['topology']['solids'] == 1
            assert len(body['supportFaces']) == 2, body['supportFaces']
            assert {f['role'] for f in body['supportFaces']} == {'pipe.start', 'pipe.end'}
            assert math.isclose(body['volumeMm3'], expected, abs_tol=1e-4), body['volumeMm3']
            assert math.isclose(body['pathLengthMm'], run_length, abs_tol=1e-6)
            assert math.isclose(body['outerRadiusMm'], radius, abs_tol=1e-8)
            assert math.isclose(body['innerRadiusMm'], inner, abs_tol=1e-8)
            return body

        call('window.resize', {'width': 1280, 'height': 900})
        call('sketch.create', {'plane': 'XY', 'name': 'Pipe centerline'})
        call('sketch.line', {'x1': 0, 'y1': 0, 'x2': 0, 'y2': 30, 'units': 'mm'})
        call('sketch.addArc', {'mode': 'center', 'centerX': 10, 'centerY': 30,
                               'radius': 10, 'startAngleDeg': 180,
                               'sweepAngleDeg': -90, 'units': 'mm'})
        path = call('app.state')['activeSession']['draftSketch']
        entities = path['parameters']['entities']
        assert [e['type'] for e in entities] == ['line', 'arc']
        call('sketch.finish')
        inputs = {'pathSketchId': path['id'], 'pathEntityIds': [e['id'] for e in entities],
                  'pathStartPointId': entities[0]['startPointId'], 'diameterMm': 4,
                  'wallThicknessMm': 1}
        before = inspect()
        session = call('command.begin', {'command': 'solid.pipe', 'inputs': inputs})
        assert session['nativeControlsAvailable']
        preview = call('command.preview', {'session': session['id']})
        assert preview['previewed'] and math.isclose(preview['geometry']['volumeMm3'],
            3 * math.pi * length, abs_tol=1e-4)
        assert inspect()['authored'] == before['authored']
        call('view.home')
        call('capture.screenshot', {'path': str(out/'pipe-preview.png')})
        call('command.cancel', {'session': session['id']})
        assert inspect()['authored'] == before['authored'] and inspect()['undo'] == before['undo']

        session = call('command.begin', {'command': 'solid.pipe', 'inputs': inputs})
        preview = call('command.preview', {'session': session['id']})
        feature_id, body_id = preview['featureId'], preview['bodyId']
        call('command.commit', {'session': session['id']})
        first = inspect()
        body = geometry(2, 1)
        assert body['featureId'] == feature_id and body['bodyId'] == body_id
        assert first['authored']['features'][-1]['parameters']['pathEntityIds'] == inputs['pathEntityIds']
        assert call('app.state')['activeSession'] == {}
        call('history.undo')
        assert inspect()['authored'] == before['authored']
        call('history.redo')
        assert inspect()['authored'] == first['authored']

        bad = inspect()
        call('solid.pipe', {**inputs, 'featureId': feature_id, 'wallThicknessMm': 2},
             error='invalid_arguments')
        assert inspect()['authored'] == bad['authored'] and inspect()['undo'] == bad['undo']
        edit_session = call('feature.editDialog', {'featureId': feature_id})
        assert edit_session['command'] == 'solid.pipe' and edit_session['nativeControlsAvailable']
        call('command.update', {'session': edit_session['id'], 'inputs': {'diameterMm': 6}})
        diameter_preview = call('command.preview', {'session': edit_session['id']})
        assert math.isclose(diameter_preview['geometry']['volumeMm3'],
            5 * math.pi * length, abs_tol=1e-4)
        call('command.commit', {'session': edit_session['id']})
        diameter_edited = geometry(3, 2)
        assert diameter_edited['bodyId'] == body_id
        call('solid.pipe', {**inputs, 'featureId': feature_id, 'diameterMm': 6,
                            'wallThicknessMm': 1.5})
        edited = geometry(3, 1.5)
        assert edited['bodyId'] == body_id
        assert [f['faceId'] for f in edited['supportFaces']] == [f['faceId'] for f in body['supportFaces']]

        source = next(f for f in inspect()['authored']['features'] if f['id'] == path['id'])
        source_params = source['parameters']
        first_point = entities[0]['startPointId']
        next(p for p in source_params['points'] if p['id'] == first_point)['vMm'] = -5
        call('feature.edit', {'featureId': path['id'], 'parameters': {'points': source_params['points']}})
        revised = geometry(3, 1.5, length + 5)
        assert revised['featureId'] == feature_id and revised['bodyId'] == body_id
        model = out/'pipe.alloy'
        call('document.save', {'path': str(model)})
        saved = inspect()
        call('document.open', {'path': str(model)})
        assert inspect()['authored'] == saved['authored']
        geometry(3, 1.5, length + 5)
        call('view.home')
        call('capture.screenshot', {'path': str(out/'pipe-committed.png')})

        mesh_path = out/'pipe.stl'
        result = call('document.exportStl', {'path': str(mesh_path), 'bodyId': body_id,
            'units': 'mm', 'format': 'binary', 'surfaceDeviationMm': .05,
            'normalDeviationDeg': 8})
        mesh = inspect_stl(mesh_path, True)
        assert mesh['triangles'] == result['triangles']
        assert len(mesh['components']) == 1 and mesh['eulerCharacteristic'] == 0, mesh
        assert math.isclose(mesh['signedVolume'], revised['volumeMm3'], rel_tol=.02), mesh
        assert inspect()['authored'] == saved['authored']
        (out/'report.json').write_text(json.dumps({'passed': True, 'source': inputs,
            'first': body, 'diameterEdited': diameter_edited, 'edited': edited,
            'revised': revised, 'export': result,
            'mesh': mesh, 'gpu': call('app.state')['viewport']['gpu']}, indent=2)+'\n')
        (out/'transcript.json').write_text(json.dumps(transcript, indent=2)+'\n')


if __name__ == '__main__':
    main()
