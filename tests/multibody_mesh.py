#!/usr/bin/env python3
"""Independent triangle readback of OCCT multi-body output; no editor launch."""
import argparse
import math
from pathlib import Path
import subprocess
import tempfile
from stl_export_api import inspect_stl

parser = argparse.ArgumentParser()
parser.add_argument('--fixture', required=True)
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix='alloy-multi-stl-') as directory:
    subprocess.run([args.fixture, directory], check=True)
    for filename, binary, scale in [('bodies-mm.stl', True, 1), ('bodies-in.stl', False, 1/25.4)]:
        mesh = inspect_stl(Path(directory)/filename, binary)
        components = sorted(mesh['components'], key=lambda c: c['bounds'][0][0])
        assert len(components) == 2
        for observed, low, high, volume in zip(components, [(-5,2,3),(40,-5,7)], [(15,12,18),(52,7,19)], [3000,1728]):
            assert observed['triangles'] == 12
            assert math.isclose(observed['signedVolume'], volume*scale**3, rel_tol=1e-6)
            for axis in range(3):
                assert math.isclose(observed['bounds'][0][axis], low[axis]*scale, abs_tol=1e-6)
                assert math.isclose(observed['bounds'][1][axis], high[axis]*scale, abs_tol=1e-6)
    assert len(inspect_stl(Path(directory)/'single.stl', True)['components']) == 1
print('PASS: two closed oriented components, distinct placement, units, legacy body and atomic rejection')
