#!/usr/bin/env python3
"""Independent serialized-STL check of a retained open curved surface."""
import argparse
from collections import Counter
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import time

from app_harness import application
from alloyctl import request


def triangles(path, binary):
    data = path.read_bytes()
    if binary:
        count, = struct.unpack_from('<I', data, 80)
        assert len(data) == 84 + count * 50
        return [[record[3:6], record[6:9], record[9:12]] for record in
                (struct.unpack_from('<12fH', data, 84 + i * 50) for i in range(count))]
    points = [tuple(map(float, row)) for row in re.findall(
        r'vertex\s+(\S+)\s+(\S+)\s+(\S+)', data.decode('ascii'))]
    assert points and len(points) % 3 == 0
    return [points[i:i+3] for i in range(0, len(points), 3)]


def measure(path, binary, unit):
    scale = 25.4 if unit == 'in' else 1.0
    facets = [[tuple(v * scale for v in point) for point in tri]
              for tri in triangles(path, binary)]
    edges = Counter()
    area = 0.0
    for tri in facets:
        a, b, c = tri
        assert all(math.isfinite(v) for p in tri for v in p)
        u = [b[i] - a[i] for i in range(3)]
        v = [c[i] - a[i] for i in range(3)]
        cross = (u[1]*v[2]-u[2]*v[1], u[2]*v[0]-u[0]*v[2], u[0]*v[1]-u[1]*v[0])
        area += math.sqrt(sum(x*x for x in cross)) / 2
        vertices = [tuple(round(x, 5) for x in p) for p in tri]
        for x, y in zip(vertices, vertices[1:] + vertices[:1]):
            edges[tuple(sorted((x, y)))] += 1
    points = [p for tri in facets for p in tri]
    bounds = {'min': [min(p[i] for p in points) for i in range(3)],
              'max': [max(p[i] for p in points) for i in range(3)]}
    assert edges and max(edges.values()) == 2
    boundary = sum(n == 1 for n in edges.values())
    assert boundary > 0, boundary
    assert math.isclose(area, 100 * math.pi, rel_tol=.015), area
    for side, expected in (('min', [0, 0, -5]), ('max', [20, 20, 5])):
        assert all(abs(x-y) < .06 for x, y in zip(bounds[side], expected)), bounds
    return {'triangles': len(facets), 'boundaryEdges': boundary,
            'nonmanifoldEdges': sum(n > 2 for n in edges.values()),
            'areaMm2': area, 'boundsMm': bounds,
            'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    out = Path(args.output).resolve()
    out.mkdir(parents=True, exist_ok=True)
    observations = []
    with application(args.app, out, command_timeout=90) as (call, transcript):
        def raw(command, parameters=None, refused=False):
            response = request(call.endpoint, command, parameters or {}, timeout=90,
                               transport=call.transport)
            transcript.append({'command': command, 'args': parameters or {}, 'response': response})
            assert response['ok'] != refused, response
            return response.get('result', {}) if not refused else response['error']

        def prepared(settings):
            started = raw('document.previewMesh', settings)
            deadline = time.monotonic() + 90
            while True:
                status = raw('document.meshPreview')
                if status.get('evaluationStatus') != 'pending':
                    break
                assert time.monotonic() < deadline, status
                time.sleep(.03)
            assert status['evaluationStatus'] == 'completed', status
            assert status['previewId'] == started['previewId']
            return status

        call('window.resize', {'width': 1280, 'height': 900})
        assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
        call('sketch.create', {'plane': 'XZ', 'name': 'Open section'})
        call('sketch.line', {'x1': 20, 'y1': -5, 'x2': 20, 'y2': 5, 'units': 'mm'})
        section = call('app.state')['activeSession']['draftSketch']
        section_entity = section['parameters']['entities'][0]
        call('sketch.finish')
        call('sketch.create', {'plane': 'XY', 'name': 'Quarter-circle path'})
        call('sketch.addArc', {'mode': 'center', 'centerX': 0, 'centerY': 0,
            'radius': 20, 'startAngleDeg': 0, 'sweepAngleDeg': 90, 'units': 'mm'})
        path = call('app.state')['activeSession']['draftSketch']
        path_entity = path['parameters']['entities'][0]
        call('sketch.finish')
        call('surface.sweep', {'sketchId': section['id'], 'entityId': section_entity['id'],
            'pathSketchId': path['id'], 'pathEntityId': path_entity['id'],
            'pathStartPointId': path_entity['startPointId']})
        baseline = call('document.inspect')
        surface, = baseline['evaluation']['bodies']
        assert surface['type'] == 'surface' and not surface['closed']
        assert math.isclose(surface['areaMm2'], 100 * math.pi, abs_tol=1e-4)
        model = out/'open-sheet.alloy'
        call('document.save', {'path': str(model)})
        baseline = call('document.inspect')
        source_bytes = model.read_bytes()
        call('view.home')
        call('capture.screenshot', {'path': str(out/'open-sheet-gpu.png')})
        for fmt, unit, deviation, angle in (('binary', 'mm', .1, 10),
                                            ('ascii', 'in', .02, 5)):
            settings = {'bodyId': surface['bodyId'], 'format': fmt, 'units': unit,
                        'surfaceDeviationMm': deviation, 'normalDeviationDeg': angle,
                        'preset': 'Custom'}
            preview = prepared(settings)
            assert preview['bodyId'] == surface['bodyId']
            assert preview['bodyType'] == 'openSurface'
            assert preview['solid'] is False and preview['closedVolume'] is False
            assert 'not a printable solid' in preview['warnings'][0]
            assert math.isclose(preview['coordinateScaleFromMm'],
                                1/25.4 if unit == 'in' else 1, rel_tol=1e-12)
            if fmt == 'binary':
                time.sleep(.08)
                call('capture.screenshot', {'path': str(out/'open-sheet-preview-gpu.png')})
            output = out/f'quarter-sheet-{fmt}-{unit}.stl'
            saved = raw('document.saveMeshPreview', {'previewId': preview['previewId'],
                'path': str(output)})
            inspected = measure(output, fmt == 'binary', unit)
            assert inspected['triangles'] == preview['triangles'] == saved['triangles']
            assert inspected['sha256'] == preview['sha256']
            assert call('document.inspect')['authored'] == baseline['authored']
            assert call('document.inspect')['undo'] == baseline['undo']
            assert model.read_bytes() == source_bytes
            observations.append({'settings': settings, 'preview': preview,
                                 'saved': saved, 'independent': inspected})
        assert observations[1]['independent']['triangles'] >= observations[0]['independent']['triangles']
        direct_path = out/'direct-open-sheet.stl'
        direct = call('document.exportStl', {'bodyId': surface['bodyId'],
            'path': str(direct_path), 'format': 'binary', 'units': 'mm',
            'surfaceDeviationMm': .1, 'normalDeviationDeg': 10})
        assert direct['bodyType'] == 'openSurface' and direct['closedVolume'] is False
        direct_mesh = measure(direct_path, True, 'mm')
        assert direct_mesh['sha256'] == observations[0]['independent']['sha256']
        assert call('document.inspect')['authored'] == baseline['authored']
        assert model.read_bytes() == source_bytes
        (out/'report.json').write_text(json.dumps(observations, indent=2)+'\n')
        (out/'transcript.json').write_text(json.dumps(transcript, indent=2)+'\n')
    print('PASS open curved sheet binary mm/ASCII inch serialized STL, topology and preserved authored state')


if __name__ == '__main__':
    main()
