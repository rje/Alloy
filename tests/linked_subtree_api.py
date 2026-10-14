#!/usr/bin/env python3
"""Actual-app linked piston copy, shared edit, placement and persistence gate."""
import argparse
import json
from pathlib import Path

from app_harness import application

parser = argparse.ArgumentParser()
parser.add_argument('--app', required=True)
parser.add_argument('--fixture', required=True)
parser.add_argument('--output', required=True)
args = parser.parse_args()
out = Path(args.output).resolve()
out.mkdir(parents=True, exist_ok=True)

with application(args.app, out, command_timeout=90) as (call, ledger):
    call('document.open', {'path': str(Path(args.fixture).resolve()), 'discard': True})
    def authored():
        return call('document.inspect')['authored']
    def instances():
        return {r['occurrenceId']: r for r in call('component.inspect')['localInstances']}
    original = authored()
    definitions = {c['name']: c['id'] for c in original['components']}
    occurrences = {o['definitionId']: o['id'] for o in original['occurrences']}
    parent = occurrences[definitions['Piston subassembly']]
    inner = occurrences[definitions['Inner piston']]
    outer = occurrences[definitions['Outer piston']]
    result = call('component.copyOccurrence', {
        'occurrenceId': parent,
        'placement': {'axis': 'Z', 'angleDeg': 0, 'translationMm': [-180, 0, 0]}})
    mapping = result['occurrenceMap']
    assert set(mapping) == {parent, inner, outer}
    assert len(set(mapping.values())) == 3 and not set(mapping.values()) & set(mapping)
    copied = authored()
    assert copied['features'] == original['features']
    assert copied['components'] == original['components']
    rows = instances()
    assert len(rows) == len(original['occurrences']) + 3
    for source in (inner, outer):
        a, b = rows[source], rows[mapping[source]]
        assert a['definitionId'] == b['definitionId']
        assert b['parentOccurrenceId'] == result['rootOccurrenceId']
        assert a['path'] != b['path']
        assert len(a['bodies']) == len(b['bodies']) == 1
        assert a['bodies'][0]['bodyId'] == b['bodies'][0]['bodyId']
        for bound in ('min', 'max'):
            aa, bb = a['bodies'][0]['boundsMm'][bound], b['bodies'][0]['boundsMm'][bound]
            assert all(abs(bb[k] - aa[k] - (-180 if k == 0 else 0)) < 1e-5 for k in range(3))
    call('view.home')
    call('capture.screenshot', {'path': str(out/'two-linked-pistons.png')})
    body_id = rows[inner]['bodies'][0]['bodyId']
    call('selection.set', {'bodyId': body_id}, error='ambiguous_instance')
    exported = call('document.exportStl', {'occurrenceId': parent, 'path': str(out/'parent.stl'),
                               'units': 'mm', 'format': 'binary', 'surfaceDeviationMm': 0.1,
                               'normalDeviationDeg': 10})
    assert exported['bodyCount'] == 2
    for occurrence in (inner, mapping[inner]):
        call('selection.set', {'bodyId': body_id, 'occurrenceId': occurrence})
        call('capture.screenshot', {'path': str(out/f'selected-{occurrence}.png')})
    source_body = next(b for b in call('document.inspect')['evaluation']['bodies'] if b['bodyId'] == body_id)
    face = source_body['supportFaces'][0]
    support = {key: face[key] for key in ('bodyId', 'faceId', 'producerFeatureId')}
    support['occurrenceId'] = mapping[inner]
    call('selection.faceSet', {'support': support})
    call('sketch.create', {}, error='unsupported_instance_edit')
    assert authored() == copied, 'Instance selection must not edit shared source'
    call('selection.set', {'bodyId': ''})
    call('history.undo')
    assert authored() == original
    call('history.redo')
    assert authored() == copied
    feature = next(f for f in copied['features'] if f.get('bodyId') == body_id and f['type'] == 'cylinder')
    call('feature.edit', {'featureId': feature['id'],
                         'parameters': {**feature['parameters'], 'heightMm': feature['parameters']['heightMm'] + 7}})
    edited = authored()
    updated = instances()
    for occurrence in (inner, mapping[inner]):
        before = rows[occurrence]['bodies'][0]['boundsMm']['max'][2]
        after = updated[occurrence]['bodies'][0]['boundsMm']['max'][2]
        assert abs(after - before - 7) < 1e-5, 'Shared source edit must update both placed shapes'
    assert edited['occurrences'] == copied['occurrences']
    saved = out/'linked-pistons.alloy'
    call('document.save', {'path': str(saved)})
    call('document.open', {'path': str(saved), 'discard': True})
    assert authored() == edited
    assert instances() == updated
    call('view.home')
    call('capture.screenshot', {'path': str(out/'linked-pistons-reopened.png')})
    (out/'receipt.json').write_text(json.dumps({
        'passed': True, 'scope': 'Unjointed shared-definition linked subtree; no internal joint remap',
        'occurrenceMap': mapping, 'instances': updated, 'ledger': ledger}, indent=2)+'\n')
    print('PASS linked subtree placement, shared edit, selection immutability, history/reopen', flush=True)
