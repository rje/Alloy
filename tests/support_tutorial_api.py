#!/usr/bin/env python3
"""Run the source video's core support tutorial through Alloy's shared API.

The external source ledger records observed values and declared frame inference.
Physical keys/gestures, annotation appearance and visual parity are separate gates.
"""
import argparse
import json
import math
from pathlib import Path

from app_harness import application
from stl_export_api import inspect_stl


def video_fillet_seeds(body, outer_feature_id):
    """Fixture identification only; commands persist exact generated edge IDs."""
    selected = {}
    for edge in body['solidEdges']:
        provenance = edge['provenance']
        if (not edge['available'] or provenance['generatorFeatureId'] != outer_feature_id
                or provenance['role'] != 'sideCorner'):
            continue
        a, b = edge['startMm'], edge['endMm']
        if (abs(a[0]-32) > 1e-6 or abs(b[0]-32) > 1e-6
                or abs(a[1]-b[1]) > 1e-6 or max(a[2], b[2]) < 23.9):
            continue
        for y in (-10, 0):
            if abs(a[1]-y) < 1e-6:
                assert y not in selected, ('ambiguous source corner', y)
                selected[y] = edge['edgeId']
    assert set(selected) == {-10, 0}, 'Both documented outer vertical corners required'
    return [selected[-10], selected[0]]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    output = Path(args.output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    receipt = {'result': 'failed', 'sourceReferenceId': 'support-original-video-20261012',
               'variant': 'video: two outer-prong seed edges, radius 1 mm',
               'limits': ['API modeling evidence; native entry, annotations and visual parity remain separate.',
                          'Foot pivot and ordered local Move composition are declared source-informed inference.',
                          'PDF six-edge/radius0.5 variant and optional text/chamfer are outside this run.',
                          'Optional diameter resize checks Alloy editing; the video points to a separate adjustment tutorial.'],
               'moveConvention': {'pivotMm': [7.5, -5, 20],
                                  'orderedLocalSteps': [{'yDeg': -10}, {'zMm': -31}, {'xMm': -2}]},
               'stages': []}
    try:
        with application(args.app, output) as (call, transcript):
            receipt['transcript'] = transcript
            state = call('app.state')
            assert state['viewport']['gpu']['hardwareAccelerated']
            assert state['document']['features'] == 0
            receipt['gpu'] = state['viewport']['gpu']

            def inspect():
                return call('document.inspect')

            def stage(name, home=True):
                if home:
                    call('view.home')
                capture = call('capture.screenshot', {'path': str(output / f'{name}.png')})
                document = inspect()
                receipt['stages'].append({'name': name, 'document': document, 'capture': capture})
                return document

            session = call('sketch.begin', {'plane': 'XY', 'name': 'Support outline'})

            def draft():
                return call('command.preview', {'session': session['id']})['authoredDraft']

            def entity_at(u, v):
                parameters = draft()['parameters']
                points = {p['id']: (p['uMm'], p['vMm']) for p in parameters['points']}
                for entity in parameters['entities']:
                    if entity['type'] != 'line':
                        continue
                    a, b = points[entity['startPointId']], points[entity['endPointId']]
                    dx, dy = b[0] - a[0], b[1] - a[1]
                    t = ((u-a[0])*dx + (v-a[1])*dy) / (dx*dx + dy*dy)
                    if 0 <= t <= 1 and math.hypot(u-a[0]-t*dx, v-a[1]-t*dy) < 1e-5:
                        return entity['id']
                raise AssertionError(('missing source line', u, v))

            def line_dimension(u, v, kind, value, annotation):
                call('sketch.dimension', {'kind': kind, 'entityId': entity_at(u, v),
                                          'value': value, 'units': 'mm', 'annotation': annotation})

            # Numbered steps1-5: numeric geometry AND authored driving dimensions.
            call('sketch.rectangle', {'x1': 0, 'y1': 0, 'x2': 32, 'y2': -10, 'units': 'mm'})
            line_dimension(16, 0, 'horizontal', 32, {'uMm': 16, 'vMm': 19})
            line_dimension(0, -2, 'vertical', 10, {'uMm': -18, 'vMm': -5})
            call('sketch.line', {'x1': 0, 'y1': -5, 'x2': 7.5, 'y2': -5, 'units': 'mm'})
            line_dimension(4, -5, 'horizontal', 7.5, {'uMm': 3.75, 'vMm': 4})
            call('sketch.addCircle', {'centerX': 7.5, 'centerY': -5, 'diameter': 15, 'units': 'mm'})
            outline_circle = draft()['parameters']['entities'][-1]['id']
            call('sketch.dimension', {'kind': 'diameter', 'entityId': outline_circle,
                                      'value': 15, 'units': 'mm', 'annotation': {'uMm': 10, 'vMm': -8}})
            for x in (23, 26, 29):
                call('sketch.rectangle', {'x1': x, 'y1': 0, 'x2': x+3, 'y2': -10, 'units': 'mm'})
                # Two3mm labels are visible in the source: inner and outer.
                # Refer to each freshly authored rectangle, not the coincident
                # original long top line returned by generic entity_at.
                if x in (23, 29):
                    rectangle_top = draft()['parameters']['entities'][-4]['id']
                    call('sketch.dimension', {'kind': 'horizontal', 'entityId': rectangle_top,
                                              'value': 3, 'units': 'mm',
                                              'annotation': {'uMm': x+1.5, 'vMm': 8.5}})
            before_trim = draft()
            call('capture.screenshot', {'path': str(output / '01-before-trim.png')})
            for u, v in [(7.5, 0), (0, -2.5), (0, -7.5), (4, -5), (7.5, -10)]:
                call('sketch.trim', {'entityId': entity_at(u, v), 'uMm': u, 'vMm': v})
            trimmed = draft()
            dimensions = trimmed['parameters']['dimensions']
            assert sorted(round(d['valueMm'], 7) for d in dimensions) == [3, 3, 15, 32]
            assert next(e for e in trimmed['parameters']['entities'] if e['id'] == outline_circle)['type'] == 'circle'
            receipt['trim'] = {'before': before_trim, 'after': trimmed,
                               'retainedDimensionValuesMm': [32, 15, 3, 3]}
            call('capture.screenshot', {'path': str(output / '02-trimmed-dimensions.png')})
            call('sketch.finish')
            outline = stage('03-five-regions', home=False)['evaluation']['sketches'][0]
            assert len(outline['profiles']) == 5
            profiles = {}
            for name, u in [('disk', 7.5), ('bridge', 18), ('inner', 24.5), ('middle', 27.5), ('outer', 30.5)]:
                point = call('view.projectSketchPoint', {'plane': 'XY', 'uMm': u, 'vMm': -5})
                picked = call('selection.pick', {'x': point['x'], 'y': point['y']})
                assert picked['kind'] == 'profile' and picked['sketchId'] == outline['sketchId']
                profiles[name] = picked['profileId']
            receipt['selectedProfiles'] = profiles
            source = {'sketchId': outline['sketchId'], 'units': 'mm'}

            # Steps6-9 use actual profiles; heights are from the same sketch plane.
            call('solid.extrude', {**source, 'profileId': profiles['disk'], 'distance': 20})
            base = stage('04-disk20')
            target = base['evaluation']['bodies'][0]['bodyId']
            disk_cap = base['evaluation']['bodies'][0]['supportFaces'][0]
            join_inputs = {**source, 'profileIds': [profiles['bridge'], profiles['middle']],
                           'distance': 4, 'operation': 'join'}
            preview_session = call('command.begin', {'command': 'solid.extrude', 'inputs': join_inputs})
            preview = call('command.preview', {'session': preview_session['id']})
            assert len(preview['resultBodyIds']) == 2
            assert call('command.preview', {'session': preview_session['id']})['resultBodyIds'] == preview['resultBodyIds']
            call('capture.screenshot', {'path': str(output / '05-two-selected-join4-preview.png')})
            call('command.cancel', {'session': preview_session['id']})
            assert inspect()['authored'] == base['authored']
            call('solid.extrude', join_inputs)
            intermediate = stage('06-bridge-middle4')
            assert intermediate['bodies'] == 2
            call('solid.extrude', {**source, 'profileId': profiles['outer'], 'distance': 24, 'operation': 'join'})
            outer_stage = stage('07-outer24')
            assert outer_stage['bodies'] == 2
            outer_feature = outer_stage['authored']['features'][-1]
            call('solid.extrude', {**source, 'profileId': profiles['inner'], 'distance': 22, 'operation': 'join'})
            joined = stage('08-inner22')
            assert joined['bodies'] == 1 and joined['evaluation']['bodies'][0]['bodyId'] == target
            joined_body = joined['evaluation']['bodies'][0]
            bridge_area = next(p['areaMm2'] for p in outline['profiles'] if p['profileId'] == profiles['bridge'])
            expected_joined_volume = math.pi*56.25*20 + bridge_area*4 + 30*(4+24+22)
            assert math.isclose(joined_body['volumeMm3'], expected_joined_volume, rel_tol=1e-8)

            # Step10 VIDEO variant: both generated outer-prong vertical corners.
            # Nearby public frames resolve the second pick; a former end-rim
            # inference is superseded. No shape ordinal/proximity rebinding.
            seeds = video_fillet_seeds(joined_body, outer_feature['id'])
            receipt['selectedFilletSeeds'] = seeds
            fillet_session = call('solid.filletDialog', {'targetBodyId': target})
            for edge in seeds:
                point = call('view.projectEdge', {'bodyId': target, 'edgeId': edge})
                picked = call('selection.pick', {'x': point['x'], 'y': point['y']})
                assert picked['edgeId'] == edge
            call('command.update', {'session': fillet_session['id'], 'inputs': {'radiusMm': 1}})
            assert call('command.preview', {'session': fillet_session['id']})['geometry']['valid']
            call('capture.screenshot', {'path': str(output / '09-two-seed-fillet-preview.png')})
            call('command.cancel', {'session': fillet_session['id']})
            assert inspect()['authored'] == joined['authored']
            fillet_session = call('solid.filletDialog', {'targetBodyId': target, 'edgeIds': seeds, 'radiusMm': 1})
            call('command.commit', {'session': fillet_session['id']})
            filleted = stage('10-filleted-video-variant')
            filleted_body = filleted['evaluation']['bodies'][0]
            assert filleted_body['valid'] and filleted_body['closed'] and filleted_body['topology']['solids'] == 1
            assert 0 < filleted_body['volumeMm3'] < expected_joined_volume
            support = {key: disk_cap[key] for key in ('producerFeatureId', 'bodyId', 'faceId')}
            assert any(all(face[key] == support[key] for key in support) for face in filleted_body['supportFaces'])

            # Step11: command-first circle, ORIGINAL disk cap20, diameter5.2 Enter.
            pending = call('sketch.circle')
            assert pending['pendingTool'] == 'circle'
            hole_session = call('sketch.begin', {'support': support, 'name': 'Straw bore'})
            assert hole_session['tool'] == 'circle'
            center = call('view.projectSketchPoint', {'sketchId': hole_session['draftSketch']['id'],
                                                     'uMm': 7.5, 'vMm': -5})
            call('sketch.placePoint', {key: center[key] for key in ('x', 'y')})
            call('sketch.circleDiameterPreview', {'text': '5.2'})
            call('sketch.circleDiameterAccept', {'text': '5.2'})
            hole_draft = call('command.preview', {'session': hole_session['id']})['authoredDraft']
            hole_dimension = next(d['id'] for d in hole_draft['parameters']['dimensions'] if d['kind'] == 'diameter')
            call('sketch.finish')
            hole_stage = stage('11-original-disk-circle')
            hole = hole_stage['evaluation']['sketches'][-1]
            assert hole['frame']['origin'][2] == 20
            assert math.isclose(hole['profiles'][0]['areaMm2'], math.pi*2.6**2, rel_tol=1e-8)
            hole_source = {'sketchId': hole['sketchId'], 'profileId': hole['profiles'][0]['profileId'], 'units': 'mm'}
            call('solid.extrude', {**hole_source, 'distance': 47})
            cutter_stage = stage('12-cutter47')
            assert cutter_stage['bodies'] == 2
            cutter = next(body for body in cutter_stage['evaluation']['bodies'] if body['bodyId'] != target)
            assert math.isclose(cutter['volumeMm3'], math.pi*2.6**2*47, rel_tol=1e-8)
            assert math.isclose(cutter['boundsMm']['min'][2], 20, abs_tol=1e-6)
            assert math.isclose(cutter['boundsMm']['max'][2], 67, abs_tol=1e-6)

            # Steps13-16: source incremental values under the declared local frame.
            call('solid.move', {'targetBodyId': cutter['bodyId'],
                                'pivotXMm': 7.5, 'pivotYMm': -5, 'pivotZMm': 20,
                                'steps': receipt['moveConvention']['orderedLocalSteps']})
            moved = stage('13-tilted-translated-cutter')
            transformed = next(body for body in moved['evaluation']['bodies'] if body['bodyId'] == cutter['bodyId'])
            assert math.isclose(transformed['volumeMm3'], cutter['volumeMm3'], rel_tol=1e-8)
            assert transformed['boundsMm']['min'][2] < 0 and transformed['boundsMm']['max'][2] > 20

            # Steps17-19: real Combine Cut; tool consumed, same retained support ID.
            combine_inputs = {'targetBodyId': target, 'toolBodyIds': [cutter['bodyId']],
                              'operation': 'cut', 'keepTools': False}
            cut_session = call('command.begin', {'command': 'solid.combine', 'inputs': combine_inputs})
            call('command.preview', {'session': cut_session['id']})
            call('capture.screenshot', {'path': str(output / '14-combine-cut-preview.png')})
            call('command.cancel', {'session': cut_session['id']})
            assert inspect()['authored'] == moved['authored']
            cut_session = call('command.begin', {'command': 'solid.combine', 'inputs': combine_inputs})
            call('command.commit', {'session': cut_session['id']})
            finished = stage('15-finished-through-hole')
            assert finished['bodies'] == 1
            final = finished['evaluation']['bodies'][0]
            assert final['bodyId'] == target and final['valid'] and final['closed'] and final['topology']['solids'] == 1
            # The cylinder crosses both disk planes fully and stays inside its
            # perimeter. Prongs/fillets are outside the bore's swept footprint.
            removed_volume = math.pi*2.6**2*20/math.cos(math.radians(10))
            expected_volume = filleted_body['volumeMm3'] - removed_volume
            assert math.isclose(final['volumeMm3'], expected_volume, rel_tol=1e-7)
            call('history.undo')
            assert inspect()['authored'] == moved['authored']
            call('history.redo')
            assert inspect()['authored'] == finished['authored']

            # One upstream prong edit replays Join -> Fillet -> support -> Move/Cut.
            call('solid.extrude', {**source, 'featureId': outer_feature['id'],
                                   'profileId': profiles['outer'], 'distance': 26})
            edited = inspect()
            assert edited['bodies'] == 1 and edited['evaluation']['bodies'][0]['bodyId'] == target
            assert edited['evaluation']['bodies'][0]['valid'] and edited['evaluation']['bodies'][0]['volumeMm3'] > final['volumeMm3']
            call('history.undo')
            assert inspect()['authored'] == finished['authored']

            # Step20 separate edit evidence; optional source branch, not a claim
            # that the core video demonstrates these exact adjustment actions.
            call('sketch.edit', {'sketchId': hole['sketchId']})
            call('sketch.dimensionUpdate', {'dimensionId': hole_dimension, 'value': 6.2, 'units': 'mm'})
            call('sketch.finish')
            resized = inspect()
            resized_volume = filleted_body['volumeMm3'] - math.pi*3.1**2*20/math.cos(math.radians(10))
            assert resized['bodies'] == 1 and resized['evaluation']['bodies'][0]['bodyId'] == target
            assert math.isclose(resized['evaluation']['bodies'][0]['volumeMm3'], resized_volume, rel_tol=1e-7)
            receipt['optionalResize'] = {'status': 'Alloy edit verified', 'diameterMm': 6.2, 'document': resized}
            call('history.undo')
            assert inspect()['authored'] == finished['authored']

            model = output / 'straw-support-video.alloy'
            call('document.save', {'path': str(model)})
            call('document.new', {'discard': True})
            call('document.open', {'path': str(model)})
            assert inspect()['authored'] == finished['authored']
            stl = output / 'straw-support-video-mm.stl'
            call('document.exportStl', {'path': str(stl), 'bodyId': target, 'units': 'mm',
                                       'format': 'binary', 'surfaceDeviationMm': 0.03, 'normalDeviationDeg': 5})
            mesh = inspect_stl(stl, True)
            assert math.isclose(mesh['signedVolume'], expected_volume, rel_tol=0.005)
            for size, expected in zip([hi-lo for lo, hi in zip(*mesh['bounds'])], [32, 15, 24]):
                assert math.isclose(size, expected, abs_tol=0.05)
            receipt.update(result='passed', joinedAnalyticVolumeMm3=expected_joined_volume,
                           removedAnalyticVolumeMm3=removed_volume, finalBody=final, mesh=mesh,
                           files={'model': str(model), 'stl': str(stl)})
    finally:
        (output / 'support-tutorial-api.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print('PASS: dimensioned Trim/regions/Join/Fillet/original-cap/Move/Cut, edit/history/reopen and closed mm STL')


if __name__ == '__main__':
    main()
