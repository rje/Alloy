#!/usr/bin/env python3
"""SRF-042 independent real-app cylindrical offset and atomic-collapse fixture."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application


def near(a, b):
    assert math.isclose(a, b, rel_tol=1e-9, abs_tol=2e-5), (a, b)


def sheet(body, radius, height):
    assert body['type'] == 'surface' and body['valid'] and not body['closed'], body
    assert body['topology']['faces'] == 1 and body['topology']['solids'] == 0, body
    assert body['topology']['edges'] == 4 and body['volumeMm3'] is None, body
    near(body['areaMm2'], radius * height * 2 * math.pi / 3)
    for actual, expected in zip(body['boundsMm']['min'], [-radius/2, 0, 0]):
        near(actual, expected)
    for actual, expected in zip(body['boundsMm']['max'], [radius, radius, height]):
        near(actual, expected)
    # Numeric checks derive radius, arc span and height from actual edge samples,
    # independently of the builder's authored distance/radius summary fields.
    arcs, generators = 0, 0
    for edge in body['solidEdges']:
        assert edge['available'], edge
        a, b, m = [edge[k] for k in ('startMm', 'endMm', 'midpointMm')]
        for p in (a, b, m):
            near(math.hypot(p[0], p[1]), radius)
        if abs(a[2]-b[2]) < 1e-5:
            arcs += 1
            near(math.dist(a, b), radius * math.sqrt(3))
            near(math.dist(a, m), radius)
            near(math.dist(b, m), radius)
        else:
            generators += 1
            near(math.dist(a, b), height)
            near(math.dist(a, m), height/2)
    assert (arcs, generators) == (2, 2), (arcs, generators)
    return {'radiusFromBoundaryMm': radius, 'heightMm': height,
            'areaMm2': body['areaMm2'], 'arcChordMm': radius*math.sqrt(3),
            'boundaryEdges': body['solidEdges']}


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
            def unchanged(before):
                after = inspect()
                for key in ('authored', 'revision', 'undo', 'evaluation'):
                    assert after[key] == before[key], key
            def body(state, body_id):
                return next(b for b in state['evaluation']['bodies'] if b['bodyId'] == body_id)
            call('window.resize', {'width': 1280, 'height': 900})
            environment = call('app.state')
            assert environment['viewport']['gpu']['hardwareAccelerated'], environment
            call('sketch.create', {'plane': 'XZ', 'name': 'Alloy cylinder R10 H30 section'})
            call('sketch.line', {'x1': 10, 'y1': 0, 'x2': 10, 'y2': 30, 'units': 'mm'})
            call('sketch.line', {'x1': 0, 'y1': 0, 'x2': 0, 'y2': 30, 'units': 'mm'})
            draft = call('app.state')['activeSession']['draftSketch']
            section, axis = draft['parameters']['entities']
            sketch_id = draft['id']
            call('sketch.construction', {'entityIds': [axis['id']], 'construction': True})
            call('sketch.finish')
            call('surface.revolve', {'sketchId': sketch_id, 'entityId': section['id'],
                                    'axisEntityId': axis['id'], 'angleDeg': 120})
            original = inspect()
            source = original['evaluation']['bodies'][0]
            source_id = source['bodyId']
            face, = source['supportFaces']
            source_ref = {k: face[k] for k in ('bodyId', 'producerFeatureId', 'faceId')}
            measurements = {'source': sheet(source, 10, 30)}
            feature_id = offset_id = None
            for distance, radius in ((0, 10), (2, 12), (-4, 6), (-9, 1)):
                inputs = {'sourceFace': source_ref, 'distanceMm': distance}
                if feature_id:
                    inputs['featureId'] = feature_id
                call('surface.offset', inputs)
                state = inspect()
                feature = state['authored']['features'][-1]
                feature_id, offset_id = feature['id'], feature['bodyId']
                result = body(state, offset_id)
                measurements[str(distance)] = sheet(result, radius, 30)
                assert body(state, source_id) == source
                assert result['sourceFaceOrientation'] == 'forward', result
                near(result['normalRadialSign'], 1)
                normal, point = result['sourceNormal'], result['sourceNormalPointMm']
                near(normal[2], 0)
                near(sum(normal[i]*point[i] for i in (0, 1)), 10)
                call('document.save', {'path': str(out / f'offset-{distance}.alloy')})
            # Establish valid -4 baseline for edit/preview failures and same-draft recovery.
            call('surface.offset', {'featureId': feature_id, 'distanceMm': -4})
            baseline = inspect()
            ids = lambda b: ([f['faceId'] for f in b['supportFaces']],
                             [e['edgeId'] for e in b['solidEdges']])
            stable_ids = ids(body(baseline, offset_id))
            failures = []
            for distance in (-10, -11):
                call('surface.offset', {'featureId': feature_id, 'distanceMm': distance}, error='evaluation_failed')
                err = transcript[-1]['response']['error']
                assert err['diagnostics']['code'] == 'surface_offset_collapse', err
                assert err['diagnostics']['sourceFace'] == source_ref, err
                failures.append(err)
                unchanged(baseline)
            session = call('feature.editDialog', {'featureId': feature_id})
            call('command.update', {'session': session['id'], 'inputs': {'distanceMm': -10}}, error='evaluation_failed')
            call('command.preview', {'session': session['id']}, error='evaluation_failed')
            call('command.commit', {'session': session['id']}, error='evaluation_failed')
            unchanged(baseline)
            collapse_state = call('app.state')
            assert not collapse_state['viewport']['gpu']['previewVisible'], collapse_state
            assert not collapse_state['viewport']['gpu']['surfaceOffsetCue'], collapse_state
            call('view.home')
            call('capture.screenshot', {'path': str(out/'collapse-preview.png')})
            call('command.update', {'session': session['id'], 'inputs': {'distanceMm': -9}})
            sheet(call('command.preview', {'session': session['id']})['geometry'], 1, 30)
            call('capture.screenshot', {'path': str(out/'valid-retry-preview.png')})
            call('command.commit', {'session': session['id']})
            recovered = inspect()
            call('history.undo')
            assert inspect()['authored'] == baseline['authored']
            call('history.redo')
            assert inspect()['authored'] == recovered['authored']
            recovered = inspect()
            # A new, uncommitted collapsing candidate must also remain uncommittable.
            session = call('surface.offsetDialog', {'sourceFace': source_ref, 'distanceMm': -4})
            call('command.update', {'session': session['id'], 'inputs': {'distanceMm': -10}}, error='evaluation_failed')
            call('command.preview', {'session': session['id']}, error='evaluation_failed')
            call('command.commit', {'session': session['id']}, error='evaluation_failed')
            call('command.cancel', {'session': session['id']})
            unchanged(recovered)
            call('sketch.edit', {'sketchId': sketch_id})
            call('sketch.movePoint', {'pointId': section['startPointId'], 'x': 12, 'y': 0, 'units': 'mm'})
            call('sketch.movePoint', {'pointId': section['endPointId'], 'x': 12, 'y': 40, 'units': 'mm'})
            call('sketch.finish')
            final = inspect()
            measurements['sourceEdited'] = sheet(body(final, source_id), 12, 40)
            measurements['offsetFollowingSource'] = sheet(body(final, offset_id), 3, 40)
            assert ids(body(final, offset_id)) == stable_ids
            assert ids(body(final, source_id)) == ids(source)
            call('history.undo')
            assert inspect()['authored'] == recovered['authored']
            call('history.redo')
            assert inspect()['authored'] == final['authored']
            model = out/'retained-cylinder-offset.alloy'
            call('document.save', {'path': str(model)})
            call('document.new', {'discard': True})
            call('document.open', {'path': str(model), 'discard': True})
            reopened = inspect()
            assert reopened['authored'] == final['authored']
            sheet(body(reopened, offset_id), 3, 40)
            call('view.home')
            call('capture.screenshot', {'path': str(out/'edited-reopened.png')})
            report = {'passed': True, 'measurements': measurements, 'sourceFace': source_ref,
                      'featureId': feature_id, 'bodyId': offset_id, 'environment': environment,
                      'captureEnvironment': call('app.state'), 'collapseState': collapse_state, 'failures': failures, 'final': reopened,
                      'scope': 'Real-entrypoint shared API; boundary-sample/area/bounds geometry, atomic refusals, stable history, reopen and GPU captures. Native physical input and public visual parity unqualified.'}
            (out/'result.json').write_text(json.dumps(report, indent=2)+'\n')
        finally:
            (out/'transcript.json').write_text(json.dumps(transcript, indent=2)+'\n')


if __name__ == '__main__':
    main()
