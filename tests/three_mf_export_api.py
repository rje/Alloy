#!/usr/bin/env python3
"""Real-app 3MF export with independent ZIP/XML and indexed-mesh readback."""
import argparse
from collections import Counter
import json
import math
from pathlib import Path
import xml.etree.ElementTree as ET
import zipfile

from app_harness import application


NS = {'m': 'http://schemas.microsoft.com/3dmanufacturing/core/2015/02'}


def inspect_3mf(path):
    with zipfile.ZipFile(path) as package:
        assert package.testzip() is None
        models = [name for name in package.namelist() if name.lower().endswith('.model')]
        assert len(models) == 1, models
        model = ET.fromstring(package.read(models[0]))
    assert model.attrib['unit'] == 'millimeter'
    objects = {}
    for obj in model.findall('m:resources/m:object', NS):
        mesh = obj.find('m:mesh', NS)
        assert mesh is not None and obj.attrib['type'] == 'model'
        vertices = [tuple(float(v.attrib[k]) for k in ('x', 'y', 'z'))
                    for v in mesh.findall('m:vertices/m:vertex', NS)]
        triangles = [tuple(int(t.attrib[k]) for k in ('v1', 'v2', 'v3'))
                     for t in mesh.findall('m:triangles/m:triangle', NS)]
        assert vertices and triangles
        assert all(len(set(t)) == 3 and all(0 <= i < len(vertices) for i in t) for t in triangles)
        edges = Counter()
        volume = 0.0
        for a, b, c in triangles:
            pa, pb, pc = vertices[a], vertices[b], vertices[c]
            ab = [pb[i] - pa[i] for i in range(3)]
            ac = [pc[i] - pa[i] for i in range(3)]
            cross = [ab[1]*ac[2]-ab[2]*ac[1], ab[2]*ac[0]-ab[0]*ac[2], ab[0]*ac[1]-ab[1]*ac[0]]
            assert all(math.isfinite(x) for p in (pa, pb, pc) for x in p)
            assert math.sqrt(sum(x*x for x in cross)) > 1e-9
            for u, v in ((a, b), (b, c), (c, a)):
                edges[u, v] += 1
            volume += sum(pa[i] * (pb[(i+1)%3]*pc[(i+2)%3] - pb[(i+2)%3]*pc[(i+1)%3])
                          for i in range(3)) / 6
        assert all(count == 1 and edges[v, u] == 1 for (u, v), count in edges.items())
        assert volume > 0
        objects[obj.attrib['id']] = {'vertices': vertices, 'triangles': len(triangles), 'volumeMm3': volume}

    items = []
    for item in model.findall('m:build/m:item', NS):
        obj = objects[item.attrib['objectid']]
        transform = [float(x) for x in item.attrib.get('transform', '1 0 0 0 1 0 0 0 1 0 0 0').split()]
        assert len(transform) == 12 and all(math.isfinite(x) for x in transform)
        # Core build-item transform is a 4x3 row-vector matrix; the last row translates.
        points = [[sum(p[r]*transform[r*3+c] for r in range(3)) + transform[9+c]
                   for c in range(3)] for p in obj['vertices']]
        items.append({'objectId': item.attrib['objectid'], 'transform': transform,
                      'boundsMm': {'min': [min(p[i] for p in points) for i in range(3)],
                                   'max': [max(p[i] for p in points) for i in range(3)]},
                      'triangles': obj['triangles'], 'volumeMm3': obj['volumeMm3']})
    assert len(objects) == len(items)
    return {'unit': 'millimeter', 'objects': len(objects), 'buildItems': items}


def close_bounds(observed, expected, tolerance=0.05):
    for side in ('min', 'max'):
        assert all(abs(a-b) < tolerance for a, b in zip(observed[side], expected[side])), (observed, expected)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    out = Path(args.output).resolve()
    out.mkdir(parents=True, exist_ok=True)
    with application(args.app, out) as (call, transcript):
        root = call('document.inspect')['authored']['documentId']
        created = call('component.create', {'name': 'Print coupon'})
        definition, original = created['definitionId'], created['occurrenceId']
        call('component.activate', {'definitionId': definition})
        call('solid.box', {'width': 20, 'depth': 10, 'height': 5, 'units': 'mm', 'name': 'Coupon'})
        first = call('document.inspect')
        feature = next(f for f in first['authored']['features'] if f['type'] == 'box')
        call('component.activate', {'definitionId': root})
        copied = call('component.copyOccurrence', {'occurrenceId': original,
            'placement': {'axis': 'Z', 'angleDeg': 90, 'translationMm': [40, 10, 0]}})
        second = copied['rootOccurrenceId']
        before = call('document.inspect')
        def export(name, selection):
            path = out/name
            result = call('document.export3mf', {'path': str(path), **selection,
                'surfaceDeviationMm': 0.1, 'normalDeviationDeg': 10})
            assert result['format'] == '3mf' and result['units'] == 'mm'
            after = call('document.inspect')
            assert after['authored'] == before['authored'] and after['undo'] == before['undo']
            assert after['revision'] == before['revision']
            return result, inspect_3mf(path)
        one_result, one = export('one-coupon.3mf', {'occurrenceId': original})
        assert one['objects'] == len(one['buildItems']) == 1 and one_result['bodyCount'] == 1
        two_result, two = export('two-coupons.3mf', {'allVisible': True})
        assert two['objects'] == len(two['buildItems']) == 2 and two_result['bodyCount'] == 2
        rows = {r['occurrenceId']: r for r in call('component.inspect')['localInstances']}
        expected = [rows[original]['bodies'][0]['boundsMm'], rows[second]['bodies'][0]['boundsMm']]
        for item, bounds in zip(sorted(two['buildItems'], key=lambda i: i['boundsMm']['min'][0]),
                                sorted(expected, key=lambda b: b['min'][0])):
            close_bounds(item['boundsMm'], bounds)
        assert any(abs(item['transform'][9]) > 1 for item in two['buildItems'])
        call('feature.edit', {'featureId': feature['id'], 'parameters': {'widthMm': 25}})
        edited = call('document.inspect')
        changed = out/'two-coupons-edited.3mf'
        call('document.export3mf', {'path': str(changed), 'allVisible': True,
            'surfaceDeviationMm': 0.1, 'normalDeviationDeg': 10})
        after_edit = inspect_3mf(changed)
        assert after_edit['objects'] == 2
        assert any(abs(a['boundsMm']['max'][i]-b['boundsMm']['max'][i]) > 4
                   for a, b in zip(sorted(two['buildItems'], key=lambda v: v['boundsMm']['min'][0]),
                                   sorted(after_edit['buildItems'], key=lambda v: v['boundsMm']['min'][0]))
                   for i in range(3))
        assert call('document.inspect')['authored'] == edited['authored']
        call('document.save', {'path': str(out/'coupon-source.alloy')})
        call('capture.screenshot', {'path': str(out/'two-coupons.png')})
        (out/'receipt.json').write_text(json.dumps({'passed': True, 'one': one, 'two': two,
            'afterEdit': after_edit, 'oneResult': one_result, 'twoResult': two_result,
            'transcript': transcript}, indent=2)+'\n')
    print('PASS real-app single/two-body 3MF, placed objects, source edit and independent readback')


if __name__ == '__main__':
    main()
