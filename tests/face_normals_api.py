#!/usr/bin/env python3
"""SRF-033 independent selected planar/cylinder normal and transient-state fixture."""
import argparse
import copy
import json
import math
import sys
from pathlib import Path
from app_harness import application


def near(a, b, tolerance=2e-7):
    assert math.isclose(a, b, rel_tol=1e-9, abs_tol=tolerance), (a, b)


def vector(actual, expected):
    assert len(actual) == len(expected)
    for a, b in zip(actual, expected):
        near(a, b)


def reference(face):
    return {k: face[k] for k in ('producerFeatureId', 'bodyId', 'faceId')}


def analytic(result, source, kind, angle=120):
    assert result['enabled'] and result['visible'], result
    assert result['scope'] == 'single_current_face', result
    assert result['persistence'] == 'session_only' and not result['authoredChange'], result
    normal = result['normal']
    actual_source = {k: v for k, v in normal['source'].items() if v is not None}
    assert actual_source == source and normal['surfaceType'] == kind, normal
    point, direction = normal['pointMm'], normal['unitNormal']
    near(math.sqrt(sum(n*n for n in direction)), 1)
    if kind == 'plane':
        near(point[2], 0)
        assert 30 < point[0] < 70 and 0 < point[1] < 30, point
        vector(direction, [0, 0, 1])
    else:
        # This is an independent analytic equation applied to the evaluated
        # sample position, rather than comparing two echoed response fields.
        near(math.hypot(point[0], point[1]), 10)
        assert 0 < point[2] < 30, point
        theta = math.atan2(point[1], point[0])
        assert 0 < theta < math.radians(angle), point
        vector(direction, [point[0]/10, point[1]/10, 0])
        # For this rectangular analytic trim, the documented first candidate
        # is interior and must be precisely its parametric midpoint.
        near(theta, math.radians(angle/2))
        near(point[2], 15)
    glyph = result['glyph']
    assert not glyph['pickable'] and glyph['lengthMm'] > 0, glyph
    vector(glyph['tailMm'], point)
    vector(glyph['tipMm'], [p + glyph['lengthMm']*n for p, n in zip(point, direction)])
    near(glyph['worldCapMm'], normal['faceDiagonalMm']*.35)
    assert glyph['lengthMm'] <= glyph['worldCapMm'] + 1e-7, glyph
    return normal


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
            result = lambda: call('inspect.faceNormalsResult')
            def unchanged(before):
                after = inspect()
                for key in ('authored', 'revision', 'undo', 'evaluation', 'dirty'):
                    assert after[key] == before[key], key
            def capture(name):
                return call('capture.screenshot', {'path': str(out/(name+'.png'))})

            call('window.resize', {'width': 1280, 'height': 900})
            discovery = {c['id'] for c in call('commands.list')['commands']}
            assert {'inspect.faceNormals', 'inspect.faceNormalsResult'} <= discovery
            assert not result()['enabled']
            call('sketch.create', {'plane': 'XY', 'name': 'Alloy planar normal boundary'})
            call('sketch.rectangle', {'x1': 30, 'y1': 0, 'x2': 70, 'y2': 30, 'units': 'mm'})
            sid = call('app.state')['activeSession']['draftSketch']['id']
            call('sketch.finish')
            profile, = inspect()['evaluation']['sketches'][0]['profiles']
            call('surface.patch', {'sketchId': sid, 'profileId': profile['profileId']})
            plane = inspect()['evaluation']['bodies'][0]
            planar = reference(plane['supportFaces'][0])
            call('sketch.create', {'plane': 'XZ', 'name': 'Alloy R10 H30 cylinder section'})
            call('sketch.line', {'x1': 10, 'y1': 0, 'x2': 10, 'y2': 30, 'units': 'mm'})
            call('sketch.line', {'x1': 0, 'y1': 0, 'x2': 0, 'y2': 30, 'units': 'mm'})
            draft = call('app.state')['activeSession']['draftSketch']
            section, axis = draft['parameters']['entities']
            call('sketch.construction', {'entityIds': [axis['id']], 'construction': True})
            call('sketch.finish')
            call('surface.revolve', {'sketchId': draft['id'], 'entityId': section['id'],
                 'axisEntityId': axis['id'], 'angleDeg': 120})
            initial = inspect()
            cylinder = initial['evaluation']['bodies'][-1]
            curved = reference(cylinder['supportFaces'][0])
            source = initial['authored']['features'][-1]
            model = out/'face-normals.alloy'
            call('document.save', {'path': str(model)})
            baseline = inspect()
            call('view.home')
            pixel = call('view.projectBody', {'bodyId': planar['bodyId']})
            call('selection.facePick', {'x': pixel['x'], 'y': pixel['y']})
            assert call('app.state')['selection']['support'] == planar
            assert not result()['visible']
            planar_result = call('inspect.faceNormals', {'enabled': True})
            analytic(planar_result, planar, 'plane')
            captures = {'plane': capture('planar-normal')}
            unchanged(baseline)
            call('inspect.faceNormals', {'enabled': False})
            assert not result()['visible']
            captures['planeOff'] = capture('planar-off')
            unchanged(baseline)
            call('selection.faceSet', {'support': curved})
            curved_result = call('inspect.faceNormals', {'enabled': True})
            analytic(curved_result, curved, 'cylinder')
            call('view.fitSelection')
            captures['cylinder'] = capture('cylinder-normal')
            unchanged(baseline)
            # Selection replacement and clearing rebuild only the transient glyph.
            call('selection.faceSet', {'support': planar})
            analytic(result(), planar, 'plane')
            call('selection.set', {'bodyId': ''})
            assert result()['status'] == 'no_selection' and not result()['visible']
            unchanged(baseline)
            call('selection.faceSet', {'support': curved})
            analytic(result(), curved, 'cylinder')
            call('view.orbit', {'deltaX': 18, 'deltaY': 12})
            call('window.resize', {'width': 1100, 'height': 760})
            analytic(result(), curved, 'cylinder')
            captures['resized'] = capture('cylinder-resized')
            unchanged(baseline)
            environment = call('app.state')
            gpu = environment['viewport']['gpu']
            assert gpu['hardwareAccelerated'], gpu
            if sys.platform == 'darwin':
                assert 'apple' in json.dumps(gpu).lower(), gpu
            # Authored visibility is a model edit; only its overlay refresh is
            # read-only. Undo must revert that edit, never a normals toggle.
            call('feature.visibility', {'featureId': source['id'], 'visible': False})
            assert not result()['visible']
            call('history.undo')
            assert inspect()['authored'] == baseline['authored']
            call('selection.faceSet', {'support': curved})
            parameters = copy.deepcopy(source['parameters'])
            parameters['angleDeg'] = 90
            call('feature.edit', {'featureId': source['id'], 'parameters': parameters})
            edited = inspect()
            edited_result = call('app.state')['viewport']['faceNormals']
            analytic(edited_result, curved, 'cylinder', 90)
            call('inspect.faceNormals', {'enabled': False})
            call('inspect.faceNormals', {'enabled': True})
            unchanged(edited)
            call('history.undo')
            assert inspect()['authored'] == baseline['authored']
            analytic(result(), curved, 'cylinder')
            call('history.redo')
            assert inspect()['authored'] == edited['authored']
            analytic(result(), curved, 'cylinder', 90)
            call('history.undo')
            assert inspect()['authored'] == baseline['authored']
            call('document.save', {'path': str(out/'face-normals-after.alloy')})
            assert model.read_bytes() == (out/'face-normals-after.alloy').read_bytes()
            call('document.new', {'discard': True})
            assert not result()['enabled'] and not result()['visible']
            call('inspect.faceNormals', {'enabled': True})
            call('document.open', {'path': str(model), 'discard': True})
            assert not result()['enabled'] and not result()['visible']
            reopened = inspect()
            assert reopened['authored'] == baseline['authored'] and not reopened['dirty']
            call('selection.faceSet', {'support': curved})
            analytic(call('inspect.faceNormals', {'enabled': True}), curved, 'cylinder')
            call('window.resize', {'width': 1280, 'height': 900})
            call('view.home')
            captures['reopened'] = capture('reopened-normal')
            report = {'passed': True, 'planar': planar_result, 'cylinder': curved_result,
                      'editedCylinder': edited_result, 'environment': environment,
                      'finalEnvironment': call('app.state'), 'captures': captures,
                      'baseline': baseline, 'reopened': reopened,
                      'scope': 'Shared API and programmatic BRep ray pick, analytic oriented normals at declared samples, transient model/history invariants, recompute, save/reopen and hardware GPU captures. Independent reopened BRep D1 probe separate. Physical actual-hit input and matched public pixels unverified.'}
            (out/'result.json').write_text(json.dumps(report, indent=2)+'\n')
        finally:
            (out/'transcript.json').write_text(json.dumps(transcript, indent=2)+'\n')


if __name__ == '__main__':
    main()
