#!/usr/bin/env python3
"""SRF-026 independent real-app open-section Sweep geometry/history fixture."""
import argparse
import json
import math
import uuid
from pathlib import Path
from app_harness import application


def near(actual, expected):
    assert math.isclose(actual, expected, abs_tol=2e-5), (actual, expected)


def vector(actual, expected):
    assert len(actual) == 3
    for a, e in zip(actual, expected):
        near(a, e)


def boundary(body, role, a, b):
    edge = next(e for e in body['solidEdges'] if e['provenance']['role'] == role)
    actual = [edge['startMm'], edge['endMm']]
    expected = [a, b]
    assert min(max(math.dist(x, y) for x, y in zip(actual, order))
               for order in (expected, expected[::-1])) < 2e-5, (role, actual, expected)


def sheet(body, kind, height=10, reach=40, reverse=False):
    assert body['type'] == 'surface' and body['valid'] and not body['closed'], body
    assert body['topology']['faces'] == 1 and body['topology']['solids'] == 0, body
    assert body['topology']['edges'] == 4 and body['volumeMm3'] is None, body
    half = height / 2
    if kind == 'line':
        near(body['areaMm2'], height * reach)
        vector(body['boundsMm']['min'], [0, 0, -half])
        vector(body['boundsMm']['max'], [reach, 0, half])
        a, b = [0, 0], [reach, 0]
    else:
        angle = math.radians(reach)
        near(body['areaMm2'], height * 20 * angle)
        vector(body['boundsMm']['min'], [20 * math.cos(angle), 0, -half])
        vector(body['boundsMm']['max'], [20, 20 * math.sin(angle), half])
        a, b = [20, 0], [20 * math.cos(angle), 20 * math.sin(angle)]
    if reverse:
        a, b = b, a
    boundary(body, 'start', [*a, -half], [*a, half])
    boundary(body, 'end', [*b, -half], [*b, half])


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--app', required=True)
    p.add_argument('--output', required=True)
    args = p.parse_args()
    out = Path(args.output).resolve()
    out.mkdir(parents=True, exist_ok=True)
    results = []
    with application(args.app, out, command_timeout=90) as (call, transcript):
        try:
            def inspect():
                return call('document.inspect')

            def unchanged(before):
                after = inspect()
                for key in ('authored', 'revision', 'undo', 'evaluation'):
                    assert after[key] == before[key], key

            def draft():
                return call('app.state')['activeSession']['draftSketch']

            call('window.resize', {'width': 1280, 'height': 900})
            environment = call('app.state')
            for kind in ('line', 'arc'):
                call('document.new', {'discard': True})
                call('sketch.create', {'plane': 'YZ' if kind == 'line' else 'XZ', 'name': 'Alloy open section'})
                u = 0 if kind == 'line' else 20
                call('sketch.line', {'x1': u, 'y1': -5, 'x2': u, 'y2': 5, 'units': 'mm'})
                section = draft()
                entity = section['parameters']['entities'][0]
                call('sketch.line', {'x1': u + 3, 'y1': -5, 'x2': u + 3, 'y2': 5, 'units': 'mm'})
                misaligned = draft()['parameters']['entities'][-1]
                call('sketch.finish')
                call('sketch.create', {'plane': 'XY', 'name': 'Alloy single-entity path'})
                if kind == 'line':
                    call('sketch.line', {'x1': 0, 'y1': 0, 'x2': 40, 'y2': 0, 'units': 'mm'})
                else:
                    call('sketch.addArc', {'mode': 'center', 'centerX': 0, 'centerY': 0, 'radius': 20,
                                          'startAngleDeg': 0, 'sweepAngleDeg': 90, 'units': 'mm'})
                path = draft()
                path_entity = path['parameters']['entities'][0]
                if kind == 'arc':
                    for point_id in (path_entity['centerPointId'], path_entity['startPointId']):
                        call('sketch.constraint', {'type': 'fixed', 'pointIds': [point_id]})
                    call('sketch.dimension', {'kind': 'arcSweep', 'entityId': path_entity['id'], 'valueDeg': 90})
                    sweep_dimension = draft()['parameters']['dimensions'][-1]['id']
                endpoint = (40, 0) if kind == 'line' else (0, 20)
                call('sketch.line', {'x1': endpoint[0], 'y1': endpoint[1], 'x2': endpoint[0],
                                     'y2': endpoint[1] + 10, 'units': 'mm'})
                branch = draft()['parameters']['entities'][-1]
                call('sketch.line', {'x1': endpoint[0], 'y1': endpoint[1], 'x2': endpoint[0] + 10,
                                     'y2': endpoint[1] - 10, 'units': 'mm'})
                branch_two = draft()['parameters']['entities'][-1]
                call('sketch.line', {'x1': 60, 'y1': 20, 'x2': 70, 'y2': 20, 'units': 'mm'})
                disconnected = draft()['parameters']['entities'][-1]
                call('sketch.finish')
                # A second authored section at the opposite endpoint permits one
                # atomic retained-reference reversal, without moving source geometry.
                plane = call('construction.plane', {'source': {'plane': 'YZ' if kind == 'line' else 'XZ'},
                             'offset': 40 if kind == 'line' else -20, 'units': 'mm'})
                call('sketch.create', {'planeId': plane['planeId'], 'name': 'Opposite endpoint section'})
                call('sketch.line', {'x1': 0, 'y1': -5, 'x2': 0, 'y2': 5, 'units': 'mm'})
                opposite = draft()
                call('sketch.finish')
                call('feature.visibility', {'featureId': plane['planeId'], 'visible': False})
                call('feature.visibility', {'featureId': opposite['id'], 'visible': False})
                inputs = {'sketchId': section['id'], 'entityId': entity['id'], 'pathSketchId': path['id'],
                          'pathEntityId': path_entity['id'], 'pathStartPointId': path_entity['startPointId']}
                initial = inspect()
                session = call('command.begin', {'command': 'surface.sweep', 'inputs': inputs})
                preview = call('command.preview', {'session': session['id']})
                reach = 40 if kind == 'line' else 90
                sheet(preview['geometry'], kind, reach=reach)
                panel = call('app.state')['ui']['activeDialog']
                assert panel['visible'] and panel['command'] == 'surface.sweep', panel
                for key, value in inputs.items():
                    assert panel['inputs'][key] == value, panel
                call('view.home')
                call('capture.screenshot', {'path': str(out / f'{kind}-preview.png')})
                unchanged(initial)
                call('command.cancel', {'session': session['id']})
                unchanged(initial)
                session = call('command.begin', {'command': 'surface.sweep', 'inputs': inputs})
                call('command.update', {'session': session['id'], 'inputs': {'entityId': misaligned['id']}}, error='evaluation_failed')
                call('command.preview', {'session': session['id']}, error='evaluation_failed')
                call('command.commit', {'session': session['id']}, error='evaluation_failed')
                unchanged(initial)
                call('command.update', {'session': session['id'], 'inputs': {'entityId': entity['id']}})
                preview = call('command.preview', {'session': session['id']})
                call('command.commit', {'session': session['id']})
                committed = inspect()
                feature = committed['authored']['features'][-1]
                feature_id, body_id = feature['id'], feature['bodyId']
                assert feature['type'] == 'surfaceSweep'
                for key, value in {**inputs, 'orientation': 'perpendicular', 'extent': 'full', 'operation': 'newBody', 'isSolid': False}.items():
                    assert feature['parameters'][key] == value, (key, feature)
                assert (feature_id, body_id) == (preview['featureId'], preview['bodyId'])

                def body(state):
                    return next(b for b in state['evaluation']['bodies'] if b['bodyId'] == body_id)

                sheet(body(committed), kind, reach=reach)
                for key, expected in [('sourceSketchId', section['id']), ('sourceEntityId', entity['id']),
                                      ('pathSketchId', path['id']), ('pathEntityId', path_entity['id']),
                                      ('pathStartPointId', path_entity['startPointId'])]:
                    assert body(committed)[key] == expected, (key, body(committed))
                face_ids = [f['faceId'] for f in body(committed)['supportFaces']]
                edge_ids = [e['edgeId'] for e in body(committed)['solidEdges']]
                call('history.undo')
                assert inspect()['authored'] == initial['authored']
                call('history.redo')
                assert inspect()['authored'] == committed['authored']
                edit = call('feature.editDialog', {'featureId': feature_id})
                sheet(call('command.preview', {'session': edit['id']})['geometry'], kind, reach=reach)
                call('command.cancel', {'session': edit['id']})
                assert inspect()['authored'] == committed['authored']
                for invalid in ({'pathEntityId': str(uuid.uuid4())}, {'entityId': misaligned['id']},
                                {'pathStartPointId': str(uuid.uuid4())}):
                    before = inspect()
                    call('surface.sweep', {**inputs, 'featureId': feature_id, **invalid}, error='evaluation_failed')
                    unchanged(before)
                for selection in ([path_entity['id'], branch['id'], branch_two['id']],
                                  [path_entity['id'], disconnected['id']]):
                    before = inspect()
                    call('surface.sweep', {**inputs, 'featureId': feature_id,
                         'pathEntityIds': selection}, error='invalid_arguments')
                    unchanged(before)
                call('surface.sweep', {**inputs, 'featureId': feature_id, 'sketchId': opposite['id'],
                                      'entityId': opposite['parameters']['entities'][0]['id'],
                                      'pathStartPointId': path_entity['endPointId']})
                reversed_state = inspect()
                sheet(body(reversed_state), kind, reach=reach, reverse=True)
                assert body(reversed_state)['bodyId'] == body_id
                call('history.undo')
                assert inspect()['authored'] == committed['authored']
                call('sketch.edit', {'sketchId': section['id']})
                for point_id, v in [(entity['startPointId'], -7), (entity['endPointId'], 7)]:
                    call('sketch.movePoint', {'pointId': point_id, 'x': u, 'y': v, 'units': 'mm'})
                call('sketch.finish')
                section_edited = inspect()
                sheet(body(section_edited), kind, height=14, reach=reach)
                call('sketch.edit', {'sketchId': path['id']})
                if kind == 'line':
                    call('sketch.movePoint', {'pointId': path_entity['endPointId'], 'x': 50, 'y': 0, 'units': 'mm'})
                    reach = 50
                else:
                    call('sketch.dimensionUpdate', {'dimensionId': sweep_dimension, 'valueDeg': 60})
                    reach = 60
                call('sketch.finish')
                final = inspect()
                sheet(body(final), kind, height=14, reach=reach)
                assert [f['faceId'] for f in body(final)['supportFaces']] == face_ids
                assert [e['edgeId'] for e in body(final)['solidEdges']] == edge_ids
                call('history.undo')
                assert inspect()['authored'] == section_edited['authored']
                call('history.redo')
                assert inspect()['authored'] == final['authored']
                model = out / f'{kind}-retained-sheet.alloy'
                call('document.save', {'path': str(model)})
                call('document.new', {'discard': True})
                call('document.open', {'path': str(model), 'discard': True})
                reopened = inspect()
                assert reopened['authored'] == final['authored']
                sheet(body(reopened), kind, height=14, reach=reach)
                call('view.home')
                call('capture.screenshot', {'path': str(out / f'{kind}-reopened.png')})
                results.append({'kind': kind, 'initial': committed, 'final': reopened,
                                'gpu': call('app.state')['viewport'], 'featureId': feature_id, 'bodyId': body_id})
            (out / 'result.json').write_text(json.dumps({'passed': True, 'environment': environment, 'fixtures': results,
                'scope': 'Real-app shared API, analytic open-sheet geometry, endpoint G0, source edits, atomic refusal, history and reopen. Native actual-hit input and matched reference pixels remain separate.'}, indent=2) + '\n')
        finally:
            (out / 'transcript.json').write_text(json.dumps(transcript, indent=2) + '\n')
    print('PASS: line and arc open-sheet Sweep, G0, edits/refusal/history/reopen')


if __name__ == '__main__':
    main()
