#!/usr/bin/env python3
"""SRF-035 independent shared-edge planar numeric/API lifecycle fixture."""
import argparse
import copy
import json
import math
import sys
import uuid
from pathlib import Path
from app_harness import application


def near(actual, expected, tolerance=2e-7):
    assert math.isclose(actual, expected, rel_tol=1e-9, abs_tol=tolerance), (actual, expected)


def reference(face):
    return {key: face[key] for key in ('producerFeatureId', 'bodyId', 'faceId')}


def analytic(result, angle):
    """Independent planes: y=0 and y=x*tan(angle), meeting on the Z axis.

    Their tangent directions are (1,0,0) and (cos(a),sin(a),0).
    Cross each with extrusion direction (0,0,1) for analytic plane normals.
    Nine affine edge samples must cover Z=-20..20 in either canonical direction.
    This cross-check uses model construction, not reported UVs or echoed maxima.
    """
    assert result['status'] == 'ready', result
    assert result['sampleCount'] == 9 and len(result['samples']) == 9, result
    assert result['method'] == 'uniform_shared_edge_parameter_including_endpoints'
    assert result['persistence'] == 'session_only' and not result['authoredChange']
    near(result['thresholds']['gapMm'], .001)
    near(result['thresholds']['tangentPlaneAngleDeg'], .1)
    assert result['sampledG0Pass'] and result['sampledG1Pass'] == (angle == 0)
    rad = math.radians(angle)
    expected_normals = ([0, -1, 0], [math.sin(rad), -math.cos(rad), 0])
    samples = result['samples']
    z_start, z_end = samples[0]['faces'][0]['pointMm'][2], samples[-1]['faces'][0]['pointMm'][2]
    near(min(z_start, z_end), -20)
    near(max(z_start, z_end), 20)
    for index, sample in enumerate(samples):
        assert sample['index'] == index
        near(sample['fraction'], index/8)
        near(sample['gapMm'], 0)
        near(sample['tangentPlaneAngleDeg'], angle)
        near(sample['orientedNormalAngleDeg'], angle)
        near(sample['normalDot'], math.cos(rad))
        near(sample['edgeParameter'], result['edgeParameterRange'][0] + index/8 *
             (result['edgeParameterRange'][1]-result['edgeParameterRange'][0]))
        for face, expected in zip(sample['faces'], expected_normals):
            point, normal = face['pointMm'], face['orientedUnitNormal']
            near(point[0], 0)
            near(point[1], 0)
            near(point[2], z_start+(z_end-z_start)*index/8)
            near(sum(v*v for v in normal), 1)
            # Surface orientation may choose either global sign, but adjacent
            # consistently directed source lines must agree in sign.
            near(abs(sum(a*b for a, b in zip(normal, expected))), 1)
            assert face['faceOrientation'] in ('forward', 'reversed')
            assert len(face['uv']) == 2
    near(result['maxGapMm'], 0)
    near(result['maxTangentPlaneAngleDeg'], angle)
    assert 0 <= result['maxGapSampleIndex'] < 9
    assert 0 <= result['maxTangentPlaneAngleSampleIndex'] < 9
    assert result['normalOrientation']['diagnosticOnly']
    assert result['normalOrientation']['oppositeHemisphereSampleCount'] == 0
    near(result['normalOrientation']['maxOrientedNormalAngleDeg'], angle)
    assert result['display']['angleUnit'] == 'deg'
    return result


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
            result = lambda inputs=None: call('inspect.surfaceContinuityResult', inputs)
            captures, failures = {}, []
            def unchanged(before):
                after = inspect()
                for key in ('authored', 'revision', 'undo', 'evaluation', 'dirty'):
                    assert after[key] == before[key], key
            def capture(name):
                captures[name] = call('capture.screenshot', {'path': str(out/(name+'.png'))})
                assert captures[name]['source'] == 'QWidget chrome + OCCT OpenGL framebuffer'
                captures[name]['viewportState'] = call('app.state')['viewport']
                captures[name]['panel'] = call('capture.dialog', {'path': str(out/(name+'-controls.png'))})
            call('window.resize', {'width': 1280, 'height': 900})
            assert {'inspect.surfaceContinuity', 'inspect.surfaceContinuityResult'} <= {
                c['id'] for c in call('commands.list')['commands']}
            call('sketch.create', {'plane': 'XY', 'name': 'Two connected analytic plane sources'})
            call('sketch.line', {'x1': -10, 'y1': 0, 'x2': 0, 'y2': 0, 'units': 'mm'})
            first = call('app.state')['activeSession']['draftSketch']['parameters']['entities'][0]
            call('sketch.line', {'x1': 0, 'y1': 0, 'x2': 10, 'y2': 0, 'units': 'mm',
                 'startSnapTarget': {'kind': 'point', 'pointId': first['endPointId']}})
            source = call('app.state')['activeSession']['draftSketch']
            entities = source['parameters']['entities']
            call('sketch.finish')
            call('surface.extrude', {'sketchId': source['id'], 'resultMode': 'perProfile',
                 'profiles': [{'entityIds': [e['id'] for e in entities]}],
                 'sideOneMm': 20, 'sideTwoMm': 20, 'name': 'Exact common-edge plane pair'})
            body, = inspect()['evaluation']['bodies']
            assert body['valid'] and body['topology']['faces'] == 2 and body['topology']['edges'] == 7, body
            # Bind each named face by its retained authored-entity provenance.
            faces = [next(f for f in body['supportFaces'] if
                     f['provenance']['sourceEntityId'] == e['id']) for e in entities]
            refs = [reference(f) for f in faces]
            common = set(e['edgeId'] for e in faces[0]['edges']) & set(
                e['edgeId'] for e in faces[1]['edges'])
            assert len(common) == 1
            inputs = {'faces': refs, 'edgeId': common.pop()}
            call('document.save', {'path': str(out/'continuous.alloy')})
            baseline = inspect()
            continuous = analytic(result(inputs), 0)
            unchanged(baseline)
            call('view.home')
            call('capture.screenshot', {'path': str(out/'continuous-geometry.png')})
            analytic(call('inspect.surfaceContinuity', inputs), 0)
            call('view.home')
            call('selection.edgeSet', {'bodyId': body['bodyId'], 'edgeId': inputs['edgeId']})
            capture('continuous-panel')
            unchanged(baseline)
            for invalid, code in (({'faces': [refs[0]], 'edgeId': inputs['edgeId']}, 'continuity_inputs'),
                 ({'faces': [refs[0], refs[0]], 'edgeId': inputs['edgeId']}, 'continuity_duplicate_face'),
                 ({**inputs, 'edgeId': next(e['edgeId'] for e in faces[0]['edges']
                    if e['edgeId'] != inputs['edgeId'])}, 'continuity_not_shared'),
                 ({**inputs, 'edgeId': str(uuid.uuid4())}, 'continuity_stale_edge'),
                 ({'faces': [refs[0], {**refs[1], 'faceId': str(uuid.uuid4())}],
                   'edgeId': inputs['edgeId']}, 'continuity_stale')):
                failure = result(invalid)
                assert failure['status'] in ('unavailable', 'stale', 'unsupported') and failure['error']['code'] == code, failure
                failures.append(failure)
                unchanged(baseline)
            # Move only the final source endpoint. The common point/edge and
            # both exact retained face references must survive the source edit.
            parameters = copy.deepcopy(source['parameters'])
            endpoint = entities[1]['endPointId']
            for point in parameters['points']:
                if point['id'] == endpoint:
                    point['vMm'] = 10
            call('feature.edit', {'featureId': source['id'], 'parameters': parameters})
            edited = inspect()
            kink = analytic(result(), 45)
            assert kink == result(inputs)
            analytic(call('inspect.surfaceContinuity', inputs), 45)
            capture('kink-panel')
            unchanged(edited)
            call('document.save', {'path': str(out/'kink.alloy')})
            call('history.undo')
            assert inspect()['authored'] == baseline['authored']
            analytic(result(), 0)
            call('history.redo')
            assert inspect()['authored'] == edited['authored']
            analytic(result(), 45)
            call('document.units', {'units': 'in'})
            inches = inspect()
            converted = analytic(result(inputs), 45)
            assert converted['display']['lengthUnit'] == 'in'
            near(converted['display']['gapThreshold'], .001/25.4)
            near(converted['display']['angleThreshold'], .1)
            unchanged(inches)
            call('history.undo')
            assert inspect()['authored'] == edited['authored']
            call('window.resize', {'width': 1100, 'height': 760})
            analytic(call('inspect.surfaceContinuity', inputs), 45)
            capture('kink-resized')
            environment = call('app.state')
            gpu = environment['viewport']['gpu']
            assert gpu['hardwareAccelerated'] and gpu['vendor'] and gpu['version'], gpu
            if sys.platform == 'darwin':
                assert 'apple' in json.dumps(gpu).lower(), gpu
            dpr = captures['kink-resized']['devicePixelRatio']
            near(gpu['nativeWidth']*dpr, gpu['framebufferWidth'])
            near(gpu['nativeHeight']*dpr, gpu['framebufferHeight'])
            call('document.save', {'path': str(out/'kink-after.alloy')})
            assert (out/'kink.alloy').read_bytes() == (out/'kink-after.alloy').read_bytes()
            call('document.new', {'discard': True})
            call('document.open', {'path': str(out/'kink.alloy'), 'discard': True})
            reopened = inspect()
            assert reopened['authored'] == edited['authored'] and not reopened['dirty']
            repeated = analytic(result(inputs), 45)
            assert repeated == kink, (repeated, kink)
            unchanged(reopened)
            call('inspect.surfaceContinuity', inputs)
            call('view.home')
            call('selection.edgeSet', {'bodyId': body['bodyId'], 'edgeId': inputs['edgeId']})
            capture('reopened-panel')
            report = {'passed': True, 'continuous': continuous, 'kink': kink,
                'inches': converted, 'reopenedResult': repeated, 'failures': failures,
                'baseline': baseline, 'edited': edited, 'reopened': reopened,
                'environment': environment, 'captures': captures,
                'scope': 'Independent analytic construction equations at nine exact samples; shared real-app API and passive authored/revision/dirty/history/evaluation invariants; retained source edit, Undo/Redo, canonical units and reopen; actual GPU panel captures. Physical native actual-hit and 1x/2x, distinct-edge gap failures, curved numeric inspection, and matched public pixels remain unqualified.'}
            (out/'result.json').write_text(json.dumps(report, indent=2)+'\n')
        finally:
            (out/'transcript.json').write_text(json.dumps(transcript, indent=2)+'\n')


if __name__ == '__main__':
    main()
