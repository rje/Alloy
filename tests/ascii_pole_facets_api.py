#!/usr/bin/env python3
"""Bounded shared-app ASCII export of an existing authored pole fixture or die."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl

p = argparse.ArgumentParser()
p.add_argument('--app', required=True)
p.add_argument('--fixture', required=True)
p.add_argument('--output', required=True)
a = p.parse_args()
out = Path(a.output).resolve()
with application(a.app, out) as (call, transcript):
    call('document.open', {'path': str(Path(a.fixture).resolve())})
    source = call('document.inspect')
    scope = source['authored']['occurrences'][0]['id']
    exports = []
    for units, scale in [('mm', 1), ('in', 1 / 25.4)]:
        path = out / f'die-{units}-ascii.stl'
        result = call('document.exportStl', {'occurrenceId': scope, 'path': str(path), 'units': units, 'format': 'ascii', 'surfaceDeviationMm': .05, 'normalDeviationDeg': 8})
        mesh = inspect_stl(path, False)
        assert result['serializedCollapsedTrianglesRemoved'] > 0
        assert result['triangles'] == mesh['triangles']
        assert result['bytes'] == path.stat().st_size
        assert len(mesh['components']) == 1
        assert all(math.isclose(mesh['bounds'][1][i] - mesh['bounds'][0][i], 20 * scale, abs_tol=1e-6) for i in range(3))
        assert math.isclose(mesh['signedVolume'] / scale**3, source['evaluation']['bodies'][0]['volumeMm3'], rel_tol=5e-5)
        after = call('document.inspect')
        assert after['authored'] == source['authored'] and after['undo'] == source['undo'] and after['revision'] == source['revision']
        exports.append({'result': result, 'mesh': mesh})
    (out / 'ascii-export-report.json').write_text(json.dumps({'passed': True, 'source': source, 'exports': exports}, indent=2))
    (out / 'transcript.json').write_text(json.dumps(transcript, indent=2))
