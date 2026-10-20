#!/usr/bin/env python3
"""SRF-027 independent real-app one-loop planar Patch geometry/history fixture."""
import argparse
import json
import math
import uuid
from pathlib import Path
from app_harness import application


def near(a, b):
    assert math.isclose(a, b, abs_tol=2e-5), (a, b)


def vector(a, b):
    for x, y in zip(a, b):
        near(x, y)


def sheet(body, width):
    assert body['type'] == 'surface' and body['valid'] and not body['closed'], body
    assert body['topology']['faces'] == 1 and body['topology']['solids'] == 0, body
    assert body['boundaryLoopCount'] == 1 and body['closedBoundary'], body
    assert body['topology']['edges'] == 4, body
    assert body['volumeMm3'] is None, body
    near(body['areaMm2'], width * 30)
    vector(body['boundsMm']['min'], [0, 0, 20])
    vector(body['boundsMm']['max'], [width, 30, 20])
    face, = body['supportFaces']
    assert face['planar'], face
    near(face['frame']['origin'][2], 20)
    normal = face['frame']['normal']
    near(normal[0], 0)
    near(normal[1], 0)
    near(abs(normal[2]), 1)
    corners = [[0, 0, 20], [width, 0, 20], [width, 30, 20], [0, 30, 20]]
    expected = list(zip(corners, corners[1:] + corners[:1]))
    actual = body['solidEdges']
    assert len(actual) == 4 and all(e['available'] for e in actual)
    # Each OCCT boundary must cover exactly one source segment, not merely share bounds.
    deviations = []
    for edge in actual:
        endpoints = [edge['startMm'], edge['endMm']]
        errors = [min(max(math.dist(a, b) for a, b in zip(endpoints, order))
                      for order in (pair, pair[::-1])) for pair in expected]
        index = min(range(len(errors)), key=errors.__getitem__)
        deviations.append(errors[index])
        expected.pop(index)
        vector(edge['midpointMm'], [(a + b) / 2 for a, b in zip(*endpoints)])
    assert max(deviations) < 2e-5, deviations
    perimeter = sum(math.dist(e['startMm'], e['endMm']) for e in actual)
    near(perimeter, 2 * (width + 30))
    near(body['perimeterMm'], perimeter)
    return {'areaMm2': body['areaMm2'], 'perimeterMm': perimeter,
            'normal': normal, 'maxSourceBoundaryDeviationMm': max(deviations)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    out = Path(args.output).resolve()
    out.mkdir(parents=True, exist_ok=True)
    transcript = []
    with application(args.app, out, command_timeout=90) as (call, transcript):
        try:
            def inspect():
                return call('document.inspect')

            def draft():
                return call('app.state')['activeSession']['draftSketch']

            def unchanged(before):
                after = inspect()
                for key in ('authored', 'revision', 'undo', 'evaluation'):
                    assert after[key] == before[key], key

            def result_body(state):
                return next(b for b in state['evaluation']['bodies'] if b['bodyId'] == body_id)

            call('window.resize', {'width': 1280, 'height': 900})
            environment = call('app.state')
            assert environment['viewport']['gpu']['hardwareAccelerated'], environment['viewport']
            assert 'surface.patch' in {c['id'] for c in call('commands.list')['commands']}
            plane = call('construction.plane', {'source': {'plane': 'XY'}, 'offset': 20,
                         'units': 'mm', 'name': 'Alloy planar cap support'})['planeId']
            call('sketch.create', {'planeId': plane, 'name': 'Alloy 40 x 30 cap boundary'})
            call('sketch.rectangle', {'x1': 0, 'y1': 0, 'x2': 40, 'y2': 30, 'units': 'mm'})
            edges = draft()['parameters']['entities']
            call('sketch.constraint', {'type': 'fixed', 'pointIds': [edges[0]['startPointId']]})
            call('sketch.dimension', {'kind': 'horizontal', 'entityId': edges[0]['id'], 'value': 40, 'units': 'mm'})
            width_dimension = draft()['parameters']['dimensions'][-1]['id']
            call('sketch.dimension', {'kind': 'vertical', 'entityId': edges[1]['id'], 'value': 30, 'units': 'mm'})
            sketch_id = draft()['id']
            source_entities = [e['id'] for e in edges]
            call('sketch.finish')
            original = inspect()
            profile, = original['evaluation']['sketches'][0]['profiles']
            profile_id = profile['profileId']
            inputs = {'sketchId': sketch_id, 'profileId': profile_id}
            for invalid in ({'sketchId': sketch_id}, {**inputs, 'profileIds': [profile_id]}):
                call('surface.patch', invalid, error='invalid_arguments')
                unchanged(original)
            empty = call('surface.patchDialog')
            call('command.preview', {'session': empty['id']}, error='selection_required')
            call('command.cancel', {'session': empty['id']})
            unchanged(original)
            session = call('surface.patchDialog', inputs)
            preview = call('command.preview', {'session': session['id']})
            measurements = {'initial': sheet(preview['geometry'], 40)}
            reserved = (preview['featureId'], preview['bodyId'])
            panel = call('app.state')['ui']['activeDialog']
            assert panel['visible'] and panel['command'] == 'surface.patch', panel
            unchanged(original)
            call('view.home')
            call('capture.screenshot', {'path': str(out / 'patch-preview.png')})
            call('command.cancel', {'session': session['id']})
            unchanged(original)
            session = call('command.begin', {'command': 'surface.patch', 'inputs': inputs})
            call('command.update', {'session': session['id'], 'inputs': {'profileId': str(uuid.uuid4())}}, error='evaluation_failed')
            call('command.commit', {'session': session['id']}, error='evaluation_failed')
            unchanged(original)
            call('command.update', {'session': session['id'], 'inputs': {'profileId': profile_id}})
            call('command.commit', {'session': session['id']})
            committed = inspect()
            feature = committed['authored']['features'][-1]
            assert feature['type'] == 'surfacePatch', feature
            feature_id, body_id = feature['id'], feature['bodyId']
            for key, value in {**inputs, 'operation': 'newBody', 'isSolid': False}.items():
                assert feature['parameters'][key] == value, feature
            measurements['committed'] = sheet(result_body(committed), 40)
            face_ids = [f['faceId'] for f in result_body(committed)['supportFaces']]
            edge_ids = [e['edgeId'] for e in result_body(committed)['solidEdges']]
            call('capture.screenshot', {'path': str(out / 'patch-committed.png')})
            call('history.undo')
            assert inspect()['authored'] == original['authored']
            call('history.redo')
            assert inspect()['authored'] == committed['authored']
            edit = call('feature.editDialog', {'featureId': feature_id})
            sheet(call('command.preview', {'session': edit['id']})['geometry'], 40)
            call('command.cancel', {'session': edit['id']})
            call('sketch.edit', {'sketchId': sketch_id})
            call('sketch.dimensionUpdate', {'dimensionId': width_dimension, 'value': 50, 'units': 'mm'})
            call('sketch.finish')
            final = inspect()
            measurements['edited'] = sheet(result_body(final), 50)
            assert final['evaluation']['sketches'][0]['profiles'][0]['profileId'] == profile_id
            assert [e['id'] for e in final['authored']['features'][1]['parameters']['entities']] == source_entities
            assert [f['faceId'] for f in result_body(final)['supportFaces']] == face_ids
            assert [e['edgeId'] for e in result_body(final)['solidEdges']] == edge_ids
            call('history.undo')
            assert inspect()['authored'] == committed['authored']
            call('history.redo')
            assert inspect()['authored'] == final['authored']
            model = out / 'retained-planar-cap.alloy'
            call('document.save', {'path': str(model)})
            call('document.new', {'discard': True})
            call('document.open', {'path': str(model), 'discard': True})
            reopened = inspect()
            assert reopened['authored'] == final['authored']
            measurements['reopened'] = sheet(result_body(reopened), 50)
            assert [f['faceId'] for f in result_body(reopened)['supportFaces']] == face_ids
            assert [e['edgeId'] for e in result_body(reopened)['solidEdges']] == edge_ids
            call('view.home')
            call('capture.screenshot', {'path': str(out / 'patch-edited-reopened.png')})
            capture_environment = call('app.state')
            refusals = []
            # These source fixtures remain valid sketches even when no valid closed profile exists.
            for kind in ('open', 'selfIntersecting', 'disconnected'):
                call('document.new', {'discard': True})
                call('sketch.create', {'plane': 'XY', 'name': 'Alloy invalid boundary ' + kind})
                if kind == 'disconnected':
                    for x in (0, 60):
                        call('sketch.rectangle', {'x1': x, 'y1': 0, 'x2': x + 40, 'y2': 30, 'units': 'mm'})
                else:
                    segments = ([(0, 0, 40, 0), (40, 0, 40, 30), (40, 30, 0, 30)] if kind == 'open'
                                else [(0, 0, 40, 30), (40, 30, 0, 30), (0, 30, 40, 0), (40, 0, 0, 0)])
                    for coords in segments:
                        call('sketch.line', {**dict(zip(('x1', 'y1', 'x2', 'y2'), coords)), 'units': 'mm'})
                sid = draft()['id']
                call('sketch.finish')
                before = inspect()
                profiles = before['evaluation']['sketches'][0]['profiles']
                pid = profiles[0]['profileId'] if profiles else str(uuid.uuid4())
                call('surface.patch', {'sketchId': sid, 'profileId': pid}, error='evaluation_failed')
                unchanged(before)
                refusals.append({'kind': kind, 'evaluatedProfileCount': len(profiles)})
            report = {'passed': True, 'measurements': measurements, 'featureId': feature_id,
                      'bodyId': body_id, 'sketchId': sketch_id, 'profileId': profile_id,
                      'reservedPreviewIds': reserved, 'faceIds': face_ids, 'edgeIds': edge_ids,
                      'refusals': refusals, 'environment': environment, 'captureEnvironment': capture_environment,
                      'final': reopened, 'scope': 'Real-app shared API and analytic geometry/history; native actual-hit and matched reference visuals separate.'}
            (out / 'result.json').write_text(json.dumps(report, indent=2) + '\n')
        finally:
            (out / 'transcript.json').write_text(json.dumps(transcript, indent=2) + '\n')


if __name__ == '__main__':
    main()
