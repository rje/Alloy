#!/usr/bin/env python3
"""Export a source-derived component with a hidden sheet and visible solid."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--app', required=True)
p.add_argument('--model', required=True)
p.add_argument('--output', required=True, type=Path)
a = p.parse_args(); out = a.output.resolve()
with application(a.app, out) as (call, ledger):
    call('document.open', {'path': str(Path(a.model).resolve())})
    initial = call('document.inspect')
    sheet, = [b for b in initial['evaluation']['bodies'] if b['type'] == 'surface']
    solid, = [b for b in initial['evaluation']['bodies'] if b['type'] == 'solid']
    assert not sheet['visible'] and solid['visible']
    if solid['ownerId'] != sheet['ownerId']:
        call('component.reparent', {'destinationId': sheet['ownerId'], 'bodyId': solid['bodyId']})
    before = call('document.inspect')
    call('document.save', {'path': str(out / 'organized-component.alloy')})
    call('document.open', {'path': str(out / 'organized-component.alloy')})
    assert call('document.inspect')['authored'] == before['authored']
    occurrence, = [o for o in before['authored']['occurrences'] if o['definitionId'] == sheet['ownerId']]
    settings = {'path': str(out / 'component-mm.stl'), 'units': 'mm', 'format': 'binary',
                'surfaceDeviationMm': .1, 'normalDeviationDeg': 10}
    result = call('document.exportStl', {**settings, 'occurrenceId': occurrence['id']})
    assert result['bodyIds'] == [solid['bodyId']]
    mesh = inspect_stl(settings['path'], True)
    assert len(mesh['components']) == 1
    assert math.isclose(mesh['signedVolume'], solid['volumeMm3'], rel_tol=.01)
    assert call('document.inspect')['authored'] == before['authored']
    call('document.exportStl', {**settings, 'path': str(out / 'explicit-sheet.stl'),
         'bodyId': sheet['bodyId']}, error='solid_required')
    call('feature.visibility', {'featureId': sheet['featureId'], 'visible': True})
    call('document.exportStl', {**settings, 'path': str(out / 'visible-sheet.stl'),
         'occurrenceId': occurrence['id']}, error='solid_required')
    call('history.undo')
    assert call('document.inspect')['authored'] == before['authored']
    call('component.activate', {'definitionId': sheet['ownerId']})
    dialog = call('document.exportMeshDialog')
    assert dialog['occurrenceId'] == occurrence['id']
    call('capture.screenshot', {'path': str(out / 'component-selection.png')})
    (out / 'report.json').write_text(json.dumps({'passed': True, 'export': result, 'mesh': mesh}, indent=2))
    (out / 'transcript.json').write_text(json.dumps(ledger, indent=2))
print('PASS: visible component solid exported; retained surface preserved')
