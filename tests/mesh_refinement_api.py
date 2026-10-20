#!/usr/bin/env python3
"""PRT-011 independent real-entrypoint prepared mesh artifact verification.

The readers below inspect serialized bytes, independently of Alloy's reported
triangle count and viewport tessellation. Run only with the private GUI lease.
"""
import argparse
import base64
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import time
import xml.etree.ElementTree as ET
import zipfile

from app_harness import application
from alloyctl import request
from stl_export_api import inspect_stl
from three_mf_export_api import inspect_3mf

NS = {'m': 'http://schemas.microsoft.com/3dmanufacturing/core/2015/02'}


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def serialized_triangles(path, fmt, encoding='binary'):
    """Read world-space triangles; apply the Core3MF row-vector transform."""
    data = Path(path).read_bytes()
    if fmt == 'stl':
        if encoding == 'binary':
            count, = struct.unpack_from('<I', data, 80)
            assert len(data) == 84 + count * 50
            return [[v[3:6], v[6:9], v[9:12]] for v in
                    (struct.unpack_from('<12fH', data, 84+i*50) for i in range(count))]
        points = [tuple(map(float, row)) for row in
                  re.findall(r'vertex\s+(\S+)\s+(\S+)\s+(\S+)', data.decode('ascii'))]
        assert points and len(points) % 3 == 0
        return [points[i:i+3] for i in range(0, len(points), 3)]
    with zipfile.ZipFile(path) as package:
        models = [p for p in package.namelist() if p.lower().endswith('.model')]
        assert len(models) == 1
        model = ET.fromstring(package.read(models[0]))
    assert model.attrib['unit'] == 'millimeter'
    objects = {}
    for obj in model.findall('m:resources/m:object', NS):
        vertices = [tuple(float(v.attrib[k]) for k in ('x', 'y', 'z'))
                    for v in obj.findall('m:mesh/m:vertices/m:vertex', NS)]
        objects[obj.attrib['id']] = [[vertices[int(t.attrib[k])] for k in ('v1', 'v2', 'v3')]
                                    for t in obj.findall('m:mesh/m:triangles/m:triangle', NS)]
    triangles = []
    for item in model.findall('m:build/m:item', NS):
        matrix = list(map(float, item.attrib.get('transform', '1 0 0 0 1 0 0 0 1 0 0 0').split()))
        assert len(matrix) == 12
        triangles.extend([tuple(sum(p[r]*matrix[r*3+c] for r in range(3))+matrix[9+c]
                                for c in range(3)) for p in triangle]
                         for triangle in objects[item.attrib['objectid']])
    return triangles


def inspect_artifact(path, fmt, encoding='binary', units='mm', center=(0, 0), radius=10):
    if fmt == 'stl':
        summary = inspect_stl(path, encoding == 'binary')
        assert len(summary['components']) == 1
    else:
        summary = inspect_3mf(path)
        assert summary['objects'] == len(summary['buildItems']) == 1
    triangles = serialized_triangles(path, fmt, encoding)
    scale = 25.4 if units == 'in' else 1
    points = [[tuple(x*scale for x in p) for p in t] for t in triangles]
    bounds = {'min': [min(p[i] for t in points for p in t) for i in range(3)],
              'max': [max(p[i] for t in points for p in t) for i in range(3)]}
    # A cylindrical side chord has endpoints on the exact radial surface.
    # Sampling its midpoint gives its exact maximum radial chord deficit.
    # Caps/diagonals across the circle are excluded by differing endpoint Z.
    errors = []
    for triangle in points:
        for a, b in zip(triangle, triangle[1:] + triangle[:1]):
            radial = [math.hypot(p[0]-center[0], p[1]-center[1]) for p in (a, b)]
            if (abs(a[2]-b[2]) > 1e-6 and
                    all(abs(r-radius) < 1e-4 for r in radial)):
                midpoint = [(a[i]+b[i])/2 for i in range(3)]
                errors.append(radius-math.hypot(midpoint[0]-center[0], midpoint[1]-center[1]))
    assert errors, 'Cylinder side edges were not identified'
    return {'triangles': len(triangles), 'sha256': sha256(path), 'boundsMm': bounds,
            'maxCylinderChordErrorMm': max(errors), 'independentReader': summary}


def unchanged(call, baseline):
    current = call('document.inspect')
    for key in ('authored', 'undo', 'revision', 'evaluation'):
        assert current[key] == baseline[key], key
    return current


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    out = Path(args.output).resolve()
    out.mkdir(parents=True, exist_ok=True)
    run(args.app, out)


def run(app, out):
    evidence, failures = [], []
    with application(app, out, command_timeout=90) as (call, ledger):
        def raw(command, parameters=None, *, refused=False):
            response = request(call.endpoint, command, parameters or {}, timeout=90,
                               transport=call.transport)
            ledger_response = json.loads(json.dumps(response))
            ledger_response.get('result', {}).pop('bytesBase64', None)
            ledger.append({'command': command, 'args': parameters or {}, 'response': ledger_response})
            (out/'partial-ledger.json').write_text(json.dumps(ledger, indent=2)+'\n')
            assert response['ok'] != refused, response
            if refused:
                failures.append(response)
            return response.get('result', {})

        def prepare(settings):
            started = raw('document.previewMesh', settings)
            deadline = time.monotonic() + 90
            while True:
                status = raw('document.meshPreview')
                if status.get('evaluationStatus') != 'pending':
                    break
                assert time.monotonic() < deadline, status
                time.sleep(.03)
            assert status['evaluationStatus'] == 'completed', status
            assert status['previewId'] == started['previewId'], (started, status)
            return status

        def capture(name):
            # Preparation readiness and the render timer are separate; allow the
            # published immutable facets to reach the native GPU presentation.
            time.sleep(.08)
            return call('capture.screenshot', {'path': str(out/(name+'.png'))})

        call('window.resize', {'width': 1280, 'height': 900})
        root = call('document.inspect')['authored']['documentId']
        component = call('component.create', {'name': 'Curved refinement coupon'})
        call('component.activate', {'definitionId': component['definitionId']})
        call('solid.cylinder', {'radius': 10, 'height': 20, 'units': 'mm', 'name': 'R10 H20'})
        source = call('document.inspect')
        cylinder = source['evaluation']['bodies'][0]['bodyId']
        feature = source['authored']['features'][-1]['id']
        call('component.activate', {'definitionId': root})
        copied = call('component.copyOccurrence', {'occurrenceId': component['occurrenceId'],
            'placement': {'axis': 'Z', 'angleDeg': 90, 'translationMm': [40, 10, 5]}})
        occurrence = copied['rootOccurrenceId']
        instances = call('component.inspect')['localInstances']
        selected = next(row for row in instances if row['occurrenceId'] == occurrence)
        expected_bounds = selected['bodies'][0]['boundsMm']
        call('selection.set', {'bodyId': cylinder, 'occurrenceId': occurrence})
        call('view.home')
        call('view.fitSelection')
        call('document.save', {'path': str(out/'cylinder-source.alloy')})
        baseline = call('document.inspect')
        source_hash = sha256(out/'cylinder-source.alloy')
        capture('normal-before')
        # These values are explicit Alloy defaults, not inferred public defaults.
        cases = [('Low', .5, 25, 'binary', 'mm'),
                 ('Medium', .1, 10, 'binary', 'mm'),
                 ('High', .02, 5, 'binary', 'mm'),
                 ('Custom', .01, 3, 'binary', 'mm'),
                 ('Medium', .1, 10, 'ascii', 'in'),
                 ('Medium', .1, 10, '3mf', 'mm')]
        for label, deviation, angle, fmt, units in cases:
            settings = {'bodyId': cylinder, 'occurrenceId': occurrence, 'format': fmt,
                        'units': units, 'surfaceDeviationMm': deviation,
                        'normalDeviationDeg': angle, 'preset': label}
            status = prepare(settings)
            prepared = raw('document.meshPreview', {'includeBytes': True})
            prepared_bytes = base64.b64decode(prepared.pop('bytesBase64'), validate=True)
            assert hashlib.sha256(prepared_bytes).hexdigest() == status['sha256']
            assert len(prepared_bytes) == status['bytes']
            assert status['settings']['preset'] == label
            unchanged(call, baseline)
            passive = raw('document.meshPreview')
            assert passive['previewId'] == status['previewId'] and passive['sha256'] == status['sha256']
            call('selection.set', {'bodyId': cylinder, 'occurrenceId': occurrence})
            assert raw('document.meshPreview')['sha256'] == status['sha256']
            if fmt == 'binary':
                capture(label+'-preview')
            destination = out/(label+'-'+fmt+('.3mf' if fmt == '3mf' else '.stl'))
            saved = raw('document.saveMeshPreview', {'previewId': status['previewId'], 'path': str(destination)})
            mesh = inspect_artifact(destination, '3mf' if fmt == '3mf' else 'stl',
                                    fmt, units, center=(40, 10))
            assert mesh['triangles'] == status['triangles'], (mesh, status)
            assert mesh['sha256'] == status['sha256'], (mesh, status)
            assert destination.read_bytes() == prepared_bytes
            for side in ('min', 'max'):
                assert all(abs(a-b) < deviation+.0001 for a, b in
                           zip(mesh['boundsMm'][side], expected_bounds[side])), mesh
            assert mesh['maxCylinderChordErrorMm'] <= deviation+.0001, mesh
            if fmt == '3mf':
                transform = mesh['independentReader']['buildItems'][0]['transform']
                assert any(abs(x) > 1 for x in transform[9:]), transform
            unchanged(call, baseline)
            evidence.append({'label': label, 'settings': settings, 'preview': status,
                             'saved': saved, 'mesh': mesh, 'file': destination.name})
            (out/'partial-exports.json').write_text(json.dumps(evidence, indent=2)+'\n')
        binary_rows = [row for row in evidence if row['settings']['format'] == 'binary']
        for coarse, fine in zip(binary_rows, binary_rows[1:]):
            assert fine['mesh']['triangles'] >= coarse['mesh']['triangles']
            assert fine['mesh']['maxCylinderChordErrorMm'] <= coarse['mesh']['maxCylinderChordErrorMm']+1e-6
        settings = evidence[-1]['settings']
        # Publication refusal must retain the prepared bytes for safe retry.
        status = prepare(settings)
        blocker = out/'destination-parent-is-file'
        blocker.write_bytes(b'preserve-existing-parent')
        raw('document.saveMeshPreview', {'previewId': status['previewId'], 'path': str(blocker/'blocked.3mf')}, refused=True)
        assert blocker.read_bytes() == b'preserve-existing-parent'
        retry = out/'retry.3mf'
        raw('document.saveMeshPreview', {'previewId': status['previewId'], 'path': str(retry)})
        assert sha256(retry) == status['sha256']
        unchanged(call, baseline)
        # Clear/cancel never names or publishes an output destination.
        status = prepare(settings)
        absent = out/'cancel-must-not-exist.3mf'
        assert not absent.exists()
        sentinel = out/'cancel-preserved.3mf'
        sentinel.write_bytes(b'preserve-existing-output')
        raw('document.clearMeshPreview', {'previewId': status['previewId']})
        raw('document.saveMeshPreview', {'previewId': status['previewId'], 'path': str(absent)}, refused=True)
        raw('document.saveMeshPreview', {'previewId': status['previewId'], 'path': str(sentinel)}, refused=True)
        assert not absent.exists() and sentinel.read_bytes() == b'preserve-existing-output'
        unchanged(call, baseline)
        capture('normal-after-cancel')
        # Cancel the next generation before waiting for preparation to finish.
        late = raw('document.previewMesh', dict(settings, preset='Custom',
                    surfaceDeviationMm=.001, normalDeviationDeg=1))
        raw('document.clearMeshPreview', {'previewId': late['previewId']})
        current = prepare(settings)
        assert current['previewId'] != late['previewId']
        raw('document.saveMeshPreview', {'previewId': late['previewId'], 'path': str(absent)}, refused=True)
        assert raw('document.meshPreview')['previewId'] == current['previewId']
        assert not absent.exists()
        unchanged(call, baseline)
        # A newer settings generation invalidates the older artifact token.
        old = prepare(settings)
        newer = prepare(dict(settings, surfaceDeviationMm=.04, preset='Custom'))
        assert old['previewId'] != newer['previewId']
        raw('document.saveMeshPreview', {'previewId': old['previewId'], 'path': str(absent)}, refused=True)
        # Exact instance identity, not merely the shared body, binds the token.
        call('selection.set', {'bodyId': cylinder, 'occurrenceId': component['occurrenceId']})
        raw('document.saveMeshPreview', {'previewId': newer['previewId'], 'path': str(absent)}, refused=True)
        call('selection.set', {'bodyId': cylinder, 'occurrenceId': occurrence})
        old = prepare(settings)
        call('feature.edit', {'featureId': feature, 'parameters': {'heightMm': 25}})
        raw('document.saveMeshPreview', {'previewId': old['previewId'], 'path': str(absent)}, refused=True)
        call('history.undo')
        assert call('document.inspect')['authored'] == baseline['authored']
        old = prepare(settings)
        call('document.tabNew')
        raw('document.saveMeshPreview', {'previewId': old['previewId'], 'path': str(absent)}, refused=True)
        assert not absent.exists()
        assert sha256(out/'cylinder-source.alloy') == source_hash
        environment = call('app.state')
        (out/'receipt.json').write_text(json.dumps({'passed': True, 'exports': evidence,
            'refusals': failures, 'cancelledGenerationInitialState': late, 'environment': environment,
            'limits': ['Native chooser/pointer, 1x/2x, exact public visual parity and other platforms unqualified.',
                       'Cylinder chord deficit is a bounded analytic measure, not a universal surface certificate.'],
            'ledger': ledger}, indent=2)+'\n')
    print('PASS independent prepared-artifact counts, hashes, units, placement and transient lifecycle')


if __name__ == '__main__':
    main()
