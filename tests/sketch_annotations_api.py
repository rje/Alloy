#!/usr/bin/env python3
"""Real-app placed dimensions and one selectable, evaluated line profile."""
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
        before = call('document.inspect')
        session = call('sketch.begin', {'plane': 'XY', 'name': 'Placed dimensions'})

        def preview():
            return call('command.preview', {'session': session['id']})

        def pixels(u, v):
            p = call('view.projectSketchPoint', {'plane': 'XY', 'uMm': u, 'vMm': v})
            return {'x': p['x'], 'y': p['y']}

        call('sketch.rectangle', {'x1': 0, 'y1': 0, 'x2': fixture['widthMm'], 'y2': fixture['heightMm'], 'units': 'mm'})
        draft = preview()['authoredDraft']
        points = [p['id'] for p in draft['parameters']['points']]
        entities = [e['id'] for e in draft['parameters']['entities']]
        call('sketch.constraint', {'type': 'fixed', 'pointIds': [points[0]]})
        profile = preview()['sketchGeometry']['profiles'][0]
        profile_id = profile['profileId']
        near(profile['areaMm2'], fixture['widthMm'] * fixture['heightMm'])
        picked = call('sketch.selectionPick', pixels(20, 12.5))
        assert picked['picked']['kind'] == 'profile' and picked['profileIds'] == [profile_id], picked

        # Placement is disposable and cannot add dimensions until numeric acceptance.
        call('sketch.selectionSet', {'entityIds': [entities[0]]})
        usable = preview()['authoredDraft']
        pending = call('sketch.dimensionPreview', {**pixels(20, -10), 'kind': 'auto'})
        assert pending['phase'] == 'placing' and pending['kind'] == 'horizontal'
        assert preview()['authoredDraft'] == usable
        call('sketch.dimensionCancel')
        assert preview()['authoredDraft'] == usable
        call('sketch.dimensionPreview', pixels(20, -10))
        call('sketch.dimensionPlace', pixels(20, -10))
        assert call('app.state')['inlineEditor']['visible']
        captures.append(call('capture.screenshot', {'path': str(output / 'inline-editor.png')}))
        call('sketch.dimensionAccept', {'text': '1+2'}, 'invalid_dimension_value')
        assert preview()['authoredDraft'] == usable
        call('sketch.dimensionAccept', {'text': str(fixture['widthMm']) + ' mm'})
        assert not call('app.state')['inlineEditor']['visible']
        width_id = preview()['authoredDraft']['parameters']['dimensions'][-1]['id']

        call('sketch.selectionSet', {'entityIds': [entities[1]]})
        call('sketch.dimensionPreview', pixels(48, 12.5))
        call('sketch.dimensionPlace', pixels(48, 12.5))
        call('sketch.dimensionAccept', {'value': fixture['heightMm'], 'units': 'mm'})
        call('sketch.selectionSet', {'pointIds': [points[0], points[2]]})
        call('sketch.dimensionPreview', {**pixels(22, 16), 'kind': 'linear', 'driving': False})
        call('sketch.dimensionPlace', pixels(22, 16))
        call('sketch.dimensionAccept')
        driven_id = preview()['authoredDraft']['parameters']['dimensions'][-1]['id']
        call('sketch.dimensionEdit', {'dimensionId': driven_id})
        assert call('app.state')['inlineEditor']['readOnly']
        call('sketch.dimensionCancel')

        call('sketch.dimensionEdit', {'dimensionId': width_id})
        call('sketch.dimensionAccept', {'text': str(fixture['editedWidthMm'] / 25.4) + ' in'})
        edited = preview()
        near(edited['sketchGeometry']['profiles'][0]['areaMm2'], fixture['editedWidthMm'] * fixture['heightMm'])
        assert edited['sketchGeometry']['profiles'][0]['profileId'] == profile_id
        assert edited['sketchGeometry']['solver']['dof'] == 0
        old_points = edited['authoredDraft']['parameters']['points']
        call('sketch.annotationMove', {'dimensionId': width_id, 'uMm': 25, 'vMm': -14})
        placed = preview()
        assert placed['authoredDraft']['parameters']['points'] == old_points
        for old, new in zip(old_points, placed['authoredDraft']['parameters']['points']):
            assert old['id'] == new['id']
            near(new['uMm'], old['uMm'])
            near(new['vMm'], old['vMm'])
        width = next(d for d in placed['authoredDraft']['parameters']['dimensions'] if d['id'] == width_id)
        assert width['annotation'] == {'uMm': 25, 'vMm': -14}
        near(width['valueMm'], fixture['editedWidthMm'])
        annotations = call('app.state')['viewport']['sketchAnnotations']
        assert len(annotations) == 3
        vertical = next(a for a in annotations if a['kind'] == 'vertical')
        near(abs(vertical['label']['angleDegrees']), 90)
        assert all(len(a['witnesses']) == 2 and len(a['line']) == 2 for a in annotations)
        assert next(a for a in annotations if a['dimensionId'] == driven_id)['label']['text'].startswith('(')
        captures.append(call('capture.screenshot', {'path': str(output / 'placed-dimensions.png')}))
        assert call('document.inspect') == before
        call('sketch.finish')
        committed = call('document.inspect')
        assert committed['undo']['count'] == 1 and committed['bodies'] == 0
        call('history.undo')
        assert call('document.inspect')['authored'] == before['authored']
        call('history.redo')
        assert call('document.inspect')['authored'] == committed['authored']
        path = output / 'placed-rectangle.alloy'
        call('document.save', {'path': str(path)})
        call('document.new', {'discard': False})
        call('document.open', {'path': str(path)})
        reopened = call('document.inspect')
        assert reopened['authored'] == committed['authored']
        assert reopened['evaluation']['sketches'][0]['profiles'][0]['profileId'] == profile_id
        near(reopened['evaluation']['sketches'][0]['profiles'][0]['areaMm2'], 1250)
    result = {
        'result': 'passed', 'fixtureSha256': hashlib.sha256(Path(args.fixture).read_bytes()).hexdigest(),
        'verifiedCapabilities': ['sketch.annotation_placement', 'sketch.inline_dimension_commands',
            'sketch.annotation_storage', 'sketch.evaluated_line_profiles', 'sketch.profile_selection'],
        'initialState': initial, 'transcript': transcript, 'captures': captures,
        'evidenceDimensions': {'api': {'status': 'passed'}, 'hardware': {'status': 'passed'},
            'geometry': {'status': 'passed'}, 'save_reopen': {'status': 'passed'},
            'visual': {'status': 'captured', 'reason': 'Actual GPU annotations and inline editor; native input and reference review remain separate.'}}
    }
    (output / 'sketch-annotations-api.json').write_text(json.dumps(result, indent=2) + '\n')
    print('PASS: placed dimensions, inline commands, annotation storage and evaluated profile selection')


if __name__ == '__main__':
    main()
