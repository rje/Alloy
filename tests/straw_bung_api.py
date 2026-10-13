#!/usr/bin/env python3
"""Run the public video's numeric straw-bung sequence through Alloy's API.

This checks the modeling chain; native command-first and manipulator gestures
need their own evidence. The source ledger remains outside the checkout.
"""
import argparse
import json
import math
from pathlib import Path

from app_harness import application
from stl_export_api import inspect_stl


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    output = Path(args.output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    receipt = {'result': 'failed', 'sourceReferenceId': 'bung-original-video-20261012',
               'limits': ['API modeling evidence; native keys, clicks and exact visual parity are separate.',
                          'Pivot at cutter foot and local transform composition are declared source-informed conventions.'],
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

            def stage(name):
                call('view.home')
                capture = call('capture.screenshot', {'path': str(output / f'{name}.png')})
                receipt['stages'].append({'name': name, 'document': inspect(), 'capture': capture})

            def circle(diameter, body=None):
                inputs = {'plane': 'XY'}
                if body is not None:
                    face = body['supportFaces'][0]
                    inputs = {'support': {k: face[k] for k in ('producerFeatureId', 'bodyId', 'faceId')}}
                pending = call('sketch.circle')
                assert pending['pendingTool'] == 'circle'
                started = call('sketch.begin', inputs)
                assert started['tool'] == 'circle'
                origin = call('view.projectSketchPoint', {'sketchId': started['draftSketch']['id'],
                                                          'uMm': 0, 'vMm': 0})
                call('sketch.placePoint', {key: origin[key] for key in ('x', 'y')})
                call('sketch.circleDiameterPreview', {'text': str(diameter)})
                call('sketch.circleDiameterAccept', {'text': str(diameter)})
                call('sketch.finish')
                sketch = inspect()['evaluation']['sketches'][-1]
                return {'sketchId': sketch['sketchId'], 'profileId': sketch['profiles'][0]['profileId']}

            base_profile = circle(26)
            call('solid.extrude', {**base_profile, 'distance': 3, 'units': 'mm'})
            base = inspect()['evaluation']['bodies'][0]
            target = base['bodyId']
            stage('01-flange')

            boss_profile = circle(19, base)
            call('solid.extrude', {**boss_profile, 'distance': 8, 'units': 'mm',
                                  'operation': 'join', 'targetBodyIds': [target]})
            joined = inspect()
            boss = joined['evaluation']['bodies'][0]
            join_feature = joined['authored']['features'][-1]
            uncut_volume = math.pi * (13**2 * 3 + 9.5**2 * 8)
            assert joined['bodies'] == 1 and boss['bodyId'] == target
            assert math.isclose(boss['volumeMm3'], uncut_volume, rel_tol=1e-8)
            stage('02-joined-boss')

            cutter_profile = circle(6.2, boss)
            call('solid.extrude', {**cutter_profile, 'distance': 40, 'units': 'mm'})
            bodies = inspect()['evaluation']['bodies']
            cutter = next(b for b in bodies if b['bodyId'] != target)
            assert len(bodies) == 2
            stage('03-face-supported-cutter')

            call('solid.move', {'targetBodyId': cutter['bodyId'],
                               'pivotXMm': 0, 'pivotYMm': 0, 'pivotZMm': 11,
                               'steps': [{'yDeg': 10}, {'zMm': -20}]})
            moved = inspect()
            stage('04-tilted-translated-cutter')
            session = call('command.begin', {'command': 'solid.combine', 'inputs': {
                'targetBodyId': target, 'toolBodyIds': [cutter['bodyId']],
                'operation': 'cut', 'keepTools': False}})
            call('command.preview', {'session': session['id']})
            call('command.commit', {'session': session['id']})
            finished = inspect()
            assert finished['bodies'] == 1
            final = finished['evaluation']['bodies'][0]
            assert final['bodyId'] == target and final['valid'] and final['closed']
            assert final['topology']['solids'] == 1
            expected_volume = uncut_volume - math.pi * 3.1**2 * 11 / math.cos(math.radians(10))
            assert math.isclose(final['volumeMm3'], expected_volume, rel_tol=1e-7)
            stage('05-angled-through-hole')
            call('history.undo')
            assert inspect()['authored'] == moved['authored']
            call('history.redo')
            assert inspect()['authored'] == finished['authored']

            # One upstream edit exercises Join -> supported sketch -> Move -> Cut.
            call('solid.extrude', {**boss_profile, 'featureId': join_feature['id'], 'distance': 10})
            edited = inspect()
            assert edited['bodies'] == 1 and edited['evaluation']['bodies'][0]['valid']
            assert edited['evaluation']['bodies'][0]['volumeMm3'] > final['volumeMm3']
            call('history.undo')
            assert inspect()['authored'] == finished['authored']

            model = output / 'straw-bung.alloy'
            call('document.save', {'path': str(model)})
            call('document.new', {'discard': True})
            call('document.open', {'path': str(model)})
            assert inspect()['authored'] == finished['authored']
            stl = output / 'straw-bung-mm.stl'
            call('document.exportStl', {'path': str(stl), 'bodyId': target, 'units': 'mm',
                                       'format': 'binary', 'surfaceDeviationMm': 0.03,
                                       'normalDeviationDeg': 5})
            mesh = inspect_stl(stl, True)
            assert math.isclose(mesh['signedVolume'], expected_volume, rel_tol=0.005)
            for size, expected in zip([hi-lo for lo, hi in zip(*mesh['bounds'])], [26, 26, 11]):
                assert math.isclose(size, expected, abs_tol=0.05)
            receipt.update(result='passed', finalBody=final, mesh=mesh,
                           files={'model': str(model), 'stl': str(stl)})
    finally:
        (output / 'straw-bung-api.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print('PASS: sketch/Join/face sketch/Move/Combine chain, edit/history/reopen and closed mm STL')


if __name__ == '__main__':
    main()
