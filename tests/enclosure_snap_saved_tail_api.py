#!/usr/bin/env python3
"""Verify the saved from-blank snap model after a late runner assertion.

This opens the original recorded document; it does not rebuild its geometry.
"""
import argparse
import hashlib
import json
from pathlib import Path

from app_harness import application
from stl_export_api import inspect_stl


parser = argparse.ArgumentParser()
parser.add_argument('--app', required=True)
parser.add_argument('--source', required=True)
parser.add_argument('--output', required=True)
args = parser.parse_args()
source = Path(args.source).resolve()
out = Path(args.output).resolve()
out.mkdir(parents=True, exist_ok=True)


def body(state, identity):
    return next(b for b in state['evaluation']['bodies'] if b['bodyId'] == identity)


def by_id(features, identity):
    return next(f for f in features if f['id'] == identity)


with application(args.app, out, command_timeout=180) as (call, ledger):
    call('window.resize', {'width': 1400, 'height': 1000})
    assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
    call('document.open', {'path': str(source), 'discard': True})
    original = call('document.inspect')
    features = original['authored']['features']
    split = next(f for f in features if f['type'] == 'splitBody')
    bottom_id = split['parameters']['targetBodyId']
    top_id = split['bodyId']
    bottom, top = body(original, bottom_id), body(original, top_id)
    assert (bottom['name'], top['name']) == ('Bottom', 'Top')
    assert all(x['valid'] and x['closed'] and x['topology']['solids'] == 1
               for x in (bottom, top))
    mirrors = [f for f in features if f['type'] == 'featureMirror']
    assert len(mirrors) == 2
    final_fillet = features[-1]
    assert final_fillet['type'] == 'fillet'
    assert final_fillet['parameters']['targetBodyId'] == bottom_id
    assert len(final_fillet['parameters']['groups']) == 2
    assert all(g['radiusMm'] == .5 for g in final_fillet['parameters']['groups'])
    assert features.index(final_fillet) > max(features.index(f) for f in mirrors)

    # Exercise a real geometric edit and history on the retained final Fillet.
    call('component.activate', {'definitionId': final_fillet['ownerId']})
    groups = [dict(g, radiusMm=.45) for g in final_fillet['parameters']['groups']]
    call('solid.fillet', {'featureId': final_fillet['id'],
                          'targetBodyId': bottom_id, 'groups': groups})
    edited = call('document.inspect')
    assert by_id(edited['authored']['features'], final_fillet['id'])['parameters']['groups'] == groups
    call('history.undo')
    assert call('document.inspect')['authored'] == original['authored']
    call('history.redo')
    assert call('document.inspect')['authored'] == edited['authored']
    call('history.undo')
    assert call('document.inspect')['authored'] == original['authored']

    # The pre-Split face-linked Top analysis loses its support. This origin-XY
    # section is an explicit inspection adaptation at the same z=24.5 plane.
    top_analysis = next(a for a in call('analysis.inspect')['analyses'] if a['name'] == 'Top')
    assert top_analysis['availability'] == 'unavailable'
    call('inspect.section', {'plane': 'XY', 'name': 'Top fit inspection'})
    section = next(a for a in call('analysis.inspect')['analyses']
                   if a['name'] == 'Top fit inspection')
    call('inspect.section', {'analysisId': section['id'], 'offsetMm': 24.5, 'flip': True})
    section = next(a for a in call('analysis.inspect')['analyses']
                   if a['id'] == section['id'])
    assert section['availability'] == 'available'
    assert section['resolvedPlane']['origin'] == [0, 0, 24.5]
    assert section['resolvedPlane']['normal'] == [0, 0, -1]
    call('analysis.visibility', {'analysisId': section['id'], 'visible': True})
    clipped = call('app.state')['viewport']['gpu']['sectionClippedActors']
    assert clipped > 0
    call('view.home')
    call('view.orientation', {'name': 'front'})
    call('view.zoom', {'factor': 2})
    call('capture.screenshot', {'path': str(out / 'adapted-top-section.png')})
    call('analysis.visibility', {'analysisId': section['id'], 'visible': False})

    # Return to the original authored model before export and persisted replay.
    call('document.open', {'path': str(source), 'discard': True})
    reopened = call('document.inspect')
    assert reopened['authored'] == original['authored']
    assert abs(body(reopened, bottom_id)['volumeMm3'] - bottom['volumeMm3']) < 1e-4
    assert abs(body(reopened, top_id)['volumeMm3'] - top['volumeMm3']) < 1e-4
    call('view.home')
    call('view.orientation', {'name': 'front'})
    call('capture.screenshot', {'path': str(out / 'final-halves-fit.png')})
    call('view.zoom', {'factor': 2.5})
    call('capture.screenshot', {'path': str(out / 'final-snaps-closeup.png')})

    # The closed halves share their split-seam face. A combined STL contains
    # coincident face triangles and is not a single manifold printable shell;
    # export each named half independently for printing and strict readback.
    stls = []
    steps = []
    for name, identity in (('Bottom', bottom_id), ('Top', top_id)):
        stl = out / f'{name.lower()}-snap-mm.stl'
        stl_export = call('document.exportStl', {'path': str(stl),
            'bodyId': identity, 'units': 'mm', 'format': 'binary',
            'surfaceDeviationMm': .1, 'normalDeviationDeg': 10})
        mesh = inspect_stl(stl, True, allow_cavities=True)
        assert len(mesh['components']) == 1
        stls.append({'name': name, 'export': stl_export, 'readback': mesh})
        exported = call('document.exportStep', {'path': str(out / f'{name.lower()}-snap-mm.step'),
                                              'bodyId': identity, 'units': 'mm'})
        assert abs(exported['volumeMm3'] - body(reopened, identity)['volumeMm3']) < 1e-4
        steps.append(exported)

    result = {'passed': True, 'scope': 'saved-model post-run verification tail',
        'source': str(source), 'sourceSha256': hashlib.sha256(source.read_bytes()).hexdigest(),
        'bottomBodyId': bottom_id, 'topBodyId': top_id,
        'bottomVolumeMm3': bottom['volumeMm3'], 'topVolumeMm3': top['volumeMm3'],
        'finalFilletId': final_fillet['id'], 'mirrorIds': [f['id'] for f in mirrors],
        'geometricEditRadiusMm': .45, 'originalRadiusMm': .5,
        'originalTopSectionAvailability': top_analysis['availability'],
        'adaptedSectionId': section['id'], 'adaptedSectionClippedActors': clipped,
        'stlExports': stls, 'stepExports': steps}
    (out / 'tail-result.json').write_text(json.dumps(result, indent=2))
    (out / 'tail-transcript.json').write_text(json.dumps(ledger, indent=2))
