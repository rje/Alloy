#!/usr/bin/env python3
"""One real-app sketch/profile/extrusion chain with paired history and storage."""
import argparse
import hashlib
import json
import math
from pathlib import Path
from app_harness import application


def near(value, expected):
    assert math.isclose(value, expected, rel_tol=1e-8, abs_tol=1e-6), (value, expected)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('app', 'output', 'fixture'):
        parser.add_argument('--' + name, required=True)
    args = parser.parse_args()
    output = Path(args.output).resolve()
    fixture = json.loads(Path(args.fixture).read_text())
    captures = []
    with application(args.app, output) as (call, transcript):
        initial = call('app.state')
        assert initial['viewport']['gpu']['hardwareAccelerated']
        session = call('sketch.begin', {'plane': 'XY', 'name': 'Extrusion source'})
        call('sketch.rectangle', {'x1': 0, 'y1': 0, 'x2': fixture['widthMm'],
                                 'y2': fixture['heightMm'], 'units': 'mm'})
        draft = call('command.preview', {'session': session['id']})['authoredDraft']
        points = [p['id'] for p in draft['parameters']['points']]
        edges = [e['id'] for e in draft['parameters']['entities']]
        call('sketch.constraint', {'type': 'fixed', 'pointIds': [points[0]]})
        call('sketch.dimension', {'kind': 'horizontal', 'entityId': edges[0],
                                 'value': fixture['widthMm'], 'annotation': {'uMm': 20, 'vMm': -10}})
        call('sketch.dimension', {'kind': 'vertical', 'entityId': edges[1],
                                 'value': fixture['heightMm'], 'annotation': {'uMm': 48, 'vMm': 12.5}})
        preview = call('command.preview', {'session': session['id']})
        sketch_id = preview['authoredDraft']['id']
        width_id = preview['authoredDraft']['parameters']['dimensions'][0]['id']
        profile_id = preview['sketchGeometry']['profiles'][0]['profileId']
        near(preview['sketchGeometry']['profiles'][0]['areaMm2'], 1000)
        call('sketch.finish')
        source = call('document.inspect')
        assert source['undo']['count'] == 1
        center = call('view.projectSketchPoint', {'plane': 'XY', 'uMm': 20, 'vMm': 12.5})
        picked = call('selection.pick', {'x': center['x'], 'y': center['y']})
        assert picked['kind'] == 'profile' and picked['profileId'] == profile_id, picked
        captures.append(call('capture.screenshot', {'path': str(output / 'finished-profile.png')}))

        extrude = call('command.begin', {'command': 'solid.extrude', 'inputs': {
            'sketchId': sketch_id, 'profileId': profile_id, 'distance': fixture['distanceMm'], 'units': 'mm'}})
        call('capture.dialog', {'path': str(output / 'extrusion-panel.png')})
        candidate = call('command.preview', {'session': extrude['id']})
        near(candidate['geometry']['volumeMm3'], 5000)
        assert candidate['geometry']['valid'] and candidate['geometry']['closed']
        ids = (candidate['featureId'], candidate['bodyId'])
        again = call('command.preview', {'session': extrude['id']})
        assert (again['featureId'], again['bodyId']) == ids
        assert call('document.inspect') == source
        call('view.home')
        camera = call('app.state')['viewport']['gpu']['camera']
        captures.append(call('capture.screenshot', {'path': str(output / 'extrusion-preview.png')}))
        call('command.update', {'session': extrude['id'], 'inputs': {'distance': 0}})
        call('command.preview', {'session': extrude['id']}, 'evaluation_failed')
        assert not call('app.state')['viewport']['gpu']['previewVisible']
        assert call('document.inspect') == source
        call('command.update', {'session': extrude['id'], 'inputs': {'distance': fixture['distanceMm']}})
        candidate = call('command.preview', {'session': extrude['id']})
        assert (candidate['featureId'], candidate['bodyId']) == ids
        assert call('app.state')['viewport']['gpu']['camera'] == camera
        call('command.commit', {'session': extrude['id']})
        committed = call('document.inspect')
        assert committed['undo']['count'] == 2 and committed['bodies'] == 1
        assert not committed['authored']['features'][0]['visible']
        near(committed['evaluation']['bodies'][0]['volumeMm3'], 5000)
        call('history.undo')
        assert call('document.inspect')['authored'] == source['authored']
        call('history.redo')
        assert call('document.inspect')['authored'] == committed['authored']
        captures.append(call('capture.screenshot', {'path': str(output / 'committed-extrusion.png')}))

        edit = call('sketch.edit', {'sketchId': sketch_id})
        hidden = call('app.state')['viewport']['gpu']['editDisplay']
        assert ids[1] in hidden['hiddenBodyIds'], hidden
        call('sketch.dimensionUpdate', {'dimensionId': width_id, 'value': fixture['editedWidthMm']})
        call('sketch.finish')
        changed = call('document.inspect')
        assert changed['undo']['count'] == 3
        near(changed['evaluation']['sketches'][0]['profiles'][0]['areaMm2'], 1250)
        near(changed['evaluation']['bodies'][0]['volumeMm3'], 6250)
        assert changed['evaluation']['sketches'][0]['profiles'][0]['profileId'] == profile_id
        assert (changed['authored']['features'][1]['id'], changed['authored']['features'][1]['bodyId']) == ids
        call('history.undo')
        assert call('document.inspect')['authored'] == committed['authored']
        near(call('document.inspect')['evaluation']['bodies'][0]['volumeMm3'], 5000)
        call('history.redo')
        assert call('document.inspect')['authored'] == changed['authored']
        near(call('document.inspect')['evaluation']['bodies'][0]['volumeMm3'], 6250)
        changed = call('document.inspect')
        call('view.home')
        captures.append(call('capture.screenshot', {'path': str(output / 'upstream-width-edit.png')}))

        reedit = call('feature.editDialog', {'featureId': ids[0]})
        call('command.update', {'session': reedit['id'], 'inputs': {'distance': -7}})
        reverse = call('command.preview', {'session': reedit['id']})
        near(reverse['geometry']['volumeMm3'], 8750)
        assert reverse['geometry']['boundsMm']['min'][2] < 0
        call('command.cancel', {'session': reedit['id']})
        assert call('document.inspect') == changed
        call('feature.delete', {'featureId': sketch_id}, 'evaluation_failed')
        assert call('document.inspect') == changed
        call('feature.edit', {'featureId': ids[0], 'parameters': {'profileId': '11111111-1111-4111-8111-111111111111'}}, 'evaluation_failed')
        assert call('document.inspect') == changed

        path = output / 'sketch-extrusion.alloy'
        call('document.save', {'path': str(path)})
        call('document.new', {'discard': False})
        call('document.open', {'path': str(path)})
        reopened = call('document.inspect')
        assert reopened['authored'] == changed['authored']
        assert reopened['undo']['count'] == 0
        near(reopened['evaluation']['bodies'][0]['volumeMm3'], 6250)
        assert reopened['evaluation']['sketches'][0]['profiles'][0]['profileId'] == profile_id
        final = call('app.state')
    result = {
        'result': 'passed', 'fixtureSha256': hashlib.sha256(Path(args.fixture).read_bytes()).hexdigest(),
        'verifiedCapabilities': ['history.ordered_recompute', 'history.profile_dependencies',
            'solids.profile_extrusion', 'solids.extrusion_preview', 'solids.extrusion_storage',
            'sketch.finished_profile_selection'],
        'initialState': initial, 'finalState': final, 'transcript': transcript, 'captures': captures,
        'evidenceDimensions': {'api': {'status': 'passed'}, 'hardware': {'status': 'passed'},
            'geometry': {'status': 'passed'}, 'save_reopen': {'status': 'passed'},
            'visual': {'status': 'captured', 'reason': 'Actual profile, preview and edited solid; physical input and visual review remain separate.'}}
    }
    (output / 'sketch-extrude-api.json').write_text(json.dumps(result, indent=2) + '\n')
    print('PASS: sketch/profile/extrusion ordered edit, preview/history/storage chain')


if __name__ == '__main__':
    main()
