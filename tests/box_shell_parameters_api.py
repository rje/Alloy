#!/usr/bin/env python3
"""Bounded Box and Shell direct-name lifecycle through the actual Alloy app."""
import argparse
import json
from pathlib import Path
from app_harness import application


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    output = Path(args.output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    with application(args.app, output) as (call, transcript):
        inspect = lambda: call('document.inspect')
        for name, value in [('Width', 50), ('Length', 80), ('Height', 40), ('Thickness', 4)]:
            call('document.parameterAdd', {'name': name, 'value': value, 'units': 'mm'})
        ids = {row['name']: row['id'] for row in inspect()['userParameters']}
        call('solid.box', {'widthParameter': 'Width', 'depthParameter': 'Length',
                           'heightParameter': 'Height', 'units': 'mm'})
        box = inspect()['authored']['features'][-1]
        assert all(box['parameters'][field + 'ParameterId'] == ids[name] for field, name in
                   [('width', 'Width'), ('depth', 'Length'), ('height', 'Height')])
        assert abs(inspect()['evaluation']['bodies'][0]['volumeMm3'] - 160000) < 1e-5
        edit = call('feature.editDialog', {'featureId': box['id']})
        assert all(edit['inputs'][field + 'Parameter'] == name for field, name in
                   [('width', 'Width'), ('depth', 'Length'), ('height', 'Height')])
        call('command.cancel', {'session': edit['id']})
        call('document.parameterUpdate', {'parameterId': ids['Width'], 'name': 'NewWidth', 'value': 60})
        assert abs(inspect()['evaluation']['bodies'][0]['volumeMm3'] - 192000) < 1e-5
        assert inspect()['authored']['features'][-1]['parameters']['widthParameterId'] == ids['Width']
        before = inspect()['authored']
        call('history.undo')
        call('history.redo')
        assert inspect()['authored'] == before
        box_path = output / 'bound-box.alloy'
        call('document.save', {'path': str(box_path)})
        call('document.open', {'path': str(box_path), 'discard': True})
        assert inspect()['authored'] == before
        call('document.new', {'discard': True})

        call('document.parameterAdd', {'name': 'Thickness', 'value': 4, 'units': 'mm'})
        wall_id = inspect()['userParameters'][0]['id']
        call('sketch.begin', {'plane': 'XY'})
        call('sketch.rectangle', {'x1': 0, 'y1': 0, 'x2': 25, 'y2': 25, 'units': 'mm'})
        stock_sketch = call('app.state')['activeSession']['draftSketch']
        call('sketch.finish')
        profile = inspect()['evaluation']['sketches'][0]['profiles'][0]['profileId']
        call('solid.extrude', {'sketchId': stock_sketch['id'], 'profileId': profile,
                               'distance': 12.5, 'units': 'mm'})
        body = inspect()['evaluation']['bodies'][0]
        face = next(f for f in body['supportFaces'] if f['role'] == 'extrude.endCap')
        ref = {key: face[key] for key in ('producerFeatureId', 'bodyId', 'faceId')}
        call('solid.shell', {'targetBodyId': body['bodyId'], 'removedFace': ref,
                             'thicknessParameter': 'Thickness'})
        shell = inspect()['authored']['features'][-1]
        assert shell['parameters']['thicknessParameterId'] == wall_id
        edit = call('feature.editDialog', {'featureId': shell['id']})
        assert edit['inputs']['thicknessParameter'] == 'Thickness'
        call('command.cancel', {'session': edit['id']})
        call('sketch.begin', {'plane': 'XZ'})
        call('sketch.line', {'x1': 0, 'y1': 0, 'x2': 4, 'y2': 1, 'units': 'mm'})
        guide = call('app.state')['activeSession']['draftSketch']
        line = guide['parameters']['entities'][0]
        call('sketch.dimension', {'kind': 'horizontal', 'entityId': line['id'],
                                  'parameterName': 'Thickness'})
        call('sketch.finish')
        call('document.parameterUpdate', {'parameterId': wall_id, 'name': 'Wall', 'value': 3.5})
        changed = inspect()
        assert changed['authored']['features'][-2]['parameters']['thicknessParameterId'] == wall_id
        assert abs(changed['authored']['features'][-1]['parameters']['dimensions'][0]['valueMm'] - 3.5) < 1e-6
        call('document.parameterUpdate', {'parameterId': wall_id, 'value': 20}, 'parameter_evaluation_failed')
        assert inspect()['authored'] == changed['authored']
        call('history.undo')
        call('history.redo')
        assert inspect()['authored'] == changed['authored']
        path = output / 'bound-shell.alloy'
        call('document.save', {'path': str(path)})
        call('document.open', {'path': str(path), 'discard': True})
        assert inspect()['authored'] == changed['authored']
        call('capture.screenshot', {'path': str(output / 'bound-shell.png')})
        (output / 'api-state.json').write_text(json.dumps(inspect(), indent=2) + '\n')
    (output / 'api-transcript.json').write_text(json.dumps(transcript, indent=2) + '\n')
    print('PASS: real-app Box and Shell direct names, update, refusal, undo, and reopen')


if __name__ == '__main__':
    main()
