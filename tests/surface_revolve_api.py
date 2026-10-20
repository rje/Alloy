#!/usr/bin/env python3
"""SRF-025 independent real-entrypoint sheet geometry/history verification."""
import argparse
import json
import math
import uuid
from pathlib import Path
from app_harness import application

parser = argparse.ArgumentParser()
parser.add_argument('--app', required=True)
parser.add_argument('--output', required=True)
args = parser.parse_args()
out = Path(args.output).resolve()
out.mkdir(parents=True, exist_ok=True)


def close(actual, expected):
    assert math.isclose(actual, expected, abs_tol=2e-5), (actual, expected)


def sheet(body, radius, height, angle, reverse=False):
    assert body['type'] == 'surface' and body['valid'] and not body['closed'], body
    assert body['topology']['faces'] == 1 and body['topology']['solids'] == 0, body
    assert body['topology']['edges'] == 4, body
    assert body['volumeMm3'] is None, body
    close(body['areaMm2'], radius * height * math.radians(angle))
    theta = math.radians(angle)
    expected_min = [radius * math.cos(theta), -radius * math.sin(theta) if reverse else 0, -10]
    expected_max = [radius, 0 if reverse else radius * math.sin(theta), height - 10]
    for actual, expected in zip(body['boundsMm']['min'], expected_min):
        close(actual, expected)
    for actual, expected in zip(body['boundsMm']['max'], expected_max):
        close(actual, expected)


with application(args.app, out, command_timeout=90) as (call, transcript):
    try:
        def inspect():
            return call('document.inspect')

        def body(state):
            return next(b for b in state['evaluation']['bodies'] if b['bodyId'] == body_id)

        def unchanged(before):
            after = inspect()
            for key in ('authored', 'revision', 'undo', 'evaluation'):
                assert after[key] == before[key], key

        call('window.resize', {'width': 1280, 'height': 900})
        environment = call('app.state')
        call('sketch.create', {'plane': 'XZ', 'name': 'Open radial section — Alloy fixture'})
        call('sketch.line', {'x1': 20, 'y1': -10, 'x2': 20, 'y2': 10, 'units': 'mm'})
        call('sketch.line', {'x1': 0, 'y1': -15, 'x2': 0, 'y2': 15, 'units': 'mm'})
        draft = call('app.state')['activeSession']['draftSketch']
        section, axis = draft['parameters']['entities']
        call('sketch.construction', {'entityIds': [axis['id']], 'construction': True})
        # An unselected crossing line exercises explicit admission and same-draft retry.
        call('sketch.line', {'x1': -5, 'y1': 0, 'x2': 5, 'y2': 0, 'units': 'mm'})
        crossing = call('app.state')['activeSession']['draftSketch']['parameters']['entities'][-1]
        sketch_id = draft['id']
        call('sketch.finish')
        original = inspect()
        inputs = {'sketchId': sketch_id, 'entityId': section['id'], 'axisEntityId': axis['id']}
        session = call('command.begin', {'command': 'surface.revolve', 'inputs': inputs})
        preview = call('command.preview', {'session': session['id']})
        sheet(preview['geometry'], 20, 20, 90)
        panel = call('app.state')['ui']['activeDialog']
        assert panel['visible'] and panel['command'] == 'surface.revolve'
        for key, value in inputs.items():
            assert panel['inputs'][key] == value, panel
        reserved = (preview['featureId'], preview['bodyId'])
        unchanged(original)
        call('command.cancel', {'session': session['id']})
        unchanged(original)

        session = call('command.begin', {'command': 'surface.revolve', 'inputs': inputs})
        call('command.update', {'session': session['id'], 'inputs': {'entityId': crossing['id']}},
             error='evaluation_failed')
        call('command.preview', {'session': session['id']}, error='evaluation_failed')
        call('command.commit', {'session': session['id']}, error='evaluation_failed')
        unchanged(original)
        call('command.update', {'session': session['id'], 'inputs': {'entityId': section['id']}})
        preview = call('command.preview', {'session': session['id']})
        sheet(preview['geometry'], 20, 20, 90)
        call('command.commit', {'session': session['id']})
        committed = inspect()
        feature = committed['authored']['features'][-1]
        feature_id, body_id = feature['id'], feature['bodyId']
        assert feature['type'] == 'surfaceRevolve'
        parameters = feature['parameters']
        for key, value in {**inputs, 'angleDeg': 90, 'reverseDirection': False,
                           'extent': 'partial', 'direction': 'oneSide',
                           'operation': 'newBody', 'isSolid': False}.items():
            assert parameters[key] == value, (key, parameters)
        sheet(body(committed), 20, 20, 90)
        assert body(committed)['sourceSketchId'] == sketch_id
        assert body(committed)['sourceEntityId'] == section['id']
        assert body(committed)['axisEntityId'] == axis['id']
        face_ids = [f['faceId'] for f in body(committed)['supportFaces']]
        edge_ids = [e['edgeId'] for e in body(committed)['solidEdges']]
        call('view.home')
        call('capture.screenshot', {'path': str(out / 'quarter-cylinder.png')})

        edit = call('feature.editDialog', {'featureId': feature_id})
        call('command.update', {'session': edit['id'], 'inputs': {'angleDeg': 45, 'reverseDirection': True}})
        sheet(call('command.preview', {'session': edit['id']})['geometry'], 20, 20, 45, True)
        call('command.commit', {'session': edit['id']})
        edited = inspect()
        sheet(body(edited), 20, 20, 45, True)
        assert [f['faceId'] for f in body(edited)['supportFaces']] == face_ids
        assert [e['edgeId'] for e in body(edited)['solidEdges']] == edge_ids
        call('history.undo')
        assert inspect()['authored'] == committed['authored']
        call('history.redo')
        assert inspect()['authored'] == edited['authored']

        # Missing axis and crossing-axis replacement must preserve the committed model.
        for invalid in ({'axisEntityId': str(uuid.uuid4())}, {'entityId': crossing['id']}):
            before = inspect()
            call('surface.revolve', {**inputs, 'featureId': feature_id, 'angleDeg': 45,
                                    'reverseDirection': True, **invalid}, error='evaluation_failed')
            unchanged(before)
        for angle in (0, 360):
            before = inspect()
            call('surface.revolve', {**inputs, 'featureId': feature_id,
                                    'angleDeg': angle}, error='invalid_arguments')
            unchanged(before)

        call('sketch.edit', {'sketchId': sketch_id})
        call('sketch.movePoint', {'pointId': section['endPointId'], 'x': 20, 'y': 20, 'units': 'mm'})
        call('sketch.finish')
        final = inspect()
        sheet(body(final), 20, 30, 45, True)
        assert body(final)['bodyId'] == body_id
        call('history.undo')
        assert inspect()['authored'] == edited['authored']
        call('history.redo')
        assert inspect()['authored'] == final['authored']
        model = out / 'retained-surface-revolve.alloy'
        call('document.save', {'path': str(model)})
        call('document.new', {'discard': True})
        call('document.open', {'path': str(model), 'discard': True})
        reopened = inspect()
        assert reopened['authored'] == final['authored']
        sheet(body(reopened), 20, 30, 45, True)
        assert [f['faceId'] for f in body(reopened)['supportFaces']] == face_ids
        assert [e['edgeId'] for e in body(reopened)['solidEdges']] == edge_ids
        call('view.home')
        call('capture.screenshot', {'path': str(out / 'edited-reopened-sheet.png')})
        result = {'passed': True, 'featureId': feature_id, 'bodyId': body_id,
                  'initialPreviewReservedIds': reserved, 'sourceSketchId': sketch_id,
                  'sourceEntityId': section['id'], 'axisEntityId': axis['id'],
                  'initialAreaMm2': 200 * math.pi, 'finalAreaMm2': 150 * math.pi,
                  'environment': environment, 'final': reopened,
                  'scope': 'Shared real-app API, analytic sheet geometry and authored history; native controls and matched public visual parity separate.'}
        (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    finally:
        (out / 'transcript.json').write_text(json.dumps(transcript, indent=2) + '\n')
