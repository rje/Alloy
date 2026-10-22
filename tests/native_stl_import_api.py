#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Prepared native STL workflow through Alloy's real local API."""
import argparse
import json
import shutil
import time
from pathlib import Path
from app_harness import application
from retained_stl_import_api import raw_facets


def main():
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('app', 'output', 'fixtures'):
        p.add_argument('--'+name, required=True)
    args = p.parse_args()
    output, fixtures = Path(args.output).resolve(), Path(args.fixtures).resolve()
    output.mkdir(parents=True, exist_ok=True)
    with application(args.app, output) as (call, transcript):
        def ready():
            deadline = time.monotonic() + 30
            while True:
                state = call('app.state')['meshImport']
                if state['status'] != 'pending':
                    return state
                assert time.monotonic() < deadline, state
                time.sleep(.02)
        def preview(path):
            assert call('mesh.importPreview', {'path': str(path)})['status'] == 'pending'
            state = ready()
            assert state['status'] == 'ready', state
            return state['preview']
        def units(value):
            assert call('mesh.importUnits', {'units': value})['status'] == 'pending'
            state = ready()
            assert state['status'] == 'ready', state
            return state['preview']
        assert any(c['id'] == 'document.importMeshDialog' for c in call('commands.list')['commands'])
        for name in ('ascii_box_10mm.stl', 'open_square.stl'):
            call('document.new', {'discard': True})
            call('solid.box', {'width': 8, 'depth': 9, 'height': 10, 'x': 30, 'units': 'mm'})
            body = call('document.inspect')['evaluation']['bodies'][0]['bodyId']
            call('selection.set', {'bodyId': body})
            saved = output / (name+'.before.alloy')
            call('document.save', {'path': str(saved)})
            before, selection, saved_bytes = call('document.inspect'), call('app.state')['selection'], saved.read_bytes()
            source = output / name
            shutil.copyfile(fixtures / name, source)
            mm = preview(source)
            inch = units('in')
            for axis in ('min', 'max'):
                assert inch['boundsMm'][axis] == [x*25.4 for x in mm['boundsMm'][axis]]
            assert call('document.inspect') == before and call('app.state')['selection'] == selection
            call('solid.box', {'width': 1, 'depth': 1, 'height': 1, 'units': 'mm'}, 'active_command')
            for command in ('view.namedCreate', 'view.namedRename', 'view.namedDelete', 'view.homeSet', 'view.homeReset', 'view.conventionUp', 'view.conventionSet', 'view.conventionReset'):
                call(command, {}, 'active_command')
            call('capture.screenshot', {'path': str(output / (name+'-preview-in.png'))})
            call('mesh.importCancel')
            assert call('document.inspect') == before and call('app.state')['selection'] == selection
            assert saved.read_bytes() == saved_bytes
            preview(source)
            units('in')
            source.unlink()  # OK commits the preview, never reparses a changed/deleted source.
            call('mesh.importAccept')
            state = call('document.inspect')
            mesh = state['meshes'][0]['id']
            geometry = call('mesh.inspect', {'meshId': mesh})
            actual = [[geometry['verticesMm'][i] for i in tri] for tri in geometry['triangles']]
            expected = [[[x*25.4 for x in point] for point in facet] for facet in raw_facets(fixtures / name)]
            assert actual == expected
            assert call('app.state')['selection']['meshId'] == mesh
            call('history.undo')
            assert call('document.inspect')['authored'] == before['authored']
            call('history.redo')
            assert call('mesh.inspect', {'meshId': mesh}) == geometry
            path = output / (name+'.alloy')
            call('document.save', {'path': str(path)})
            call('document.new', {'discard': True})
            call('document.open', {'path': str(path), 'discard': True})
            assert call('mesh.inspect', {'meshId': mesh}) == geometry
        before, selection = call('document.inspect'), call('app.state')['selection']
        call('mesh.importPreview', {'path': str(fixtures / 'truncated_binary.stl')})
        failed = ready()
        assert failed['status'] == 'failed' and failed['error']['code'] == 'mesh_import_failed', failed
        call('mesh.importCancel')
        assert call('document.inspect') == before and call('app.state')['selection'] == selection
        (output/'transcript.json').write_text(json.dumps(transcript, indent=2))
        (output/'summary.json').write_text(json.dumps({'passed': True, 'responses': len(transcript)}, indent=2))


if __name__ == '__main__':
    main()
