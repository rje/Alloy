#!/usr/bin/env python3
"""Actual-app circular-origin Revolute and retained root Pin on the native fixture."""
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
    lower_occ, upper_occ = occurrences[lower], occurrences[upper]
    body = next(b for b in initial['evaluation']['bodies'] if b['ownerId']==lower)
    def hinge(edge):
        start, end, mid = (edge.get(k, []) for k in ('startMm','endMm','midpointMm'))
        return (len(start)==len(end)==len(mid)==3 and
            math.dist(start,end)<1e-6 and math.dist(start,mid)>1 and
            max(abs(start[0]),abs(mid[0]))<5 and
            max(abs(start[2]-80),abs(mid[2]-80))<5)
    circles = [e for e in body['solidEdges'] if hinge(e)]
    assert circles, 'Retained Lower hinge circular edge missing'
    circle = circles[0]
    position = {'bodyId':body['bodyId'], 'edgeId':circle['edgeId']}
    before = initial['authored']
    joint = call('joint.revolute', {'component1OccurrenceId':upper_occ,
        'component2OccurrenceId':lower_occ, 'position':position, 'angleDeg':0})['joints'][0]
    joint_id = joint['id']
    assert inspect()['authored']['features']==before['features']
    assert joint['coordinateDeg']==0 and not joint['pinAuthored']
    call('joint.previewPosition', {'positionMm':12}, error='invalid_slider')
    assert call('joint.inspect')['joints'][0]['coordinateDeg']==0
    call('view.home')
    call('capture.screenshot', {'path':str(out/'revolute-rest.png')})
    preview = call('joint.previewAngle', {'angleDeg':15})
    assert abs(preview['joints'][0]['coordinateDeg']-15)<1e-8
    assert inspect()['authored']['joints'][0]['angleDeg']==0
    call('capture.screenshot', {'path':str(out/'revolute-preview.png')})
    call('joint.revoluteStop')
    assert call('joint.inspect')['joints'][0]['coordinateDeg']==0
    call('joint.angleSet', {'jointId':joint_id, 'angleDeg':-15})
    driven = call('joint.inspect')['joints'][0]
    assert abs(driven['coordinateDeg']+15)<1e-8 and driven['driveErrorRadians']<1e-7
    call('joint.pin', {'occurrenceId':lower_occ})
    pinned = inspect()['authored']
    assert pinned['features'][-1]['type']=='assemblyPin'
    assert call('joint.inspect')['joints'][0]['pinAuthored']
    call('capture.screenshot', {'path':str(out/'revolute-pinned.png')})
    call('history.undo')
    assert not call('joint.inspect')['joints'][0]['pinAuthored']
    call('history.redo')
    assert inspect()['authored']==pinned
    path = out/'piston-revolute.alloy'
    call('document.save', {'path':str(path)})
    call('document.open', {'path':str(path), 'discard':True})
    assert inspect()['authored']==pinned
    assert call('joint.inspect')['joints'][0]['pinAuthored']
    invalid = call('joint.revolute', {'jointId':joint_id, 'position':
        {'bodyId':position['bodyId'], 'edgeId':'00000000-0000-0000-0000-000000000000'}},
        error='invalid_revolute')
    assert inspect()['authored']==pinned
    (out/'receipt.json').write_text(json.dumps({'passed':True,
        'scope':'one retained root-leaf circular Revolute, exact signed drive, Pin feature, preview/history/reopen',
        'jointId':joint_id,'pinFeatureId':pinned['features'][-1]['id'],
        'ledger':ledger}, indent=2)+'\n')
    print('PASS retained circular Revolute, signed drive, Pin, preview/history/reopen', flush=True)
