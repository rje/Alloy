#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Retained mesh display/selection/history through the actual app API."""
import argparse
import json
from pathlib import Path
import shutil
from app_harness import application


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('app', 'output', 'fixtures'):
        parser.add_argument('--'+name, required=True)
    args = parser.parse_args()
    output, fixtures = Path(args.output).resolve(), Path(args.fixtures).resolve()
    output.mkdir(parents=True, exist_ok=True)
    with application(args.app, output) as (call, transcript):
        for name in ('ascii_box_10mm.stl', 'open_square.stl'):
            call('document.new', {'discard': True})
            call('solid.box', {'width': 12, 'depth': 10, 'height': 8, 'x': 30,
                              'y': 0, 'z': 0, 'units': 'mm'})
            before = call('document.inspect')
            source = output / name
            shutil.copyfile(fixtures / name, source)
            imported = call('document.importStl', {'path': str(source), 'units': 'mm'})
            mesh = imported['meshes'][0]['id']
            geometry = call('mesh.inspect', {'meshId': mesh})
            solid = call('document.inspect')['evaluation']['bodies'][0]['bodyId']
            call('selection.set', {'bodyId': solid, 'operation': 'add'})
            ordered = call('app.state')['selection']['members']
            call('mesh.visibility', {'meshId': mesh, 'visible': True})
            assert call('app.state')['selection']['members'] == ordered
            call('history.undo')  # Return to the just-imported record (visible field omitted).
            call('selection.set', {'bodyId': mesh})
            state = call('app.state')
            assert state['selection']['kind'] == 'mesh' and state['selection']['meshId'] == mesh
            assert state['viewport']['gpu']['meshes']['bodies'][0]['brep'] is False
            assert len(call('document.inspect')['evaluation']['bodies']) == 1
            call('view.orientation', {'name': 'isometric'})
            call('view.fit')
            call('capture.screenshot', {'path': str(output / (name+'-mixed.png'))})
            call('history.undo')
            assert call('document.inspect')['authored'] == before['authored']
            assert not call('app.state')['viewport']['gpu']['meshes']['bodies']
            call('history.redo')
            assert call('mesh.inspect', {'meshId': mesh}) == geometry
            call('selection.set', {'bodyId': mesh})
            call('mesh.visibility', {'meshId': mesh, 'visible': False})
            state = call('app.state')
            assert state['selection']['members'] == []
            assert not state['viewport']['gpu']['meshes']['bodies'][0]['displayed']
            call('selection.set', {'bodyId': mesh}, 'stale_entity')
            call('history.undo')
            assert call('app.state')['viewport']['gpu']['meshes']['bodies'][0]['displayed']
            call('history.redo')
            native = output / (name+'.alloy')
            call('document.save', {'path': str(native)})
            source.unlink()
            call('document.new', {'discard': True})
            call('document.open', {'path': str(native), 'discard': True})
            assert not call('document.inspect')['meshes'][0]['visible']
            assert not call('app.state')['viewport']['gpu']['meshes']['bodies'][0]['displayed']
            hidden = call('mesh.inspect', {'meshId': mesh})
            assert hidden['verticesMm'] == geometry['verticesMm'] and hidden['triangles'] == geometry['triangles']
            call('mesh.visibility', {'meshId': mesh, 'visible': True})
            call('selection.set', {'bodyId': mesh})
            call('view.fit')
            point = call('view.projectBody', {'bodyId': mesh})
            call('selection.set', {'operation': 'clear'})
            call('selection.pick', {'x': point['x'], 'y': point['y']})
            assert call('app.state')['selection']['meshId'] == mesh
            call('selection.facePick', {'x': point['x'], 'y': point['y']}, 'unsupported_face')
        for z, expected_kind in ((-20, 'mesh'), (20, 'face')):
            call('document.new', {'discard': True})
            call('solid.box', {'width': 8, 'depth': 8, 'height': 8,
                              'x': 1, 'y': 1, 'z': z, 'units': 'mm'})
            solid = call('document.inspect')['evaluation']['bodies'][0]['bodyId']
            imported = call('document.importStl', {'path': str(fixtures / 'ascii_box_10mm.stl'), 'units': 'mm'})
            mesh = imported['meshes'][0]['id']
            call('view.orientation', {'name': 'top'})
            call('view.fit')
            point = call('view.projectBody', {'bodyId': mesh})
            call('selection.set', {'operation': 'clear'})
            call('selection.pick', {'x': point['x'], 'y': point['y']})
            selection = call('app.state')['selection']
            assert selection['kind'] == expected_kind, (z, selection)
            assert (selection['meshId'] == mesh if expected_kind == 'mesh'
                    else selection['support']['bodyId'] == solid), selection
            call('capture.screenshot', {'path': str(output / ('overlap-'+expected_kind+'-front.png'))})
        (output / 'transcript.json').write_text(json.dumps(transcript, indent=2)+'\n')
        print(f'passed {len(transcript)} API responses')


if __name__ == '__main__':
    main()
