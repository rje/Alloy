#!/usr/bin/env python3
"""SRF-031 independent retained planar Extend real-entrypoint fixture."""
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


def sheet(body, width, distance):
    assert body['type'] == 'surface' and body['valid'] and not body['closed'], body
    assert all(body['topology'][key] == value for key, value in
               {'faces': 1, 'edges': 4, 'vertices': 4, 'solids': 0}.items()), body
    assert body['volumeMm3'] is None, body
    near(body['areaMm2'], width*(30+distance))
    vector(body['boundsMm']['min'], [0, -distance, 0])
    vector(body['boundsMm']['max'], [width, 30, 0])
    face, = body['supportFaces']
    assert face['planar']
    vector(face['frame']['normal'], [0, 0, 1])
    corners = [[0, -distance, 0], [width, -distance, 0], [width, 30, 0], [0, 30, 0]]
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
    return {'widthMm': width, 'distanceMm': distance, 'areaMm2': body['areaMm2'],
            'boundsMm': body['boundsMm'], 'maxBoundaryDeviationMm': max(deviations)}


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
            def result_body(state):
                body, = state['evaluation']['bodies']
                assert body['bodyId'] == source_id
                return body
            def unchanged(before):
                after = inspect()
                for key in ('authored', 'revision', 'undo', 'evaluation'):
                    assert after[key] == before[key], key
            def ids(body):
                return (body['bodyId'], [(f['producerFeatureId'], f['faceId']) for f in body['supportFaces']],
                        [e['edgeId'] for e in body['solidEdges']])
            call('window.resize', {'width': 1280, 'height': 800})
            environment = call('app.state')
            assert environment['viewport']['gpu']['hardwareAccelerated'], environment
            assert 'surface.extend' in {c['id'] for c in call('commands.list')['commands']}
            call('sketch.create', {'plane': 'XY', 'name': 'Alloy retained 40 x 30 Extend source'})
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
            stable_ids = ids(source)
            measurements = {'source': sheet(source, 40, 0)}
            face, = source['supportFaces']
            edge, = [e for e in source['solidEdges'] if all(abs(e[k][1]) < 1e-6 for k in ('startMm', 'endMm'))]
            source_ref = {k: face[k] for k in ('bodyId', 'producerFeatureId', 'faceId')}
            source_ref['edgeId'] = edge['edgeId']
            inputs = {'sourceEdge': source_ref, 'extendType': 'natural', 'edgeAlignment': 'freeEdges',
                      'chaining': False, 'distanceMm': 20}
            call('document.save', {'path': str(out/'source-40.alloy')})
            original = inspect()
            call('view.home')
            session = call('surface.extendDialog')
            call('command.preview', {'session': session['id']}, error='selection_required')
            pixel = call('view.projectSupportEdge', {'bodyId': source_id, 'edgeId': edge['edgeId']})
            call('selection.pick', {'x': pixel['x'], 'y': pixel['y']})
            assert call('app.state')['activeSession']['inputs']['sourceEdge'] == source_ref
            measurements['preview20'] = sheet(call('command.preview', {'session': session['id']})['geometry'], 40, 20)
            panel = call('app.state')['ui']['activeDialog']
            assert panel['visible'] and panel['command'] == 'surface.extend', panel
            call('capture.screenshot', {'path': str(out/'extend-preview.png')})
            call('command.cancel', {'session': session['id']})
            unchanged(original)
            call('surface.extend', inputs)
            committed = inspect()
            feature = committed['authored']['features'][-1]
            feature_id = feature['id']
            assert feature['type'] == 'surfaceExtend', feature
            for key, value in inputs.items():
                assert feature['parameters'][key] == value, feature
            assert ids(result_body(committed)) == stable_ids
            measurements['committed20'] = sheet(result_body(committed), 40, 20)
            call('document.save', {'path': str(out/'extended-40-20.alloy')})
            call('capture.screenshot', {'path': str(out/'extend-committed.png')})
            call('timeline.rollbackBefore', {'featureId': feature_id})
            prior_stage = inspect()
            assert result_body(prior_stage) == source
            call('timeline.returnToLatest')
            assert inspect()['authored'] == committed['authored']
            call('history.undo')
            assert inspect()['authored'] == original['authored']
            sheet(result_body(inspect()), 40, 0)
            call('history.redo')
            assert inspect()['authored'] == committed['authored']
            baseline = inspect()
            failures = []
            for invalid, code in (({'distanceMm': 0}, 'invalid_arguments'),
                                  ({'distanceMm': -1}, 'invalid_arguments'),
                                  ({'extendType': 'tangent'}, 'invalid_arguments'),
                                  ({'chaining': True}, 'invalid_arguments'),
                                  ({'sourceEdge': {**source_ref, 'edgeId': str(uuid.uuid4())}}, 'evaluation_failed')):
                call('surface.extend', {'featureId': feature_id, **invalid}, error=code)
                failures.append(transcript[-1]['response']['error'])
                unchanged(baseline)
            edit = call('feature.editDialog', {'featureId': feature_id})
            sheet(call('command.preview', {'session': edit['id']})['geometry'], 40, 20)
            stale = {**source_ref, 'edgeId': str(uuid.uuid4())}
            call('command.update', {'session': edit['id'], 'inputs': {'sourceEdge': stale}}, error='surface_source_unavailable')
            call('command.commit', {'session': edit['id']}, error='surface_source_unavailable')
            unchanged(baseline)
            call('command.update', {'session': edit['id'], 'inputs': {'sourceEdge': source_ref, 'distanceMm': 10}})
            measurements['editPreview10'] = sheet(call('command.preview', {'session': edit['id']})['geometry'], 40, 10)
            call('command.cancel', {'session': edit['id']})
            unchanged(baseline)
            edit = call('feature.editDialog', {'featureId': feature_id})
            call('command.update', {'session': edit['id'], 'inputs': {'distanceMm': 10}})
            call('command.commit', {'session': edit['id']})
            measurements['edited10'] = sheet(result_body(inspect()), 40, 10)
            assert ids(result_body(inspect())) == stable_ids
            call('document.save', {'path': str(out/'extended-40-10.alloy')})
            call('surface.extend', {'featureId': feature_id, 'distanceMm': 20})
            before_width = inspect()
            call('sketch.edit', {'sketchId': sketch_id})
            call('sketch.dimensionUpdate', {'dimensionId': width_dimension, 'value': 45, 'units': 'mm'})
            call('sketch.finish')
            final = inspect()
            measurements['sourceWidthEdit'] = sheet(result_body(final), 45, 20)
            assert ids(result_body(final)) == stable_ids
            assert final['authored']['features'][-1]['parameters']['sourceEdge'] == source_ref
            call('history.undo')
            assert inspect()['authored'] == before_width['authored']
            call('history.redo')
            assert inspect()['authored'] == final['authored']
            model = out/'extended-45-20.alloy'
            call('document.save', {'path': str(model)})
            call('document.new', {'discard': True})
            call('document.open', {'path': str(model), 'discard': True})
            reopened = inspect()
            assert reopened['authored'] == final['authored']
            measurements['reopened'] = sheet(result_body(reopened), 45, 20)
            assert ids(result_body(reopened)) == stable_ids
            call('view.home')
            call('capture.screenshot', {'path': str(out/'extend-edited-reopened.png')})
            report = {'passed': True, 'measurements': measurements, 'sourceEdge': source_ref,
                      'featureId': feature_id, 'bodyId': source_id, 'environment': environment,
                      'captureEnvironment': call('app.state'), 'failures': failures, 'final': reopened,
                      'scope': 'Shared real-app API and programmatic canvas-hit route; analytic boundaries/area/bounds, in-place identity/history/reopen and GPU captures. Independent actual BRep probe is separate. Curved self-intersection, target termination, physical native hit/drag and public pixel parity remain unverified.'}
            (out/'result.json').write_text(json.dumps(report, indent=2)+'\n')
        finally:
            (out/'transcript.json').write_text(json.dumps(transcript, indent=2)+'\n')


if __name__ == '__main__':
    main()
