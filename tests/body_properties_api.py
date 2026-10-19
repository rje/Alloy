#!/usr/bin/env python3
"""Selected solid Properties: canonical OCCT data, display units, edits, history and file."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application


def near(actual, expected):
    assert math.isclose(actual, expected, rel_tol=1e-8, abs_tol=1e-6), (actual, expected)


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--app', required=True)
parser.add_argument('--output', required=True)
args = parser.parse_args()
out = Path(args.output).resolve()
out.mkdir(parents=True, exist_ok=True)
with application(args.app, out) as (call, transcript):
    call('body.properties', error='body_required')
    call('document.parameterAdd', {'name': 'Width', 'value': 25.4, 'units': 'mm'})
    parameter = call('document.inspect')['userParameters'][0]['id']
    call('solid.box', {'widthParameter': 'Width', 'depth': 12.7, 'height': 6.35,
                       'units': 'mm', 'name': 'Asymmetric stock'})
    initial = call('document.inspect')
    body = initial['evaluation']['bodies'][0]
    body_id = body['bodyId']
    call('selection.set', {'bodyId': body_id})
    before_inspect = call('document.inspect')
    properties = call('body.properties')
    assert properties['bodyId'] == body_id and properties['bodyName'] == 'Asymmetric stock'
    assert properties['componentId'] == initial['authored']['documentId']
    assert properties['featureId'] == initial['authored']['features'][-1]['id']
    assert properties['canonical']['boundsMm'] == body['boundsMm']
    for actual, expected in zip(properties['display']['size'], (25.4, 12.7, 6.35)):
        near(actual, expected)
    area = 2 * (25.4 * 12.7 + 25.4 * 6.35 + 12.7 * 6.35)
    volume = 25.4 * 12.7 * 6.35
    near(properties['canonical']['areaMm2'], area)
    near(properties['canonical']['volumeMm3'], volume)
    assert call('document.inspect') == before_inspect
    call('document.units', {'units': 'in'})
    inches = call('body.properties')
    assert inches['bodyId'] == body_id
    assert inches['canonical'] == properties['canonical']
    assert inches['display']['lengthUnit'] == 'in'
    for actual, expected in zip(inches['display']['size'], (1, 0.5, 0.25)):
        near(actual, expected)
    near(inches['display']['area'], area / 25.4**2)
    near(inches['display']['volume'], volume / 25.4**3)
    gpu = call('app.state')['viewport']['gpu']
    assert gpu['hardwareAccelerated']
    call('view.fit')
    call('capture.screenshot', {'path': str(out / 'selected-inch-solid.png')})
    call('document.parameterUpdate', {'parameterId': parameter, 'name': 'Width', 'value': 50.8})
    changed = call('document.inspect')
    changed_properties = call('body.properties')
    assert changed_properties['bodyId'] == body_id
    near(changed_properties['display']['size'][0], 2)
    near(changed_properties['canonical']['volumeMm3'], 50.8 * 12.7 * 6.35)
    call('history.undo')
    assert call('body.properties')['canonical'] == properties['canonical']
    call('history.redo')
    assert call('body.properties')['canonical'] == changed_properties['canonical']
    call('body.properties', {'bodyId': '00000000-0000-0000-0000-000000000001'}, error='stale_body')
    saved = out / 'properties-stock.alloy'
    call('document.save', {'path': str(saved)})
    call('document.new', {'discard': False})
    call('document.open', {'path': str(saved)})
    call('selection.set', {'bodyId': body_id})
    assert call('body.properties')['canonical'] == changed_properties['canonical']
    assert call('document.inspect')['authored'] == changed['authored']
    call('component.createFromBodies', {'bodyIds': [body_id]})
    component = call('document.inspect')['authored']
    original_occurrence = component['occurrences'][0]['id']
    copied = call('component.copyOccurrence', {'occurrenceId': original_occurrence,
        'placement': {'axis': 'Z', 'angleDeg': 0, 'translationMm': [100, 0, 0]}})
    second_occurrence = copied['rootOccurrenceId']
    call('body.properties', {'bodyId': body_id}, error='ambiguous_instance')
    first = call('body.properties', {'bodyId': body_id, 'occurrenceId': original_occurrence})
    second = call('body.properties', {'bodyId': body_id, 'occurrenceId': second_occurrence})
    assert first['occurrenceId'] == original_occurrence
    assert second['occurrenceId'] == second_occurrence
    near(second['canonical']['boundsMm']['min'][0] - first['canonical']['boundsMm']['min'][0], 100)
    near(second['canonical']['areaMm2'], first['canonical']['areaMm2'])
    near(second['canonical']['volumeMm3'], first['canonical']['volumeMm3'])
    call('history.undo')
    call('body.properties', {'bodyId': body_id, 'occurrenceId': second_occurrence}, error='stale_instance')
    assert call('body.properties', {'bodyId': body_id, 'occurrenceId': original_occurrence})['occurrenceId'] == original_occurrence
    (out / 'receipt.json').write_text(json.dumps({'result': 'passed', 'calls': len(transcript),
        'bodyId': body_id, 'gpu': gpu, 'transcript': transcript}, indent=2) + '\n')
print('PASS: selected solid Properties follow active units and authored edits')
