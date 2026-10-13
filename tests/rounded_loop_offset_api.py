#!/usr/bin/env python3
"""Real-entrypoint retained rounded-loop offset and rim Join contract."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    output = Path(args.output).resolve()
    fixture = Path(__file__).resolve().parent / 'fixtures/rounded-rectangle-source.alloy'
    with application(args.app, output) as (call, transcript):
        call('document.open', {'path': str(fixture)})
        inspect = lambda: call('document.inspect')
        source = inspect()['evaluation']['sketches'][0]
        source_id = source['sketchId']
        outer_area = 1800 - 36 * (4 - math.pi)
        assert math.isclose(source['profiles'][0]['areaMm2'], outer_area, abs_tol=1e-5)
        call('solid.extrude', {'sketchId': source_id, 'profileId': source['profiles'][0]['profileId'],
                               'distance': 2, 'units': 'mm'})
        body = inspect()['evaluation']['bodies'][0]
        cap = next(f for f in body['supportFaces'] if f['role'] == 'extrude.endCap')
        support = {k: cap[k] for k in ('producerFeatureId', 'bodyId', 'faceId')}
        assert math.isclose(cap['frame']['origin'][2], 2, abs_tol=1e-8)
        call('sketch.begin', {'support': support, 'name': 'Retained rounded inset', 'units': 'mm'})
        call('sketch.tool', {'tool': 'offset'})
        active = lambda: call('app.state')['activeSession']
        original = active()['draftSketch']
        end_rim = next(e for e in body['solidEdges'] if e['provenance']['role'] == 'endRim')
        point = call('view.projectEdge', {'bodyId': body['bodyId'], 'edgeId': end_rim['edgeId']})
        call('sketch.placePoint', {'x': point['x'], 'y': point['y']})
        pending = active()['pendingLoopOffset']
        assert pending['valid'] and pending['chain'] and pending['distanceMm'] == -1.5
        assert active()['draftSketch'] == original
        call('capture.screenshot', {'path': str(output / 'loop-preview.png')})
        call('sketch.offsetCancel')
        assert active()['draftSketch'] == original and 'pendingLoopOffset' not in active()
        call('sketch.tool', {'tool': 'offset'})
        call('sketch.placePoint', {'x': point['x'], 'y': point['y']})
        call('sketch.loopOffsetPreview', {'distance': -100, 'units': 'mm'}, error='ambiguous_offset')
        assert active()['draftSketch'] == original
        call('sketch.loopOffsetPreview', {'distance': -1.5, 'units': 'mm'})
        accepted = call('sketch.loopOffsetAccept')
        profile = accepted['sketchGeometry']['profiles'][0]
        assert profile['innerLoopCount'] == 1 and profile['distanceMm'] == -1.5
        assert all(math.isclose(value, 4.5, abs_tol=1e-6) for value in profile['innerArcRadiiMm'])
        assert all(math.isclose(actual, target, abs_tol=1e-5)
                   for actual, target in zip(profile['innerBoundsMm']['size'][:2], (57, 27)))
        annulus_area = outer_area - (57 * 27 - 4.5**2 * (4 - math.pi))
        assert math.isclose(profile['areaMm2'], annulus_area, abs_tol=1e-5)
        offset_id = accepted['draftSketch']['parameters']['loopOffsets'][0]['id']
        sketch_id = accepted['draftSketch']['id']
        call('capture.screenshot', {'path': str(output / 'loop-accepted.png')})
        call('sketch.finish')
        authored_offset = inspect()['authored']
        assert authored_offset['features'][-1]['parameters']['loopOffsets'][0]['id'] == offset_id
        call('solid.extrude', {'sketchId': sketch_id, 'profileId': profile['profileId'],
                               'operation': 'join', 'targetBodyIds': [body['bodyId']],
                               'distance': 1.5, 'units': 'mm'})
        joined = inspect()
        assert len(joined['evaluation']['bodies']) == 1
        assert math.isclose(joined['evaluation']['bodies'][0]['volumeMm3'],
                            outer_area * 2 + annulus_area * 1.5, abs_tol=1e-4)
        call('selection.pick', {'x': 20, 'y': 20})
        call('view.home')
        call('capture.screenshot', {'path': str(output / 'joined-rim.png')})
        path = output / 'rounded-loop-rim.alloy'
        call('document.save', {'path': str(path)})
        saved = inspect()['authored']
        call('document.open', {'path': str(path)})
        assert inspect()['authored'] == saved
        call('sketch.edit', {'sketchId': sketch_id})
        call('sketch.loopOffsetUpdate', {'offsetId': offset_id, 'distance': -1.25, 'units': 'mm'})
        call('sketch.finish')
        assert inspect()['evaluation']['sketches'][-1]['profiles'][0]['profileId'] == profile['profileId']
        call('history.undo')
        assert inspect()['authored'] == saved
        radius = next(d for d in saved['features'][0]['parameters']['dimensions'] if d['kind'] == 'radius')
        call('sketch.edit', {'sketchId': source_id})
        call('sketch.dimensionUpdate', {'dimensionId': radius['id'], 'value': 5.5, 'units': 'mm'})
        call('sketch.finish')
        updated = inspect()['evaluation']['sketches'][-1]['profiles'][0]
        assert updated['profileId'] == profile['profileId']
        assert not math.isclose(updated['areaMm2'], annulus_area, abs_tol=1e-3)
        call('history.undo')
        assert inspect()['authored'] == saved
        (output / 'report.json').write_text(json.dumps({
            'scope': 'Real Alloy API: actual top-boundary click and generated-loop commands; native Qt input is a separate check.',
            'sourceAreaMm2': outer_area, 'annulusAreaMm2': annulus_area,
            'joinedVolumeMm3': joined['evaluation']['bodies'][0]['volumeMm3'],
            'offsetId': offset_id, 'profileId': profile['profileId'], 'callCount': len(transcript)}, indent=2) + '\n')
    print('PASS: retained exact top-face loop, signed inset, Join, edits, undo and reopen')


if __name__ == '__main__':
    main()
