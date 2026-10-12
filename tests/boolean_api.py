#!/usr/bin/env python3
"""Real-app explicit Combine preview, history, and storage workflow."""
import argparse
import hashlib
import json
import math
from pathlib import Path
from app_harness import application


def near(actual, expected):
    assert math.isclose(actual, expected, rel_tol=1e-8, abs_tol=1e-6), (actual, expected)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--fixture', required=True)
    args = parser.parse_args()
    output = Path(args.output).resolve()
    fixture = json.loads(Path(args.fixture).read_text())
    with application(args.app, output) as (call, transcript):
        initial = call('app.state')
        assert initial['viewport']['gpu']['hardwareAccelerated']
        call('solid.box', fixture['plate'])
        call('solid.cylinder', fixture['tool'])
        source = call('document.inspect')
        target, tool = [f['bodyId'] for f in source['authored']['features']]
        session = call('command.begin', {'command': 'solid.combine', 'inputs': {
            'targetBodyId': target, 'toolBodyIds': [tool], 'operation': 'cut', 'keepTools': False}})
        candidate = call('command.preview', {'session': session['id']})
        near(candidate['geometry']['volumeMm3'], 3000 - 45 * math.pi)
        assert call('document.inspect') == source
        call('view.home')
        call('capture.screenshot', {'path': str(output / 'cut-preview.png')})
        call('capture.dialog', {'path': str(output / 'combine-panel.png')})
        call('command.cancel', {'session': session['id']})
        assert call('document.inspect') == source
        session = call('command.begin', {'command': 'solid.combine', 'inputs': {
            'targetBodyId': target, 'toolBodyIds': [tool], 'operation': 'cut', 'keepTools': False}})
        call('command.commit', {'session': session['id']})
        cut = call('document.inspect')
        assert cut['bodies'] == 1 and cut['undo']['count'] == source['undo']['count'] + 1
        assert cut['evaluation']['bodies'][0]['bodyId'] == target
        near(cut['evaluation']['bodies'][0]['volumeMm3'], 3000 - 45 * math.pi)
        boolean_id = cut['authored']['features'][-1]['id']
        call('history.undo')
        assert call('document.inspect')['authored'] == source['authored']
        call('history.redo')
        assert call('document.inspect')['authored'] == cut['authored']
        cut = call('document.inspect')
        edit = call('feature.editDialog', {'featureId': boolean_id})
        # Consumed tools must reappear at the feature's input stage for canvas editing.
        call('solid.combineSelectionMode', {'role': 'tools'})
        call('view.orientation', {'name': 'top'})
        point = call('view.projectSketchPoint', {'plane': 'XY', 'uMm': 15, 'vMm': 10})
        picked = call('selection.pick', {'x': point['x'], 'y': point['y']})
        assert picked['bodyId'] == tool, picked
        assert picked['combineInputs']['toolBodyIds'] == [], picked
        call('command.update', {'session': edit['id'], 'inputs': {'toolBodyIds': [tool]}})
        call('command.update', {'session': edit['id'], 'inputs': {'operation': 'intersect'}})
        near(call('command.preview', {'session': edit['id']})['geometry']['volumeMm3'], 45 * math.pi)
        call('command.update', {'session': edit['id'], 'inputs': {'operation': 'join', 'keepTools': True}})
        near(call('command.preview', {'session': edit['id']})['geometry']['volumeMm3'], 3000 + 45 * math.pi)
        call('command.cancel', {'session': edit['id']})
        assert call('document.inspect') == cut
        plate_id = source['authored']['features'][0]['id']
        call('feature.edit', {'featureId': plate_id, 'parameters': {'widthMm': 40}})
        edited = call('document.inspect')
        near(edited['evaluation']['bodies'][0]['volumeMm3'], 4000 - 45 * math.pi)
        call('view.home')
        call('capture.screenshot', {'path': str(output / 'edited-cut.png')})
        path = output / 'editable-cut.alloy'
        call('document.save', {'path': str(path)})
        call('document.new', {'discard': False})
        call('document.open', {'path': str(path)})
        reopened = call('document.inspect')
        assert reopened['authored'] == edited['authored']
        assert reopened['undo']['count'] == 0
        near(reopened['evaluation']['bodies'][0]['volumeMm3'], 4000 - 45 * math.pi)
    (output / 'boolean-api.json').write_text(json.dumps({
        'result': 'passed', 'fixtureSha256': hashlib.sha256(Path(args.fixture).read_bytes()).hexdigest(), 'initialState': initial, 'transcript': transcript,
        'verifiedCapabilities': ['solids.combine', 'solids.combine_preview', 'solids.combine_storage'],
        'evidenceDimensions': {'api': {'status': 'passed'}, 'geometry': {'status': 'passed'},
            'hardware': {'status': 'passed'}, 'save_reopen': {'status': 'passed'},
            'visual': {'status': 'captured', 'reason': 'Independent visual/native review remains required.'}}
    }, indent=2) + '\n')
    print('PASS: explicit Combine preview, history, upstream edit and reopen')


if __name__ == '__main__':
    main()
