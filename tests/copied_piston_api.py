#!/usr/bin/env python3
"""Actual-app copied piston loop with independent mounting/travel checks."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application

parser = argparse.ArgumentParser()
parser.add_argument('--app', required=True)
parser.add_argument('--fixture', required=True, help='Accepted four-joint coupled model')
parser.add_argument('--output', required=True)
args = parser.parse_args()
out = Path(args.output).resolve()
out.mkdir(parents=True, exist_ok=True)


def near(a, b, label, tolerance=1e-5):
    assert math.isfinite(a) and abs(a-b) < tolerance, (label, a, b)


def transform(pose, point):
    return [sum(pose[4*r+c]*point[c] for c in range(3))+pose[4*r+3] for r in range(3)]


with application(args.app, out, command_timeout=90) as (call, ledger):
    call('document.open', {'path': str(Path(args.fixture).resolve()), 'discard': True})
    def inspect(): return call('document.inspect')
    def poses(state): return {r['occurrenceId']: r['worldPose'] for r in state['occurrences']}
    initial = inspect()
    definitions = {c['name']: c['id'] for c in initial['authored']['components']}
    occurrences = {o['definitionId']: o['id'] for o in initial['authored']['occurrences']}
    leaf = {name: occurrences[definitions[name]] for name in
            ('Lower arm', 'Upper arm', 'Inner piston', 'Outer piston')}
    parent = occurrences[definitions['Piston subassembly']]
    joints = call('joint.inspect')['joints']
    driver = next(j['id'] for j in joints if j['type'] == 'asBuiltRevolute')
    cylinder = next(j['id'] for j in joints if j['type'] == 'asBuiltCylindrical')
    call('joint.angleSet', {'jointId': driver, 'angleDeg': 0})
    call('position.revert')
    reference = inspect()['authored']
    copied = call('component.copyOccurrence', {'occurrenceId': parent,
        'placement': {'axis': 'Z', 'angleDeg': 0, 'translationMm': [-180, 0, 0]}})
    occurrence_map, joint_map = copied['occurrenceMap'], copied['jointMap']
    assert set(joint_map) == {cylinder}, 'Copy only the internal Cylindrical joint'
    after_copy = inspect()['authored']
    assert after_copy['features'] == reference['features'], 'Do not copy source geometry or Pin'
    assert after_copy['components'] == reference['components']
    assert after_copy['joints'][:len(reference['joints'])] == reference['joints']
    assert len(after_copy['joints']) == len(reference['joints']) + 1

    def arm_origin(name, x, z):
        body = next(b for b in initial['evaluation']['bodies'] if b['ownerId'] == definitions[name])
        for edge in body['solidEdges']:
            start, end, mid = (edge.get(k, []) for k in ('startMm', 'endMm', 'midpointMm'))
            if any(len(p) != 3 for p in (start, end, mid)):
                continue
            if math.dist(start, end) < 1e-6 and math.dist(start, mid) > 1 and max(
                    abs(start[0]-x), abs(mid[0]-x), abs(start[2]-z), abs(mid[2]-z)) < 7:
                return {'bodyId': body['bodyId'], 'edgeId': edge['edgeId']}
        raise AssertionError('Missing retained left arm mounting circle')

    for moving, fixed, z, name in (
            ('Inner piston', 'Lower arm', 0, 'Copied lower piston eye'),
            ('Outer piston', 'Upper arm', 180, 'Copied upper piston eye')):
        call('joint.revolute', {'name': name,
            'component1OccurrenceId': occurrence_map[leaf[moving]],
            'component2OccurrenceId': leaf[fixed],
            'position': arm_origin(fixed, -90, z), 'angleDeg': 0})
    state = call('joint.inspect')
    assert len(state['joints']) == 7 and len(state['islands']) == 1
    assert state['islands'][0]['pinAuthored']
    rest = poses(state)
    original = inspect()['authored']
    call('view.home')
    call('capture.screenshot', {'path': str(out/'two-pistons-at-rest.png')})
    samples = []
    for angle in (-15, 15):
        state = call('joint.previewAngle', {'jointId': driver, 'angleDeg': angle})
        posed = poses(state)
        for a, b in zip(posed[leaf['Lower arm']], rest[leaf['Lower arm']]):
            near(a, b, 'Complete fixed Lower pose')
        theta = math.radians(angle)
        sample = {'angleDeg': angle, 'pistons': []}
        for x, inner, outer, cid in (
                (90, leaf['Inner piston'], leaf['Outer piston'], cylinder),
                (-90, occurrence_map[leaf['Inner piston']], occurrence_map[leaf['Outer piston']], joint_map[cylinder])):
            upper = [x*math.cos(theta)+100*math.sin(theta), 0,
                     80-x*math.sin(theta)+100*math.cos(theta)]
            lower = [x, 0, 0]
            # Both instances retain the same definition-local eye centers.
            for a, b in zip(transform(posed[inner], [90, 0, 0]), lower):
                near(a, b, 'Inner eye remains on its own Lower mount')
            for a, b in zip(transform(posed[outer], [90, 0, 180]), upper):
                near(a, b, 'Outer eye follows its own Upper mount')
            travel = next(j['coordinateMm'] for j in state['joints'] if j['id'] == cid)
            near(travel, 180-math.dist(lower, upper), 'Independent telescoping travel')
            assert abs(travel) > 1
            sample['pistons'].append({'xMm': x, 'slideMm': travel, 'upperMountMm': upper})
        assert inspect()['authored'] == original
        call('capture.screenshot', {'path': str(out/f'two-pistons-angle-{angle}.png')})
        samples.append(sample)
        call('position.revert')
        reverted = poses(call('joint.inspect'))
        for key in rest:
            for a, b in zip(reverted[key], rest[key]): near(a, b, 'Revert all occurrences')
    call('joint.angleSet', {'jointId': driver, 'angleDeg': 15})
    committed = inspect()['authored']
    call('history.undo')
    assert inspect()['authored'] == original
    call('history.redo')
    assert inspect()['authored'] == committed
    saved = out/'copied-piston-mechanism.alloy'
    call('document.save', {'path': str(saved)})
    call('document.open', {'path': str(saved), 'discard': True})
    assert inspect()['authored'] == committed
    final = call('joint.inspect')
    near(next(j['coordinateDeg'] for j in final['joints'] if j['id'] == driver), 15, 'Reopened driver')
    call('view.home')
    call('capture.screenshot', {'path': str(out/'copied-piston-reopened.png')})
    (out/'receipt.json').write_text(json.dumps({'passed': True, 'samples': samples,
        'scope': 'Adapted six-leaf seven-joint copied piston; sampled kinematics, no collision/stroke-limit claim',
        'occurrenceMap': occurrence_map, 'jointMap': joint_map, 'ledger': ledger}, indent=2)+'\n')
    print('PASS copied internal joint, both piston travel, full Pin/Revert and history/reopen', flush=True)
