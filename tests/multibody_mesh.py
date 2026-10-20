#!/usr/bin/env python3
"""Independent triangle readback of OCCT multi-body output; no editor launch."""
import argparse
import math
import json
import zipfile
import xml.etree.ElementTree as ET
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
    report = json.loads((Path(directory)/'batch.json').read_text())
    ns = {'m':'http://schemas.microsoft.com/3dmanufacturing/core/2015/02'}
    for fmt, batch in report.items():
        assert batch['bodyCount'] == 2 and batch['triangles'] == 24
        assert [r['bodyId'] for r in batch['files']] == ['first', 'second']
        assert len(list(Path(batch['path']).iterdir())) == 2
        for row, low, high in zip(batch['files'], [(-5,2,3),(40,-5,7)], [(15,12,18),(52,7,19)]):
            assert row['filename'].isascii()
            if fmt != '3mf':
                mesh = inspect_stl(Path(row['path']), fmt == 'binary')
                assert len(mesh['components']) == 1
                bounds = mesh['components'][0]['bounds']
            else:
                with zipfile.ZipFile(row['path']) as archive:
                    model = ET.fromstring(archive.read('3D/3dmodel.model'))
                assert model.attrib['unit'] == 'millimeter'
                objects = model.findall('m:resources/m:object',ns); items = model.findall('m:build/m:item',ns)
                assert len(objects) == len(items) == 1
                assert len(objects[0].findall('m:mesh/m:triangles/m:triangle',ns)) == 12
                transform = list(map(float,items[0].attrib.get('transform','1 0 0 0 1 0 0 0 1 0 0 0').split()))
                vertices = [tuple(float(v.attrib[a]) for a in ('x','y','z')) for v in objects[0].findall('m:mesh/m:vertices/m:vertex',ns)]
                points = [tuple(sum(v[j]*transform[3*j+i] for j in range(3))+transform[9+i] for i in range(3)) for v in vertices]
                bounds = [[min(p[a] for p in points) for a in range(3)], [max(p[a] for p in points) for a in range(3)]]
            for observed, expected in zip(bounds, [low,high]):
                assert all(math.isclose(x,y,abs_tol=1e-6) for x,y in zip(observed,expected)), (fmt,row,bounds)
    assert len(inspect_stl(Path(directory)/'single.stl', True)['components']) == 1
print('PASS: two closed oriented components, distinct placement, units, legacy body and atomic rejection')
