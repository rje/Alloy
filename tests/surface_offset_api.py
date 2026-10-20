#!/usr/bin/env python3
"""SRF-029 independent real-app planar Surface Offset geometry/history fixture."""
import argparse
import json
import math
import uuid
from pathlib import Path
from app_harness import application


def near(a, b):
    assert math.isclose(a, b, rel_tol=1e-9, abs_tol=2e-5), (a, b)


def vector(a, b):
    assert len(a) == len(b)
    for x, y in zip(a, b):
        near(x, y)


def reference(face):
    return {key: face[key] for key in ('bodyId', 'producerFeatureId', 'faceId')}


def sheet(body, width, z):
    assert body['type'] == 'surface' and body['valid'] and not body['closed'], body
    assert body['topology']['faces'] == 1 and body['topology']['solids'] == 0, body
    assert body['topology']['edges'] == 4 and body['volumeMm3'] is None, body
    near(body['areaMm2'], width * 30)
    vector(body['boundsMm']['min'], [0, 0, z])
    vector(body['boundsMm']['max'], [width, 30, z])
    face, = body['supportFaces']
    assert face['planar'], face
    vector(face['frame']['normal'], [0, 0, 1])
    near(face['frame']['origin'][2], z)
    corners = [[0, 0, z], [width, 0, z], [width, 30, z], [0, 30, z]]
    expected = list(zip(corners, corners[1:] + corners[:1]))
    deviations = []
    for edge in body['solidEdges']:
        assert edge['available'], edge
        endpoints = [edge['startMm'], edge['endMm']]
        errors = [min(max(math.dist(a, b) for a, b in zip(endpoints, order))
                      for order in (pair, pair[::-1])) for pair in expected]
        index = min(range(len(errors)), key=errors.__getitem__)
        deviations.append(errors[index])
        expected.pop(index)
        vector(edge['midpointMm'], [(a + b) / 2 for a, b in zip(*endpoints)])
    assert not expected and max(deviations) < 2e-5, deviations
    return {'areaMm2': body['areaMm2'], 'zMm': z,
            'normal': face['frame']['normal'], 'maxBoundaryDeviationMm': max(deviations)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    out = Path(args.output).resolve()
    out.mkdir(parents=True, exist_ok=True)
    with application(args.app, out, command_timeout=90) as (call, transcript):
        try:
            inspect = lambda: call('document.inspect')
            draft = lambda: call('app.state')['activeSession']['draftSketch']

            def unchanged(before):
                after = inspect()
                for key in ('authored', 'revision', 'undo', 'evaluation'):
                    assert after[key] == before[key], key

            def body(state, body_id):
                return next(b for b in state['evaluation']['bodies'] if b['bodyId'] == body_id)

            call('window.resize', {'width': 1280, 'height': 900})
            environment = call('app.state')
            assert environment['viewport']['gpu']['hardwareAccelerated'], environment['viewport']
            assert 'surface.offset' in {c['id'] for c in call('commands.list')['commands']}
            plane = call('construction.plane', {'source': {'plane': 'XY'}, 'offset': 20,
                         'units': 'mm', 'name': 'Alloy offset source support'})['planeId']
            call('sketch.create', {'planeId': plane, 'name': 'Alloy 40 x 30 offset boundary'})
            call('sketch.rectangle', {'x1': 0, 'y1': 0, 'x2': 40, 'y2': 30, 'units': 'mm'})
            edges = draft()['parameters']['entities']
            call('sketch.constraint', {'type': 'fixed', 'pointIds': [edges[0]['startPointId']]})
            call('sketch.dimension', {'kind': 'horizontal', 'entityId': edges[0]['id'], 'value': 40, 'units': 'mm'})
            width_dimension = draft()['parameters']['dimensions'][-1]['id']
            call('sketch.dimension', {'kind': 'vertical', 'entityId': edges[1]['id'], 'value': 30, 'units': 'mm'})
            sketch_id = draft()['id']
            call('sketch.finish')
            profile, = inspect()['evaluation']['sketches'][0]['profiles']
            call('surface.patch', {'sketchId': sketch_id, 'profileId': profile['profileId']})
            original = inspect()
            source = original['evaluation']['bodies'][0]
            source_id = source['bodyId']
            source_ref = reference(source['supportFaces'][0])
            measurements = {'source': sheet(source, 40, 20)}
            inputs = {'sourceFace': source_ref, 'distanceMm': 5}
            for invalid in ({'distanceMm': 5}, {**inputs, 'sourceFaces': [source_ref, source_ref]},
                            {**inputs, 'distanceMm': 100001}):
                call('surface.offset', invalid, error='invalid_arguments')
                unchanged(original)
            empty = call('surface.offsetDialog')
            call('command.preview', {'session': empty['id']}, error='selection_required')
            call('command.cancel', {'session': empty['id']})
            unchanged(original)
            session = call('surface.offsetDialog', inputs)
            preview = call('command.preview', {'session': session['id']})
            measurements['positivePreview'] = sheet(preview['geometry'], 40, 25)
            panel = call('app.state')['ui']['activeDialog']
            assert panel['visible'] and panel['command'] == 'surface.offset', panel
            unchanged(original)
            call('view.home')
            call('capture.screenshot', {'path': str(out / 'positive-preview.png')})
            call('command.update', {'session': session['id'], 'inputs': {'distanceMm': -2}})
            measurements['negativePreview'] = sheet(call('command.preview', {'session': session['id']})['geometry'], 40, 18)
            call('capture.screenshot', {'path': str(out / 'negative-preview.png')})
            call('command.update', {'session': session['id'], 'inputs': {'distanceMm': 0}})
            measurements['zeroPreview'] = sheet(call('command.preview', {'session': session['id']})['geometry'], 40, 20)
            call('command.cancel', {'session': session['id']})
            unchanged(original)

            session = call('command.begin', {'command': 'surface.offset', 'inputs': inputs})
            stale_ref = {**source_ref, 'faceId': str(uuid.uuid4())}
            call('command.update', {'session': session['id'], 'inputs': {'sourceFace': stale_ref}}, error='surface_source_unavailable')
            error = transcript[-1]['response']['error']
            assert error['diagnostics']['sourceFace'] == stale_ref, error
            assert error['diagnostics']['faceId'] == stale_ref['faceId'], error
            call('command.commit', {'session': session['id']}, error='surface_source_unavailable')
            error = transcript[-1]['response']['error']
            assert error['diagnostics']['sourceFace'] == stale_ref, error
            assert error['diagnostics']['faceId'] == stale_ref['faceId'], error
            unchanged(original)
            call('command.update', {'session': session['id'], 'inputs': {'sourceFace': source_ref}})
            call('command.commit', {'session': session['id']})
            committed = inspect()
            feature = committed['authored']['features'][-1]
            feature_id, offset_id = feature['id'], feature['bodyId']
            assert feature['type'] == 'surfaceOffset', feature
            for key, value in {**inputs, 'operation': 'newBody', 'isSolid': False}.items():
                assert feature['parameters'][key] == value, feature
            assert body(committed, source_id) == source
            assert committed['authored']['features'][:-1] == original['authored']['features']
            measurements['committedPositive'] = sheet(body(committed, offset_id), 40, 25)
            face_ids = [reference(f) for f in body(committed, offset_id)['supportFaces']]
            edge_ids = [e['edgeId'] for e in body(committed, offset_id)['solidEdges']]
            call('history.undo')
            assert inspect()['authored'] == original['authored']
            call('history.redo')
            assert inspect()['authored'] == committed['authored']

            edit = call('feature.editDialog', {'featureId': feature_id})
            call('command.update', {'session': edit['id'], 'inputs': {'distanceMm': -2}})
            sheet(call('command.preview', {'session': edit['id']})['geometry'], 40, 18)
            call('command.commit', {'session': edit['id']})
            edited = inspect()
            measurements['editedNegative'] = sheet(body(edited, offset_id), 40, 18)
            assert body(edited, source_id) == source
            assert [reference(f) for f in body(edited, offset_id)['supportFaces']] == face_ids
            assert [e['edgeId'] for e in body(edited, offset_id)['solidEdges']] == edge_ids
            for invalid in ({'sourceFace': stale_ref}, {'sourceFace': {**source_ref, 'bodyId': str(uuid.uuid4())}}):
                call('surface.offset', {**inputs, 'featureId': feature_id, **invalid}, error='evaluation_failed')
                unchanged(edited)

            # A second retained Offset proves the output face is a usable downstream source.
            call('surface.offset', {'sourceFace': face_ids[0], 'distanceMm': 3})
            chained = inspect()
            downstream_id = chained['authored']['features'][-1]['bodyId']
            sheet(body(chained, downstream_id), 40, 21)
            call('sketch.edit', {'sketchId': sketch_id})
            call('sketch.dimensionUpdate', {'dimensionId': width_dimension, 'value': 50, 'units': 'mm'})
            call('sketch.finish')
            final = inspect()
            measurements['sourceEdited'] = sheet(body(final, source_id), 50, 20)
            measurements['offsetFollowingSource'] = sheet(body(final, offset_id), 50, 18)
            measurements['downstreamFollowingSource'] = sheet(body(final, downstream_id), 50, 21)
            assert [reference(f) for f in body(final, offset_id)['supportFaces']] == face_ids
            call('history.undo')
            assert inspect()['authored'] == chained['authored']
            call('history.redo')
            assert inspect()['authored'] == final['authored']
            model = out / 'retained-surface-offset.alloy'
            call('document.save', {'path': str(model)})
            call('document.new', {'discard': True})
            call('document.open', {'path': str(model), 'discard': True})
            reopened = inspect()
            assert reopened['authored'] == final['authored']
            measurements['reopened'] = sheet(body(reopened, offset_id), 50, 18)
            sheet(body(reopened, downstream_id), 50, 21)
            call('view.home')
            call('capture.screenshot', {'path': str(out / 'edited-reopened.png')})
            capture_environment = call('app.state')

            # Curved input is unsupported, so refusal is not a tested fold/collapse classifier.
            call('document.new', {'discard': True})
            call('solid.cylinder', {'radius': 10, 'height': 20, 'units': 'mm'})
            curved_state = inspect()
            curved = next(f for f in curved_state['evaluation']['bodies'][0]['supportFaces'] if not f['planar'])
            call('surface.offset', {'sourceFace': reference(curved), 'distanceMm': -10}, error='evaluation_failed')
            unchanged(curved_state)
            report = {'passed': True, 'measurements': measurements, 'sourceFace': source_ref,
                      'featureId': feature_id, 'bodyId': offset_id, 'downstreamBodyId': downstream_id,
                      'environment': environment, 'captureEnvironment': capture_environment, 'final': reopened,
                      'curvedRefusal': reference(curved),
                      'scope': 'Real-entrypoint shared API, analytic planar geometry, authored history and GPU captures. Native actual-hit, Escape, 1x scale, curved offsets and self-intersection/fold classification unverified.'}
            (out / 'result.json').write_text(json.dumps(report, indent=2) + '\n')
        finally:
            (out / 'transcript.json').write_text(json.dumps(transcript, indent=2) + '\n')


if __name__ == '__main__':
    main()
