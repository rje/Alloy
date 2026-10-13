#!/usr/bin/env python3
"""Real-entrypoint STL export with independent stdlib triangle readback."""
import argparse
from collections import Counter
import json
import math
from pathlib import Path
import re
import struct
from app_harness import application


def inspect_stl(path, binary):
    data = Path(path).read_bytes()
    if binary:
        count, = struct.unpack_from('<I', data, 80)
        assert len(data) == 84+50*count
        triangles, normals = [], []
        for i in range(count):
            values = struct.unpack_from('<12fH', data, 84+50*i)
            triangles.append([values[3:6], values[6:9], values[9:12]])
            normals.append(values[:3])
    else:
        vertices = [tuple(map(float, v)) for v in re.findall(r'vertex\s+(\S+)\s+(\S+)\s+(\S+)', data.decode('ascii'))]
        assert len(vertices) % 3 == 0
        triangles = [vertices[i:i+3] for i in range(0, len(vertices), 3)]
        normals = [tuple(map(float, n)) for n in re.findall(r'facet normal\s+(\S+)\s+(\S+)\s+(\S+)', data.decode('ascii'))]
    assert triangles
    assert len(normals) == len(triangles)
    points = [p for t in triangles for p in t]
    low = [min(p[i] for p in points) for i in range(3)]
    high = [max(p[i] for p in points) for i in range(3)]
    tolerance = max(max(high[i]-low[i] for i in range(3))*1e-7, 1e-9)
    key = lambda p: tuple(round(x/tolerance) for x in p)
    edges = Counter()
    volume = 0
    neighbors = {}
    triangle_volumes = []
    for index, ((a, b, c), normal) in enumerate(zip(triangles, normals)):
        assert all(math.isfinite(x) for p in (a, b, c, normal) for x in p)
        ab, ac = [b[i]-a[i] for i in range(3)], [c[i]-a[i] for i in range(3)]
        cross = [ab[(i+1)%3]*ac[(i+2)%3]-ab[(i+2)%3]*ac[(i+1)%3] for i in range(3)]
        cross_length = math.sqrt(sum(x*x for x in cross))
        normal_length = math.sqrt(sum(x*x for x in normal))
        assert cross_length > 0 and math.isclose(normal_length, 1, rel_tol=1e-5)
        assert sum(cross[i]*normal[i] for i in range(3))/(cross_length*normal_length) > 0.99999, 'Facet normal disagrees with winding'
        ka, kb, kc = key(a), key(b), key(c)
        assert len({ka, kb, kc}) == 3, 'Degenerate output triangle'
        for u, v in ((ka, kb), (kb, kc), (kc, ka)):
            edges[u, v] += 1
        for vertex in (ka, kb, kc):
            neighbors.setdefault(vertex, []).append(index)
        triangle_volume = sum(a[i]*(b[(i+1)%3]*c[(i+2)%3]-b[(i+2)%3]*c[(i+1)%3]) for i in range(3))/6
        triangle_volumes.append(triangle_volume)
        volume += triangle_volume
    components = []
    remaining = set(range(len(triangles)))
    while remaining:
        pending = [remaining.pop()]; component = []
        while pending:
            index = pending.pop(); component.append(index)
            for point in triangles[index]:
                for adjacent in neighbors[key(point)]:
                    if adjacent in remaining:
                        remaining.remove(adjacent); pending.append(adjacent)
        points_in_component = [p for i in component for p in triangles[i]]
        signed_volume = sum(triangle_volumes[i] for i in component)
        assert signed_volume > 0, "Each component must have positive signed volume"
        components.append({"triangles": len(component), "bounds": [[min(p[i] for p in points_in_component) for i in range(3)], [max(p[i] for p in points_in_component) for i in range(3)]], "signedVolume": signed_volume})
    assert all(count == 1 and edges[v, u] == 1 for (u, v), count in edges.items()), 'Open or inconsistently oriented mesh'
    assert volume > 0, 'Mesh winding must have positive signed volume'
    return {'components': components, 'triangles': len(triangles), 'bounds': [low, high], 'signedVolume': volume, 'facetNormals': 'finite, unit length and aligned with winding'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    output = Path(args.output).resolve(); output.mkdir(parents=True, exist_ok=True)
    evidence = []
    with application(args.app, output) as (call, transcript):
        call('solid.box', {'width': 25.4, 'depth': 50.8, 'height': 12.7, 'x': 2.54, 'units': 'mm'})
        doc = call('document.inspect'); body = doc['evaluation']['bodies'][0]['bodyId']
        original = doc['authored']; history = doc['undo']
        def export(name, body_id, units='mm', binary=True, surface=0.1, angle=10):
            path = output/name
            result = call('document.exportStl', {'path': str(path), 'bodyId': body_id, 'units': units,
                'format': 'binary' if binary else 'ascii', 'surfaceDeviationMm': surface, 'normalDeviationDeg': angle})
            mesh = inspect_stl(path, binary)
            assert mesh['triangles'] == result['triangles']
            evidence.append({'file': name, 'result': result, 'mesh': mesh})
            return mesh
        for units, binary in [('mm', True), ('in', False)]:
            m = export('box-'+units+'.stl', body, units, binary)
            scale = 1 if units == 'mm' else 1/25.4
            for observed, expected in zip(m['bounds'][0], [2.54*scale, 0, 0]):
                assert math.isclose(observed, expected, abs_tol=1e-5)
            for observed, expected in zip(m['bounds'][1], [27.94*scale, 50.8*scale, 12.7*scale]):
                assert math.isclose(observed, expected, abs_tol=1e-5)
            assert math.isclose(m['signedVolume'], 25.4*50.8*12.7*scale**3, rel_tol=1e-6)
        assert call('document.inspect')['authored'] == original
        assert call('document.inspect')['undo'] == history
        sentinel = output/'preserved.stl'; sentinel.write_bytes(b'preserve-existing-file')
        settings = {'path': str(sentinel), 'bodyId': 'stale', 'units': 'mm', 'format': 'binary', 'surfaceDeviationMm': 0.1, 'normalDeviationDeg': 10}
        call('document.exportStl', settings, error='stale_body')
        call('sketch.begin', {'plane': 'XY'})
        call('document.exportStl', dict(settings, bodyId=body), error='active_command')
        assert sentinel.read_bytes() == b'preserve-existing-file'
        call('sketch.cancel')
        call('solid.cylinder', {'radius': 10, 'height': 20, 'x': 60, 'units': 'mm'})
        cylinder = call('document.inspect')['evaluation']['bodies'][-1]['bodyId']
        coarse = export('cylinder-coarse.stl', cylinder, surface=0.5, angle=30)
        fine = export('cylinder-fine.stl', cylinder, surface=0.05, angle=5)
        assert fine['triangles'] > coarse['triangles']
        exact = math.pi*10**2*20
        assert abs(fine['signedVolume']-exact) < abs(coarse['signedVolume']-exact)
        assert math.isclose(fine['signedVolume'], exact, rel_tol=0.005)
        call('view.home')
        call('capture.screenshot', {'path': str(output/'source-solids.png')})
        (output/'stl-export-api.json').write_text(json.dumps({'result': 'passed', 'exports': evidence,
            'limits': ['API export and independent mesh evidence; native dialog operation is separate.', 'One closed solid per export.'],
            'transcript': transcript}, indent=2)+'\n')
    print('PASS: STL units, refinement, closed oriented meshes, preserved document and refusal paths')


if __name__ == '__main__':
    main()
