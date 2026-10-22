#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Bounded retained STL checks through the real application, using original fixtures."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import struct
from app_harness import application


def raw_facets(path):
    data = path.read_bytes()
    if len(data) >= 84 and len(data) == 84 + 50 * struct.unpack_from('<I', data, 80)[0]:
        return [[list(struct.unpack_from('<3f', data, start + 12 + 12*k)) for k in range(3)]
                for start in range(84, len(data), 50)]
    vertices = [list(map(float, line.split()[1:])) for line in data.decode().splitlines()
                if line.strip().startswith('vertex ')]
    return [vertices[i:i+3] for i in range(0, len(vertices), 3)]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('app', 'output', 'fixtures'):
        parser.add_argument('--'+name, required=True)
    args = parser.parse_args()
    output, fixtures = Path(args.output).resolve(), Path(args.fixtures).resolve()
    output.mkdir(parents=True, exist_ok=True)
    results = []
    cases = [('ascii_box_10mm.stl', 'mm', 12, 0, 0, 0, 0),
             ('binary_box_1in.stl', 'in', 12, 0, 0, 0, 0),
             ('open_square.stl', 'mm', 2, 4, 0, 0, 0),
             ('inverted_cube.stl', 'mm', 12, 0, 0, 3, 0),
             ('three_faces_one_edge.stl', 'mm', 3, 6, 1, 0, 0),
             ('degenerate.stl', 'mm', 2, None, 0, None, 1)]
    with application(args.app, output) as (call, transcript):
        for name, units, count, boundary, nonmanifold, orientation, degenerate in cases:
            call('document.new', {'discard': True})
            before = call('document.inspect')
            source = output / ('source-'+name)
            shutil.copyfile(fixtures / name, source)
            state = call('document.importStl', {'path': str(source), 'units': units})
            assert state['bodies'] == 0 and state['features'] == 0 and len(state['meshes']) == 1
            assert state['undo']['count'] == before['undo']['count'] + 1
            mesh = state['authored']['meshes'][0]
            summary = state['meshes'][0]
            assert summary['kind'] == 'mesh' and summary['triangleCount'] == count
            quality = summary['quality']
            for key, expected in [('boundaryEdges', boundary), ('nonManifoldEdges', nonmanifold),
                                  ('inconsistentOrientationEdges', orientation), ('degenerateFacets', degenerate)]:
                if expected is not None:
                    assert quality[key] == expected, (name, quality)
            scale = 25.4 if units == 'in' else 1
            geometry = call('mesh.inspect', {'meshId': mesh['id']})
            actual = [[geometry['verticesMm'][index] for index in triangle] for triangle in geometry['triangles']]
            for invalid_limit in (1001, 2147483648, 1.5, '1'):
                call('mesh.inspect', {'meshId': mesh['id'], 'limit': invalid_limit}, 'invalid_mesh_page')
            expected = [[[value*scale for value in point] for point in facet] for facet in raw_facets(source)]
            assert actual == expected, name
            assert mesh['source']['sha256'] == hashlib.sha256(source.read_bytes()).hexdigest()
            assert mesh['source']['scaleToMm'] == scale
            assert quality['repairApplied'] is False
            call('history.undo')
            assert call('document.inspect')['authored'] == before['authored']
            call('history.redo')
            assert call('document.inspect')['authored'] == state['authored']
            saved = output / (name+'.alloy')
            call('document.save', {'path': str(saved)})
            native_mesh = json.loads(saved.read_text())['meshes'][0]
            assert native_mesh['verticesMm'] == geometry['verticesMm'] and native_mesh['triangles'] == geometry['triangles']
            source.unlink()
            call('document.new', {'discard': True})
            call('document.open', {'path': str(saved)})
            reopened = call('document.inspect')
            assert reopened['authored'] == state['authored'] and reopened['meshes'] == state['meshes']
            assert call('mesh.inspect', {'meshId': mesh['id']}) == geometry
            baseline = call('document.inspect')
            app_baseline = call('app.state')
            saved_bytes = saved.read_bytes()
            for invalid in ('truncated_binary.stl', 'empty.stl', 'malformed_ascii.stl'):
                call('document.importStl', {'path': str(fixtures / invalid), 'units': 'mm'}, 'mesh_import_failed')
                assert call('document.inspect') == baseline
                assert call('app.state')['selection'] == app_baseline['selection']
                assert saved.read_bytes() == saved_bytes
            results.append({'fixture': name, 'mesh': summary, 'rawFacetCoordinatesMatch': True,
                            'undoRedoStable': True, 'sourceIndependentReopen': True, 'atomicFailures': True})
        # Fatal import must preserve a populated design and a real B-rep selection.
        call('solid.box', {'width': 4, 'depth': 5, 'height': 6, 'units': 'mm'})
        body = call('document.inspect')['evaluation']['bodies'][0]['bodyId']
        call('selection.set', {'bodyId': body})
        selected_save = output / 'selected-populated.alloy'
        call('document.save', {'path': str(selected_save)})
        baseline, selection = call('document.inspect'), call('app.state')['selection']
        saved_bytes = selected_save.read_bytes()
        for invalid in ('truncated_binary.stl', 'empty.stl', 'malformed_ascii.stl'):
            call('document.importStl', {'path': str(fixtures / invalid), 'units': 'mm'}, 'mesh_import_failed')
            assert call('document.inspect') == baseline
            assert call('app.state')['selection'] == selection
            assert selected_save.read_bytes() == saved_bytes
        (output / 'api-transcript.json').write_text(json.dumps(transcript, indent=2))
    (output / 'results.json').write_text(json.dumps({'result': 'passed', 'cases': results}, indent=2))
    print('retained STL real-application checks passed')


if __name__ == '__main__':
    main()
