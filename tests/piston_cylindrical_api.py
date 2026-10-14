#!/usr/bin/env python3
"""Actual-app as-built Cylindrical on the native nested piston pair."""
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

with application(args.app, out, command_timeout=90) as (call, ledger):
    call('document.open', {'path':str(Path(args.fixture).resolve()), 'discard':True})
    def inspect(): return call('document.inspect')
    initial = inspect()
    components = {c['name']:c['id'] for c in initial['authored']['components']}
    occurrences = {o['definitionId']:o['id'] for o in initial['authored']['occurrences']}
    lower, upper = components['Lower arm'], components['Upper arm']
    inner, outer = components['Inner piston'], components['Outer piston']
    def body(owner): return next(b for b in inspect()['evaluation']['bodies'] if b['ownerId']==owner)
    def circle(owner, x, z, plane):
        source=body(owner)
        for edge in source['solidEdges']:
            start,end,mid=(edge.get(k,[]) for k in ('startMm','endMm','midpointMm'))
            if len(start)!=len(end) or len(start)!=3 or len(mid)!=3 or math.dist(start,end)>1e-6 or math.dist(start,mid)<1:
                continue
            if plane=='XZ' and max(abs(start[0]-x),abs(mid[0]-x),abs(start[2]-z),abs(mid[2]-z))<7:
                return {'bodyId':source['bodyId'],'edgeId':edge['edgeId']}
            if plane=='XY' and abs(start[2]-z)<1e-5 and abs(mid[2]-z)<1e-5 and abs(start[0]-x)<7:
                return {'bodyId':source['bodyId'],'edgeId':edge['edgeId']}
        raise AssertionError(f'No retained {plane} circular origin on {source["name"]}')
    hinge = circle(lower, 0, 80, 'XZ')
    call('joint.revolute', {'component1OccurrenceId':occurrences[upper],
        'component2OccurrenceId':occurrences[lower], 'position':hinge, 'angleDeg':0})
    zcircle = circle(outer, 90, 80, 'XY')
    before = inspect()['authored']
    created = call('joint.cylindrical', {'component1OccurrenceId':occurrences[inner],
        'component2OccurrenceId':occurrences[outer], 'position':zcircle,
        'rotationDeg':0, 'slideMm':0})
    cyl = next(j for j in created['joints'] if j['type']=='asBuiltCylindrical')
    arm_joint = next(j for j in created['joints'] if j['type']=='asBuiltRevolute')
    joint_id = cyl['id']
    call('joint.previewCylindrical', {'jointId':arm_joint['id'],
        'rotationDeg':15, 'slideMm':5}, error='invalid_cylindrical')
    assert cyl['relativeDegreesOfFreedom']==2
    assert abs(cyl['coordinateDeg'])<1e-7 and abs(cyl['coordinateMm'])<1e-7
    def inner_z(assembly):
        occurrence=next(o for o in assembly['occurrences'] if o['occurrenceId']==occurrences[inner])
        return occurrence['bodies'][0]['boundsMm']['min'][2]
    rest_z=inner_z(created)
    assert inspect()['authored']['features']==before['features']
    call('view.home')
    call('capture.screenshot', {'path':str(out/'cylindrical-as-built.png')})
    original = inspect()['authored']
    staged = call('joint.previewCylindrical', {'jointId':joint_id, 'rotationDeg':15, 'slideMm':5})
    moving = next(j for j in staged['joints'] if j['id']==joint_id)
    assert abs(moving['coordinateDeg']-15)<1e-6 and abs(moving['coordinateMm']-5)<1e-6
    assert abs((inner_z(staged)-rest_z)-5)<1e-5, 'Inner piston must actually slide 5 mm in world Z'
    assert inspect()['authored']==original
    call('capture.screenshot', {'path':str(out/'cylindrical-preview.png')})
    call('joint.cylindricalStop')
    reverted = next(j for j in call('joint.inspect')['joints'] if j['id']==joint_id)
    assert abs(reverted['coordinateDeg'])<1e-7 and abs(reverted['coordinateMm'])<1e-7
    call('history.undo')
    assert inspect()['authored']==before
    call('history.redo')
    assert inspect()['authored']==original
    path=out/'piston-cylindrical.alloy'
    call('document.save', {'path':str(path)})
    call('document.open', {'path':str(path), 'discard':True})
    assert inspect()['authored']==original
    reopened=next(j for j in call('joint.inspect')['joints'] if j['id']==joint_id)
    assert reopened['relativeDegreesOfFreedom']==2
    (out/'receipt.json').write_text(json.dumps({'passed':True,
        'scope':'nested piston Cylindrical as-built origin; exact independent transient rotation and slide; history/reopen',
        'jointId':joint_id,'ledger':ledger},indent=2)+'\n')
    print('PASS nested piston Cylindrical preview, Cancel/Revert, history and reopen',flush=True)
