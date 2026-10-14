#!/usr/bin/env python3
"""Real-app four-joint piston workflow; independent fixture kinematics checks."""
import argparse
import json
import math
from pathlib import Path

from app_harness import application

parser = argparse.ArgumentParser()
parser.add_argument('--app', required=True)
parser.add_argument('--fixture', required=True)
parser.add_argument('--output', required=True)
args = parser.parse_args()
out = Path(args.output).resolve()
out.mkdir(parents=True, exist_ok=True)


def near(actual, expected, message, tolerance=1e-5):
    assert math.isfinite(actual) and abs(actual-expected) < tolerance, (message, actual, expected)


def transformed(pose, point):
    return [sum(pose[4*r+c]*point[c] for c in range(3))+pose[4*r+3] for r in range(3)]


with application(args.app, out, command_timeout=90) as (call, ledger):
    call('document.open', {'path':str(Path(args.fixture).resolve()), 'discard':True})
    def inspect(): return call('document.inspect')
    initial = inspect()
    definitions = {c['name']:c['id'] for c in initial['authored']['components']}
    occurrences = {o['definitionId']:o['id'] for o in initial['authored']['occurrences']}
    leaf = {name:occurrences[definitions[name]] for name in
            ('Lower arm', 'Upper arm', 'Inner piston', 'Outer piston')}

    def origin(name, x, z, plane):
        body = next(b for b in initial['evaluation']['bodies'] if b['ownerId']==definitions[name])
        for edge in body['solidEdges']:
            start,end,mid = (edge.get(k,[]) for k in ('startMm','endMm','midpointMm'))
            if any(len(p)!=3 for p in (start,end,mid)) or math.dist(start,end)>1e-6 or math.dist(start,mid)<1:
                continue
            if plane=='XZ' and max(abs(start[0]-x),abs(mid[0]-x),abs(start[2]-z),abs(mid[2]-z))<7:
                return {'bodyId':body['bodyId'],'edgeId':edge['edgeId']}
            if plane=='XY' and abs(start[2]-z)<1e-5 and abs(mid[2]-z)<1e-5 and abs(start[0]-x)<7:
                return {'bodyId':body['bodyId'],'edgeId':edge['edgeId']}
        raise AssertionError(f'Missing retained {plane} circle on {name}')

    def revolute(moving, fixed, x, z, name):
        return call('joint.revolute', {'name':name,'component1OccurrenceId':leaf[moving],
            'component2OccurrenceId':leaf[fixed], 'position':origin(fixed,x,z,'XZ'),'angleDeg':0})

    state = revolute('Upper arm','Lower arm',0,80,'Arm hinge')
    driver_id = state['joints'][0]['id']
    state = call('joint.cylindrical', {'name':'Piston slide',
        'component1OccurrenceId':leaf['Inner piston'], 'component2OccurrenceId':leaf['Outer piston'],
        'position':origin('Outer piston',90,80,'XY'), 'rotationDeg':0,'slideMm':0})
    assert len(state['islands'])==2 and all(not i['pinAuthored'] for i in state['islands'])
    cylinder_id = next(j['id'] for j in state['joints'] if j['type']=='asBuiltCylindrical')
    revolute('Inner piston','Lower arm',90,0,'Lower piston eye')
    state = revolute('Outer piston','Upper arm',90,180,'Upper piston eye')
    assert len(state['joints'])==4 and len(state['islands'])==1
    assert not state['islands'][0]['pinAuthored'], 'Do not author Pin before its source step'
    call('joint.pin', {'occurrenceId':leaf['Lower arm']})
    call('view.home')
    original = inspect()['authored']
    rest = call('joint.inspect')

    def poses(state):
        return {r['occurrenceId']:r['worldPose'] for r in state['occurrences'] if r['occurrenceId'] in leaf.values()}

    rest_poses = poses(rest)
    samples = []
    for angle in (-15,15):
        state = call('joint.previewAngle', {'angleDeg':angle})
        placement = poses(state)
        assert state['islands'][0]['pinAuthored']
        for a,b in zip(placement[leaf['Lower arm']],rest_poses[leaf['Lower arm']]):
            near(a,b,'Full Lower Pin pose')
        for joint in state['joints']:
            near(joint['maxOriginErrorMm'],0,'Original joint origin residual')
            near(joint['maxAxisErrorRadians'],0,'Original joint axis residual',1e-6)
        theta = math.radians(angle)
        expected_upper = [90*math.cos(theta)+100*math.sin(theta),0,
                          80-90*math.sin(theta)+100*math.cos(theta)]
        actual_upper = transformed(placement[leaf['Upper arm']], [90,0,180])
        for a,b in zip(actual_upper,expected_upper): near(a,b,'Analytical upper mounting axis')
        inner_eye = transformed(placement[leaf['Inner piston']], [90,0,0])
        outer_eye = transformed(placement[leaf['Outer piston']], [90,0,180])
        for a,b in zip(inner_eye,[90,0,0]): near(a,b,'Inner end-eye stays on Lower axis')
        for a,b in zip(outer_eye,expected_upper): near(a,b,'Outer end-eye follows Upper axis')
        distance = math.dist([90,0,0],expected_upper)
        cylinder = next(j for j in state['joints'] if j['id']==cylinder_id)
        near(cylinder['coordinateMm'],180-distance,'Independent telescoping travel')
        assert abs(cylinder['coordinateMm'])>1, 'Exercise meaningful piston movement'
        assert inspect()['authored']==original
        call('capture.screenshot', {'path':str(out/f'piston-angle-{angle}.png')})
        samples.append({'angleDeg':angle,'mountDistanceMm':distance,'slideMm':cylinder['coordinateMm'],
                        'worldPoses':placement})
        call('joint.revoluteStop')
        reverted = poses(call('joint.inspect'))
        for key in rest_poses:
            for a,b in zip(reverted[key],rest_poses[key]): near(a,b,'Revert every leaf pose')
    call('joint.angleSet', {'jointId':driver_id,'angleDeg':15})
    committed = inspect()['authored']
    call('history.undo')
    assert inspect()['authored']==original
    call('history.redo')
    assert inspect()['authored']==committed
    path = out/'piston-coupled.alloy'
    call('document.save', {'path':str(path)})
    call('document.open', {'path':str(path),'discard':True})
    assert inspect()['authored']==committed
    reopened = call('joint.inspect')
    near(next(j for j in reopened['joints'] if j['id']==driver_id)['coordinateDeg'],15,'Reopened hinge drive')
    call('view.home')
    call('capture.screenshot', {'path':str(out/'piston-coupled-reopened.png')})
    (out/'receipt.json').write_text(json.dumps({'passed':True,'samples':samples,
        'scope':'Actual retained 3R+1C+Pin, analytical mounting positions and telescoping travel, full preview Revert/history/reopen; pointer drag separate',
        'ledger':ledger},indent=2)+'\n')
    print('PASS actual coupled piston, signed motion, full Pin/Revert and history/reopen',flush=True)
