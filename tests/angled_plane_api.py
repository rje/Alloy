#!/usr/bin/env python3
"""GEO-025 independent real-app retained angled-plane fixture.

The positive side is Alloy's explicit right-hand policy, not established public
reference sign parity. Run only while holding the private application lease.
"""
import argparse
import json
import math
from pathlib import Path
from app_harness import application


def dot(a, b):
    return sum(x*y for x, y in zip(a, b))


def cross(a, b):
    return [a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]]


def vector(actual, expected, tolerance=1e-7):
    assert len(actual) == len(expected)
    assert math.dist(actual, expected) < tolerance, (actual, expected)


def rotated(normal, pivot_a, pivot_b, degrees):
    axis = [b-a for a, b in zip(pivot_a, pivot_b)]
    length = math.sqrt(dot(axis, axis))
    assert length > 0
    axis = [x/length for x in axis]
    c, s = math.cos(math.radians(degrees)), math.sin(math.radians(degrees))
    return [x*c+y*s+z*dot(axis, normal)*(1-c)
            for x, y, z in zip(normal, cross(axis, normal), axis)]


def check_frame(frame, pivot_a, pivot_b, degrees, source_normal=(0, 0, 1)):
    expected = rotated(source_normal, pivot_a, pivot_b, degrees)
    # Oriented normal is checked separately from plane coincidence.
    vector(frame['normal'], expected)
    n, u, v, origin = (frame[k] for k in ('normal', 'uAxis', 'vAxis', 'origin'))
    for direction in (n, u, v):
        assert abs(dot(direction, direction)-1) < 1e-8, frame
    for a, b in ((n, u), (n, v), (u, v)):
        assert abs(dot(a, b)) < 1e-8, frame
    vector(cross(u, v), n)
    residuals = [abs(dot(n, [x-y for x, y in zip(p, origin)])) for p in (pivot_a, pivot_b)]
    assert max(residuals) < 1e-7, (frame, residuals)
    assert abs(dot(n, [b-a for a, b in zip(pivot_a, pivot_b)])) < 1e-7
    return {'angleDeg': degrees, 'expectedOrientedNormal': expected,
            'actualFrame': frame, 'maxPivotPlaneResidualMm': max(residuals),
            'equation': {'normal': n, 'constant': -dot(n, origin)}}


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
                for key in ('authored', 'evaluation', 'undo', 'revision'):
                    assert after[key] == before[key], key
            def frame(state, plane_id):
                return next(p['frame'] for p in state['evaluation']['constructionPlanes']
                            if p['featureId'] == plane_id)
            def authored(state, feature_id):
                return next(f for f in state['authored']['features'] if f['id'] == feature_id)
            call('window.resize', {'width': 1280, 'height': 900})
            environment = call('app.state')
            assert environment['viewport']['gpu']['hardwareAccelerated'], environment
            assert 'construction.angledPlane' in {c['id'] for c in call('commands.list')['commands']}
            source_id = call('construction.plane', {'source': {'plane': 'XY'}, 'offset': 0,
                                                   'units': 'mm', 'name': 'XY0'})['planeId']
            call('sketch.create', {'planeId': source_id, 'name': 'PivotY'})
            call('sketch.line', {'x1': 0, 'y1': 0, 'x2': 0, 'y2': 20, 'units': 'mm'})
            pivot_sketch = draft()
            line, = [e for e in pivot_sketch['parameters']['entities'] if e['type'] == 'line']
            call('sketch.finish')
            original = inspect()
            inputs = {'line': {'sketchId': pivot_sketch['id'], 'entityId': line['id']},
                      'plane': {'planeId': source_id}, 'angleDeg': 30, 'flipSide': False,
                      'name': 'Retained 30 degree plane'}
            session = call('command.begin', {'command': 'construction.angledPlane', 'inputs': {}})
            assert session['inputs']['angleDeg'] == 0 and session['inputs']['flipSide'] is False
            assert not session['previewed']
            assert call('app.state')['ui']['activeDialog']['visible']
            call('command.update', {'session': session['id'], 'inputs': inputs})
            preview = call('command.preview', {'session': session['id']})
            check_frame(preview['previewFrame'], [0, 0, 0], [0, 20, 0], 30)
            call('command.update', {'session': session['id'], 'inputs': {'flipSide': True}})
            preview = call('command.preview', {'session': session['id']})
            check_frame(preview['previewFrame'], [0, 0, 0], [0, 20, 0], -30)
            call('command.update', {'session': session['id'], 'inputs': {'flipSide': False}})
            call('view.home')
            call('view.zoom', {'factor': 0.2})
            call('capture.screenshot', {'path': str(out/'angle30-preview.png')})
            call('capture.dialog', {'path': str(out/'angle30-panel.png')})
            call('command.cancel', {'session': session['id']})
            unchanged(original)
            plane_id = call('construction.angledPlane', inputs)['planeId']
            baseline = inspect()
            assert len(baseline['authored']['features']) == len(original['authored']['features'])+1
            parameters = authored(baseline, plane_id)['parameters']
            assert parameters['angleDeg'] == 30 and parameters['flipSide'] is False
            retained = parameters['source']['angled']
            assert retained['plane'] == inputs['plane']
            assert retained['line'] == {**inputs['line'],
                                        'startPointId': line['startPointId'],
                                        'endPointId': line['endPointId']}
            measurements = {'positive30': check_frame(frame(baseline, plane_id), [0, 0, 0], [0, 20, 0], 30)}
            def edit(**changes):
                return call('construction.angledPlane', {**inputs, 'featureId': plane_id, **changes})
            edit(flipSide=True)
            flipped = inspect()
            measurements['flipped30'] = check_frame(frame(flipped, plane_id), [0, 0, 0], [0, 20, 0], -30)
            edit(angleDeg=45)
            fortyfive = inspect()
            measurements['positive45'] = check_frame(frame(fortyfive, plane_id), [0, 0, 0], [0, 20, 0], 45)
            call('history.undo')
            assert inspect()['authored'] == flipped['authored']
            call('history.redo')
            assert inspect()['authored'] == fortyfive['authored']
            call('sketch.create', {'planeId': plane_id, 'name': 'Attached profile'})
            call('sketch.rectangle', {'x1': 0, 'y1': 0, 'x2': 10, 'y2': 6, 'units': 'mm'})
            downstream_id = draft()['id']
            call('sketch.finish')
            def downstream(state):
                sketch = next(s for s in state['evaluation']['sketches'] if s['featureId'] == downstream_id)
                assert sketch['frame'] == frame(state, plane_id)
                for point in sketch['worldPoints']:
                    p = point['positionMm']
                    f = sketch['frame']
                    assert abs(dot(f['normal'], [x-y for x, y in zip(p, f['origin'])])) < 1e-7
                return sketch
            downstream(inspect())
            call('sketch.edit', {'sketchId': pivot_sketch['id']})
            call('sketch.movePoint', {'pointId': line['endPointId'], 'x': 10, 'y': 20, 'units': 'mm'})
            call('sketch.finish')
            line_edited = inspect()
            measurements['lineDirectionEdit'] = check_frame(frame(line_edited, plane_id), [0, 0, 0], [10, 20, 0], 45)
            downstream(line_edited)
            call('construction.plane', {'featureId': source_id, 'source': {'plane': 'XY'},
                                         'offset': 7, 'units': 'mm'})
            changed = inspect()
            measurements['sourcePlaneEdit'] = check_frame(frame(changed, plane_id), [0, 0, 7], [10, 20, 7], 45)
            downstream(changed)
            call('history.undo')
            assert inspect()['authored'] == line_edited['authored']
            call('history.redo')
            redone = inspect()
            assert redone['authored'] == changed['authored']
            changed = redone
            errors = []
            for bad in ({'line': {**inputs['line'], 'entityId': '00000000-0000-0000-0000-000000000001'}},
                        {'plane': {'plane': 'YZ'}},
                        {'plane': {'sketchId': pivot_sketch['id']}},
                        {'line': {'bodyId': source_id, 'edgeId': line['id']}}):
                call('construction.angledPlane', {**inputs, 'featureId': plane_id, **bad}, error='evaluation_failed')
                errors.append(transcript[-1]['response']['error'])
                unchanged(changed)
            call('construction.angledPlane', {**inputs, 'featureId': plane_id, 'plane': {'plane': 'XY'}},
                 error='evaluation_failed')
            errors.append(transcript[-1]['response']['error'])
            unchanged(changed)
            call('feature.editDialog', {'featureId': plane_id})
            session = call('app.state')['activeSession']
            saved_inputs = session['inputs']
            assert saved_inputs['angleDeg'] == 45 and saved_inputs['flipSide'] is False
            assert saved_inputs['plane'] == inputs['plane']
            call('command.cancel', {'session': session['id']})
            unchanged(changed)
            model = out/'retained-angled-plane.alloy'
            call('document.save', {'path': str(model)})
            call('document.new', {'discard': True})
            call('document.open', {'path': str(model), 'discard': True})
            reopened = inspect()
            assert reopened['authored'] == changed['authored']
            measurements['reopened'] = check_frame(frame(reopened, plane_id), [0, 0, 7], [10, 20, 7], 45)
            downstream(reopened)
            call('view.home')
            call('view.zoom', {'factor': 0.2})
            call('capture.screenshot', {'path': str(out/'edited-reopened.png')})
            capture_environment = call('app.state')
            (out/'result.json').write_text(json.dumps({'passed': True, 'measurements': measurements,
                'sourcePlaneId': source_id, 'planeId': plane_id, 'sourceLine': retained['line'],
                'downstreamSketchId': downstream_id, 'errors': errors, 'environment': environment,
                'captureEnvironment': capture_environment, 'final': reopened,
                'scope': 'Shared real-app API, independent analytic frames/plane equations, authored/history/reopen, compiled panel and GPU captures. Physical actual-hit input and matched public visual parity remain unknown.'}, indent=2)+'\n')
        finally:
            (out/'transcript.json').write_text(json.dumps(transcript, indent=2)+'\n')


if __name__ == '__main__':
    main()
