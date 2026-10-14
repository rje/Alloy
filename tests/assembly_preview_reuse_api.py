#!/usr/bin/env python3
"""Bounded actual-app coupled preview and Revert observation."""
import argparse
import json
import re
from pathlib import Path
from app_harness import application

parser = argparse.ArgumentParser()
parser.add_argument('--app', required=True)
parser.add_argument('--fixture', required=True)
parser.add_argument('--output', required=True)
args = parser.parse_args()
out = Path(args.output).resolve()
out.mkdir(parents=True, exist_ok=True)

with application(args.app, out, command_timeout=45) as (call, ledger):
    call('document.open', {'path': str(Path(args.fixture).resolve()), 'discard': True})
    original = call('document.inspect')['authored']
    initial = call('joint.inspect')
    assert len(initial['joints']) == 7 and len(initial['occurrences']) == 8
    driver = next(j['id'] for j in initial['joints'] if j['type'] == 'asBuiltRevolute')
    rest = {row['occurrenceId']: row['worldPose'] for row in initial['occurrences']}
    samples = []
    for angle in (-15, 15):
        state = call('joint.previewAngle', {'jointId': driver, 'angleDeg': angle})
        joints = state['joints']
        assert len(joints) == 7 and all(j['maxOriginErrorMm'] < 1e-5 for j in joints)
        assert all(j['maxAxisErrorRadians'] < 1e-6 for j in joints)
        assert abs(next(j for j in joints if j['id'] == driver)['coordinateDeg'] - angle) < 1e-5
        samples.append({'angleDeg': angle, 'jointCount': len(joints),
                        'occurrenceCount': len(state['occurrences'])})
        assert call('document.inspect')['authored'] == original
        call('position.revert')
        reverted = call('joint.inspect')
        assert {r['occurrenceId']: r['worldPose'] for r in reverted['occurrences']} == rest
    call('joint.previewAngle', {'jointId': driver, 'angleDeg': 181}, error='invalid_arguments')
    assert call('document.inspect')['authored'] == original
traces = [tuple(map(int, match.groups())) for match in re.finditer(
    r'assembly_preview_solves joint\.previewAngle command=(\d+) refresh=(\d+)',
    (out / 'app.log').read_text())]
assert traces == [(1, 0), (1, 0)], traces
(out / 'result.json').write_text(json.dumps({'samples': samples,
    'previewSolveCounts': [{'command': a, 'refresh': b} for a, b in traces],
    'ledger': ledger}, indent=2))
