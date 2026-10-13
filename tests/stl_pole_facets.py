#!/usr/bin/env python3
"""Strict binary readback of a four-feature spherical-pole regression fixture."""
import argparse
import json
import math
from pathlib import Path
import struct
import subprocess
import tempfile
from stl_export_api import inspect_stl

p = argparse.ArgumentParser()
p.add_argument('--fixture', required=True)
a = p.parse_args()
with tempfile.TemporaryDirectory(prefix='alloy-stl-pole-') as directory:
    authored = Path(__file__).parent / 'fixtures' / 'spherical-pole.alloy'
    before = authored.read_bytes()
    subprocess.run([a.fixture, directory, str(authored.resolve())], check=True, capture_output=True)
    metadata = json.loads((Path(directory) / 'metadata.json').read_text())
    for units, scale in [('mm', 1), ('in', 1 / 25.4)]:
        path = Path(directory) / f'pole-{units}.stl'
        mesh = inspect_stl(path, True)  # Existing checker stays unchanged.
        result = metadata[units]
        count, = struct.unpack_from('<I', path.read_bytes(), 80)
        assert result['serializedCollapsedTrianglesRemoved'] > 0
        assert result['collapsedTrianglesRemoved'] >= result['serializedCollapsedTrianglesRemoved']
        assert result['triangles'] == count == mesh['triangles']
        assert result['bytes'] == path.stat().st_size == 84 + 50 * count
        assert len(mesh['components']) == 1
        assert all(math.isclose(mesh['bounds'][1][i] - mesh['bounds'][0][i], 20 * scale, abs_tol=1e-6) for i in range(3))
        assert math.isclose(mesh['signedVolume'] / scale**3, metadata['brep']['volumeMm3'], rel_tol=2e-5)
        assert result['surfaceDeviationMm'] == .05 and result['normalDeviationDeg'] == 8
        assert result['authoredChanged'] is False
    # Preserve the existing ASCII branch and its metadata; strict curved ASCII
    # quality is outside this binary-facet correction. Existing multibody_mesh
    # separately verifies unchanged ASCII boxes through the strict reader.
    ascii_path = Path(directory) / 'pole-mm-ascii.stl'
    ascii_result = metadata['ascii']
    assert ascii_result['serializedCollapsedTrianglesRemoved'] == 0
    assert ascii_result['collapsedTrianglesRemoved'] == 0
    assert ascii_result['bytes'] == ascii_path.stat().st_size
    assert ascii_result['triangles'] == ascii_path.read_text().count('facet normal')
    assert authored.read_bytes() == before
print('PASS: actual serialized pole removal, unchanged strict normals/manifold/winding, mm/in, metadata and authored storage')
