#!/usr/bin/env python3
"""SRF-030 independent retained Normal Ruled real-entrypoint fixture."""
import argparse
import json
import math
from pathlib import Path
import uuid
from app_harness import application


def near(a, b):
    assert math.isclose(a, b, rel_tol=1e-9, abs_tol=2e-5), (a, b)


def vector(a, b):
    assert len(a) == len(b)
    for x, y in zip(a, b):
        near(x, y)


def flange(body, width, angle):
    # Expected values derive from the independent face-normal rotation contract,
    # not the feature's self-reported direction/length summary.
    y, z = 20*math.sin(math.radians(angle)), 20*math.cos(math.radians(angle))
    assert body['type'] == 'surface' and body['valid'] and not body['closed'], body
    assert all(body['topology'][key] == value for key, value in
               {'faces': 1, 'edges': 4, 'vertices': 4, 'solids': 0}.items()), body
    assert body['volumeMm3'] is None, body
    near(body['areaMm2'], width*20)
    vector(body['boundsMm']['min'], [0, min(0, y), min(0, z)])
    vector(body['boundsMm']['max'], [width, max(0, y), max(0, z)])
    corners = [[0, 0, 0], [width, 0, 0], [width, y, z], [0, y, z]]
    remaining = list(zip(corners, corners[1:]+corners[:1]))
    deviations = []
    for edge in body['solidEdges']:
        assert edge['available'], edge
        ends = [edge['startMm'], edge['endMm']]
        errors = [min(max(math.dist(a, b) for a, b in zip(ends, order))
                      for order in (pair, pair[::-1])) for pair in remaining]
        index = min(range(len(errors)), key=errors.__getitem__)
        deviations.append(errors[index])
        remaining.pop(index)
        vector(edge['midpointMm'], [(a+b)/2 for a, b in zip(*ends)])
    assert not remaining and max(deviations) < 2e-5, deviations
    return {'widthMm': width, 'angleDeg': angle, 'areaMm2': body['areaMm2'],
            'farEndpointsMm': corners[2:], 'maxBoundaryDeviationMm': max(deviations)}


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
            def body(state, bid):
                return next(b for b in state['evaluation']['bodies'] if b['bodyId'] == bid)
            def unchanged(before):
                after = inspect()
                for key in ('authored', 'revision', 'undo', 'evaluation'):
                    assert after[key] == before[key], key
            def ids(b):
                return ([f['faceId'] for f in b['supportFaces']], [e['edgeId'] for e in b['solidEdges']])
            call('window.resize', {'width': 1280, 'height': 900})
            environment = call('app.state')
            assert environment['viewport']['gpu']['hardwareAccelerated'], environment
            assert 'surface.ruled' in {c['id'] for c in call('commands.list')['commands']}
            call('sketch.create', {'plane': 'XY', 'name': 'Alloy 40 x 30 retained Ruled source'})
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
            source, = original['evaluation']['bodies']
            source_id = source['bodyId']
            face, = source['supportFaces']
            vector(face['frame']['normal'], [0, 0, 1])
            edge, = [e for e in source['solidEdges'] if all(abs(e[k][1]) < 1e-6 for k in ('startMm', 'endMm'))]
            vector(edge['startMm'], [0, 0, 0])
            vector(edge['endMm'], [40, 0, 0])
            source_ref = {k: face[k] for k in ('bodyId', 'producerFeatureId', 'faceId')}
            source_ref['edgeId'] = edge['edgeId']
            inputs = {'sourceEdge': source_ref, 'ruleType': 'normal', 'distanceMm': 20, 'angleDeg': 30}
            call('view.home')
            session = call('surface.ruledDialog')
            call('command.preview', {'session': session['id']}, error='selection_required')
            pixel = call('view.projectSupportEdge', {'bodyId': source_id, 'edgeId': edge['edgeId']})
            call('selection.pick', {'x': pixel['x'], 'y': pixel['y']})
            assert call('app.state')['activeSession']['inputs']['sourceEdge'] == source_ref
            measurements = {'defaultPreview': flange(call('command.preview', {'session': session['id']})['geometry'], 40, 0)}
            call('command.update', {'session': session['id'], 'inputs': {'angleDeg': 30}})
            measurements['preview30'] = flange(call('command.preview', {'session': session['id']})['geometry'], 40, 30)
            panel = call('app.state')['ui']['activeDialog']
            assert panel['visible'] and panel['command'] == 'surface.ruled', panel
            call('view.home')
            call('capture.screenshot', {'path': str(out/'normal30-preview.png')})
            call('command.cancel', {'session': session['id']})
            unchanged(original)
            call('surface.ruled', inputs)
            committed = inspect()
            feature = committed['authored']['features'][-1]
            feature_id, flange_id = feature['id'], feature['bodyId']
            assert feature['type'] == 'surfaceRuled', feature
            for key, value in {**inputs, 'operation': 'newBody', 'isSolid': False}.items():
                assert feature['parameters'][key] == value, feature
            assert body(committed, source_id) == source
            stable_ids = ids(body(committed, flange_id))
            for angle in (30, 0, -30, 90, 120, 210):
                call('surface.ruled', {'featureId': feature_id, 'angleDeg': angle})
                state = inspect()
                measurements[str(angle)] = flange(body(state, flange_id), 40, angle)
                assert ids(body(state, flange_id)) == stable_ids
                assert body(state, source_id) == source
                call('document.save', {'path': str(out/f'normal-{angle}.alloy')})
            call('surface.ruled', {'featureId': feature_id, 'angleDeg': 30})
            baseline = inspect()
            failures = []
            # Invalid creation/edit must leave committed shape/history/revision/undo intact.
            for invalid, code in (({'distanceMm': 0}, 'invalid_arguments'),
                                  ({'distanceMm': -1}, 'invalid_arguments'),
                                  ({'ruleType': 'tangent'}, 'invalid_arguments'),
                                  ({'sourceEdge': {**source_ref, 'edgeId': str(uuid.uuid4())}}, 'evaluation_failed')):
                call('surface.ruled', {'featureId': feature_id, **invalid}, error=code)
                failures.append(transcript[-1]['response']['error'])
                unchanged(baseline)
            session = call('feature.editDialog', {'featureId': feature_id})
            stale = {**source_ref, 'edgeId': str(uuid.uuid4())}
            call('command.update', {'session': session['id'], 'inputs': {'sourceEdge': stale}}, error='surface_source_unavailable')
            call('command.commit', {'session': session['id']}, error='surface_source_unavailable')
            unchanged(baseline)
            call('command.update', {'session': session['id'], 'inputs': {'sourceEdge': source_ref, 'angleDeg': -30}})
            flange(call('command.preview', {'session': session['id']})['geometry'], 40, -30)
            call('command.commit', {'session': session['id']})
            recovered = inspect()
            call('history.undo')
            assert inspect()['authored'] == baseline['authored']
            call('history.redo')
            assert inspect()['authored'] == recovered['authored']
            call('surface.ruled', {'featureId': feature_id, 'angleDeg': 30})
            before_edit = inspect()
            call('sketch.edit', {'sketchId': sketch_id})
            call('sketch.dimensionUpdate', {'dimensionId': width_dimension, 'value': 45, 'units': 'mm'})
            call('sketch.finish')
            final = inspect()
            measurements['sourceWidthEdit'] = flange(body(final, flange_id), 45, 30)
            assert ids(body(final, flange_id)) == stable_ids
            assert ids(body(final, source_id)) == ids(source)
            assert final['authored']['features'][-1]['parameters']['sourceEdge'] == source_ref
            call('history.undo')
            assert inspect()['authored'] == before_edit['authored']
            call('history.redo')
            assert inspect()['authored'] == final['authored']
            model = out/'retained-normal-ruled.alloy'
            call('document.save', {'path': str(model)})
            call('document.new', {'discard': True})
            call('document.open', {'path': str(model), 'discard': True})
            reopened = inspect()
            assert reopened['authored'] == final['authored']
            measurements['reopened'] = flange(body(reopened, flange_id), 45, 30)
            call('view.home')
            call('capture.screenshot', {'path': str(out/'edited-reopened.png')})
            capture_environment = call('app.state')
            # A solid edge has two incident faces: explicit face choice must not
            # silently bypass the bounded unique-boundary contract.
            call('document.new', {'discard': True})
            call('solid.box', {'width': 40, 'depth': 30, 'height': 10, 'units': 'mm'})
            solid_state = inspect()
            solid_face = solid_state['evaluation']['bodies'][0]['supportFaces'][0]
            shared_ref = {k: solid_face[k] for k in ('bodyId', 'producerFeatureId', 'faceId')}
            shared_ref['edgeId'] = solid_face['edges'][0]['edgeId']
            call('surface.ruled', {**inputs, 'sourceEdge': shared_ref}, error='evaluation_failed')
            failures.append(transcript[-1]['response']['error'])
            unchanged(solid_state)
            report = {'passed': True, 'measurements': measurements, 'sourceEdge': source_ref,
                      'featureId': feature_id, 'bodyId': flange_id, 'environment': environment,
                      'captureEnvironment': capture_environment, 'failures': failures, 'final': reopened,
                      'scope': 'Independent shared real-app API, analytic boundaries/area/bounds, retained identity/history/reopen and GPU captures. Physical native input and matched public visual parity remain unverified; positive angle sign is Alloy policy.'}
            (out/'result.json').write_text(json.dumps(report, indent=2)+'\n')
        finally:
            (out/'transcript.json').write_text(json.dumps(transcript, indent=2)+'\n')


if __name__ == '__main__':
    main()
