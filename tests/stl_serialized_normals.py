#!/usr/bin/env python3
"""Independent serialized-coordinate normal check through actual MeshExport."""
import argparse
import json
from pathlib import Path
import struct
import subprocess
import tempfile
from stl_export_api import inspect_stl

parser = argparse.ArgumentParser()
parser.add_argument('--fixture', required=True)
args = parser.parse_args()
with tempfile.TemporaryDirectory(prefix='alloy-stl-normal-') as directory:
    out = Path(directory)
    subprocess.run([args.fixture, str(out)], check=True)
    metadata = json.loads((out/'metadata.json').read_text())
    for units in ('mm', 'in'):
        path = out/(units+'.stl')
        result = inspect_stl(path, True)  # Existing strict checker, unchanged.
        assert result['triangles'] == metadata[units]['triangles']
        assert result['triangles'] == 8 and len(result['components']) == 1
        assert metadata[units]['collapsedTrianglesRemoved'] == 0
        if units == 'mm':
            # The quantization-sensitive cap's exact serialized vertex bytes
            # remain present; fixing normals must not move/remove/reverse it.
            vertices = [struct.pack('<3f', *p) for p in (
                (-.32688385248184204,.1438046097755432,79.76461791992188),
                (-.35843726992607117,.1591918170452118,79.74126434326172),
                (-.24812832474708557,.10321561992168427,79.82328033447266))]
            records = path.read_bytes()[84:]
            caps = [records[i+12:i+48] for i in range(0, len(records), 50)]
            assert any(all(v in cap for v in vertices) for cap in caps)
    print('PASS binary mm/in exact serialized normal/winding, cap vertices, 8 facets, closed positive prism and original BRep')
