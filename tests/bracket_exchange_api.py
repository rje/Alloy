#!/usr/bin/env python3
"""Export a saved Alloy bracket and independently check STEP/STL deliverables."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import shutil
import struct
import subprocess

from app_harness import application
from stl_export_api import inspect_stl


def close(actual, expected, tolerance=1e-4):
    assert math.isclose(actual, expected, rel_tol=tolerance, abs_tol=tolerance), (actual, expected)


def scale_binary_stl(source, destination, factor):
    data = bytearray(source.read_bytes())
    count, = struct.unpack_from('<I', data, 80)
    assert len(data) == 84 + count * 50
    data[:80] = f'Alloy print sample; geometry scale {factor}; coordinates mm'.encode().ljust(80, b' ')
    for index in range(count):
        offset = 84 + index * 50 + 12
        coordinates = struct.unpack_from('<9f', data, offset)
        struct.pack_into('<9f', data, offset, *(value * factor for value in coordinates))
    destination.write_bytes(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('app', 'source', 'output', 'step-tool'):
        parser.add_argument('--' + name, required=True)
    args = parser.parse_args()
    source, output = Path(args.source).resolve(), Path(args.output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    saved = output / 'illustrated-bracket-fullsize.alloy'
    shutil.copy2(source, saved)
    step = output / 'illustrated-bracket-fullsize-mm.step'
    stl = output / 'illustrated-bracket-fullsize-mm.stl'
    sample = output / 'illustrated-bracket-print-sample-0.2x-mm.stl'

    with application(args.app, output) as (call, transcript):
        call('document.open', {'path': str(saved)})
        before = call('document.inspect')
        bodies = before['evaluation']['bodies']
        assert len(bodies) == 1 and bodies[0]['valid'] and bodies[0]['closed'], bodies
        body = bodies[0]
        assert body['volumeMm3'] > 0
        step_result = call('document.exportStep', {'path': str(step), 'bodyId': body['bodyId'], 'units': 'mm'})
        stl_result = call('document.exportStl', {'path': str(stl), 'bodyId': body['bodyId'],
            'units': 'mm', 'format': 'binary', 'surfaceDeviationMm': 0.05, 'normalDeviationDeg': 5})
        assert call('document.inspect') == before, 'Export changed the saved design'
        assert step_result['sha256'] == hashlib.sha256(step.read_bytes()).hexdigest()
        assert stl_result['units'] == 'mm' and not stl_result['unitsEmbedded']

    result = subprocess.run([str(Path(args.step_tool).resolve()), 'read', str(step)],
                            check=True, capture_output=True, text=True)
    step_read = json.loads(result.stdout.splitlines()[-1])
    assert step_read['valid'] and step_read['solidCount'] == 1
    assert any(unit.upper() in ('MM', 'MILLIMETRE', 'MILLIMETER') for unit in step_read['fileLengthUnits'])
    close(step_read['volumeMm3'], body['volumeMm3'], 1e-7)
    bounds = body['boundsMm']
    for actual, expected in zip(step_read['boundsMm'], [*bounds['min'], *bounds['max']]):
        close(actual, expected, 1e-4)

    full_mesh = inspect_stl(stl, True)
    assert full_mesh['triangles'] == stl_result['triangles']
    for actual, expected in zip(full_mesh['bounds'][0], bounds['min']):
        close(actual, expected, 1e-3)
    for actual, expected in zip(full_mesh['bounds'][1], bounds['max']):
        close(actual, expected, 1e-3)
    close(full_mesh['signedVolume'], body['volumeMm3'], 0.005)

    scale_binary_stl(stl, sample, 0.2)
    sample_mesh = inspect_stl(sample, True)
    assert sample_mesh['triangles'] == full_mesh['triangles']
    for sample_bounds, full_bounds in zip(sample_mesh['bounds'], full_mesh['bounds']):
        for actual, expected in zip(sample_bounds, full_bounds):
            close(actual, expected * 0.2, 1e-3)
    close(sample_mesh['signedVolume'], full_mesh['signedVolume'] * 0.2**3, 1e-5)
    sample_size = [hi - lo for lo, hi in zip(*sample_mesh['bounds'])]

    report = {'result': 'passed', 'sourceSha256': hashlib.sha256(source.read_bytes()).hexdigest(),
        'body': body, 'stepExport': step_result, 'stepIndependentReadback': step_read,
        'stlExport': stl_result, 'fullsizeMesh': full_mesh, 'printSampleMesh': sample_mesh,
        'printSampleSizeMm': sample_size, 'files': {p.name: {'bytes': p.stat().st_size,
            'sha256': hashlib.sha256(p.read_bytes()).hexdigest()} for p in (saved, step, stl, sample)},
        'transcript': transcript}
    (output / 'verification.json').write_text(json.dumps(report, indent=2) + '\n')
    print(f'PASS: one valid STEP solid; closed oriented full and 0.2x STL meshes; sample size {sample_size} mm')


if __name__ == '__main__':
    main()
