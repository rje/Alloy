#!/usr/bin/env python3
"""Real-app OBJ output with independent text/topology readback."""
import argparse
from collections import Counter
import json
import math
from pathlib import Path
from app_harness import application


def inspect_obj(path):
    vertices, groups = [], []
    for line in Path(path).read_text().splitlines():
        fields = line.split()
        if not fields or fields[0].startswith('#'):
            continue
        if fields[0] == 'v':
            assert len(fields) == 4
            point = tuple(map(float, fields[1:]))
            assert all(math.isfinite(x) for x in point)
            vertices.append(point)
        elif fields[0] == 'g':
            assert len(fields) == 2 and fields[1] not in [g['name'] for g in groups]
            groups.append({'name': fields[1], 'faces': []})
        elif fields[0] == 'f':
            assert len(fields) == 4 and groups
            triangle = tuple(int(v)-1 for v in fields[1:])
            assert len(set(triangle)) == 3 and all(0 <= i < len(vertices) for i in triangle)
            groups[-1]['faces'].append(triangle)
        else:
            raise AssertionError(f'Unexpected record: {line}')
    result = []
    for group in groups:
        edges = Counter()
        triangles = group['faces']
        assert triangles
        indices = {i for t in triangles for i in t}
        points = [vertices[i] for i in indices]
        origin = points[0]
        volume = 0
        for triangle in triangles:
            a, b, c = [[vertices[i][j]-origin[j] for j in range(3)] for i in triangle]
            ab, ac = [[v[j]-a[j] for j in range(3)] for v in (b, c)]
            cross = [ab[1]*ac[2]-ab[2]*ac[1], ab[2]*ac[0]-ab[0]*ac[2], ab[0]*ac[1]-ab[1]*ac[0]]
            assert sum(x*x for x in cross) > 0
            for u, v in zip(triangle, triangle[1:]+triangle[:1]):
                edges[u, v] += 1
            volume += sum(a[i]*(b[(i+1)%3]*c[(i+2)%3]-b[(i+2)%3]*c[(i+1)%3]) for i in range(3))/6
        assert all(count == 1 and edges[v, u] == 1 for (u, v), count in edges.items())
        assert volume > 0
        result.append({'group': group['name'], 'vertices': len(indices), 'triangles': len(triangles),
                       'volume': volume, 'bounds': {'min': [min(p[i] for p in points) for i in range(3)],
                                                  'max': [max(p[i] for p in points) for i in range(3)]}})
    return result


def close_bounds(observed, expected, scale=1):
    for side in ('min', 'max'):
        assert all(abs(a-b*scale) < 1e-5 for a, b in zip(observed[side], expected[side])), (observed, expected)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    out = Path(args.output).resolve()
    out.mkdir(parents=True, exist_ok=True)
    with application(args.app, out) as (call, transcript):
        root = call('document.inspect')['authored']['documentId']
        created = call('component.create', {'name': 'Mesh coupon'})
        definition, original = created['definitionId'], created['occurrenceId']
        call('component.activate', {'definitionId': definition})
        call('solid.box', {'width': 20, 'depth': 10, 'height': 5, 'units': 'mm', 'name': 'Coupon'})
        feature = next(f for f in call('document.inspect')['authored']['features'] if f['type'] == 'box')
        call('component.activate', {'definitionId': root})
        second = call('component.copyOccurrence', {'occurrenceId': original,
            'placement': {'axis': 'Z', 'angleDeg': 90, 'translationMm': [40, 10, 7]}})['rootOccurrenceId']
        before = call('document.inspect')
        settings = {'surfaceDeviationMm': 0.1, 'normalDeviationDeg': 10}
        def export(name, units, selection):
            path = out/name
            result = call('document.exportObj', {'path': str(path), **selection, 'units': units, **settings})
            assert result['format'] == 'obj' and result['units'] == units and not result['unitsEmbedded']
            assert result['axisConvention'] == 'worldZUp' and not result['materials']
            assert call('document.inspect') == before
            return result, inspect_obj(path)
        one_result, one = export('one-coupon-mm.obj', 'mm', {'occurrenceId': original})
        two_result, two = export('two-coupons-mm.obj', 'mm', {'allVisible': True})
        inch_result, inch = export('two-coupons-in.obj', 'in', {'allVisible': True})
        assert len(one) == 1 and len(two) == len(inch) == 2
        rows = {r['occurrenceId']: r for r in call('component.inspect')['localInstances']}
        expected = [rows[original]['bodies'][0]['boundsMm'], rows[second]['bodies'][0]['boundsMm']]
        for parsed, scale in ((two, 1), (inch, 1/25.4)):
            for item, bounds in zip(sorted(parsed, key=lambda i: i['bounds']['min'][0]), sorted(expected, key=lambda b: b['min'][0])):
                close_bounds(item['bounds'], bounds, scale)
                assert item['triangles'] == 12 and item['vertices'] == 8
                assert abs(item['volume']-1000*scale**3) < 1e-5
        original_bytes = (out/'two-coupons-mm.obj').read_bytes()
        export('two-coupons-repeat.obj', 'mm', {'allVisible': True})
        assert (out/'two-coupons-repeat.obj').read_bytes() == original_bytes
        call('document.exportObj', {'path': str(out/'invalid.obj'), 'allVisible': True, 'units': 'cm', **settings}, error='invalid_arguments')
        call('document.exportObj', {'path': str(out/'missing/invalid.obj'), 'allVisible': True, 'units': 'mm', **settings}, error='invalid_path')
        call('document.exportObj', {'path': str(out/'invalid.obj'), 'bodyId': 'missing', 'units': 'mm', **settings}, error='stale_body')
        assert call('document.inspect') == before
        call('feature.edit', {'featureId': feature['id'], 'parameters': {'widthMm': 25}})
        before = call('document.inspect')
        _, edited = export('two-coupons-edited.obj', 'mm', {'allVisible': True})
        assert len(edited) == 2 and all(abs(g['volume']-1250) < 1e-5 for g in edited)
        call('history.undo')
        assert call('document.inspect')['authored'] != before['authored']
        call('history.redo')
        assert call('document.inspect')['authored'] == before['authored']
        call('document.save', {'path': str(out/'coupon-source.alloy')})
        call('view.fit')
        call('capture.screenshot', {'path': str(out/'two-coupons.png')})
        gpu = call('app.state').get('viewport', {})
        (out/'receipt.json').write_text(json.dumps({'passed': True, 'one': one, 'two': two, 'inch': inch,
            'edited': edited, 'oneResult': one_result, 'twoResult': two_result, 'inchResult': inch_result,
            'viewport': gpu, 'transcript': transcript}, indent=2)+'\n')
    print('PASS real-app grouped OBJ, world placement, mm/in, deterministic bytes, source edit and topology readback')


if __name__ == '__main__':
    main()
