#!/usr/bin/env python3
"""Run only in the serialized private-editor slot; inspect actual Metal results."""
import argparse
import json
from pathlib import Path
import subprocess

p = argparse.ArgumentParser()
p.add_argument('--app', required=True)
p.add_argument('--output', required=True)
a = p.parse_args()
out = Path(a.output).resolve()
out.mkdir(parents=True, exist_ok=True)
with (out / 'app.log').open('w') as log:
    subprocess.run([str(Path(a.app).resolve()), '--self-test', '--output', str(out)],
                   stdout=log, stderr=subprocess.STDOUT, check=True, timeout=45)
r = json.loads((out / 'report.json').read_text())
assert r['passed'] and r['applicationExitCode'] == 0 and r['cleanWindowTeardown']
assert r['finalState']['backend'] == 'Metal' and r['finalState']['deviceName']
assert r['finalState']['idSamples'] == 1
assert all(pick['code'] == pick['expectedCode'] for pick in r['picks'])
assert next(pick for pick in r['picks'] if pick['purpose'] == 'background')['code'] == 0
assert next(pick for pick in r['picks'] if pick['purpose'] == 'depth_occlusion')['rayObjectIds'] == ['cylinder', 'box']
assert r['pendingReadbackRelease']['submittedResultPresent']
assert r['cancellation']['error'] == 'readback_canceled_for_resource_release'
assert r['finalState']['resourceCreations'] >= 5
for state in r['snapshots']:
    assert all(round(state['logicalSize'][i] * state['devicePixelRatio']) == state['pixelSize'][i] for i in range(2))
print('PASS: actual Metal fixture, face IDs/depth/miss, capture, resize and cancellation/recreation')
