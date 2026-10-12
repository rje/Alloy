#!/usr/bin/env python3
"""Bounded local Cut checkpoint using a supplied five-circle native fixture and live Alloy."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'tools'))
from alloyctl import choose_endpoint, request


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ready-file', required=True)
    parser.add_argument('--fixture', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    output = Path(args.output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    fixture = Path(args.fixture).resolve()
    endpoint = choose_endpoint(ready_file=args.ready_file)
    transcript, captures = [], []

    def call(command, parameters=None):
        response = request(endpoint, command, parameters, timeout=30)
        transcript.append({'command': command, 'args': parameters or {}, 'response': response})
        assert response['ok'], response
        return response['result']

    result = {'result': 'failed', 'covers': [], 'fixtureSha256': hashlib.sha256(fixture.read_bytes()).hexdigest(),
              'fixturePath': str(fixture), 'testSha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              'transcript': transcript, 'captures': captures}
    try:
        result['initialState'] = call('app.state')
        call('document.open', {'path': str(fixture), 'discard': True})
        baseline = call('document.inspect')
        assert baseline['bodies'] == 1
        body = baseline['evaluation']['bodies'][0]
        target_id, producer_id = body['bodyId'], body['featureId']
        hole_features = [f for f in baseline['authored']['features'] if f['type'] == 'sketch'
                         and f['parameters']['plane'] == 'face'
                         and sum(e['type'] == 'circle' for e in f['parameters']['entities']) == 5]
        assert len(hole_features) == 1, 'Fixture needs one finished supported five-circle sketch'
        holes = hole_features[0]
        sketch_id = holes['id']
        diameter = [d for d in holes['parameters']['dimensions'] if d['kind'] == 'diameter' and d['driving']]
        assert len(diameter) == 1 and math.isclose(diameter[0]['valueMm'], 50)
        assert sum(c['type'] == 'equal' for c in holes['parameters']['constraints']) == 4
        profiles = next(s for s in baseline['evaluation']['sketches'] if s['sketchId'] == sketch_id)['profiles']
        profile_ids = sorted(p['profileId'] for p in profiles)
        assert len(profile_ids) == 5
        stock_depth = next(f for f in baseline['authored']['features'] if f['id'] == producer_id)['parameters']['distanceMm']
        assert stock_depth > 0 and stock_depth < 100
        original_volume = body['volumeMm3']

        def inspect_cut(document, diameter_mm, cut_id, distance):
            assert document['bodies'] == 1
            solid = document['evaluation']['bodies'][0]
            assert solid['bodyId'] == target_id and solid['featureId'] == producer_id
            assert solid['lastModifierFeatureId'] == cut_id and solid['operation'] == 'cut'
            assert solid['valid'] and solid['closed'] and solid['topology']['solids'] == 1
            assert set(solid['sourceProfileIds']) == set(profile_ids)
            assert solid['targetBodyIds'] == [target_id] and solid['distanceMm'] == distance
            removed = 5 * math.pi * (diameter_mm / 2) ** 2 * stock_depth
            assert math.isclose(solid['removedVolumeMm3'], removed, rel_tol=1e-8, abs_tol=1e-4)
            assert math.isclose(solid['volumeMm3'], original_volume - removed, rel_tol=1e-8, abs_tol=1e-4)
            cut = next(f for f in document['authored']['features'] if f['id'] == cut_id)
            assert cut['type'] == 'extrude' and 'bodyId' not in cut
            assert cut['parameters']['operation'] == 'cut' and cut['parameters']['targetBodyIds'] == [target_id]
            assert sorted(cut['parameters']['profileIds']) == profile_ids
            return solid

        for index, profile_id in enumerate(profile_ids):
            selection = call('selection.profileSet', {'sketchId': sketch_id, 'profileId': profile_id, 'additive': index > 0})
        assert sorted(selection['profileIds']) == profile_ids
        session = call('solid.pressPullDialog')  # Shared Q command, separate from physical key evidence.
        assert sorted(session['inputs']['profileIds']) == profile_ids
        call('command.update', {'session': session['id'], 'inputs': {'distance': -200, 'units': 'mm',
             'operation': 'cut', 'targetBodyIds': [target_id]}})
        preview = call('command.preview', {'session': session['id']})
        assert preview['geometryEvaluated'] and preview['bodyId'] == target_id
        assert preview['geometry']['valid'] and preview['geometry']['closed']
        captures.append(call('capture.screenshot', {'path': str(output / 'five-tools-cut-preview.png')}))
        state = call('app.state')
        assert state['viewport']['gpu']['cutPreviewToolCount'] == 5
        assert state['viewport']['gpu']['cutPreviewSelectedProfileCount'] == 5
        assert state['viewport']['gpu']['extrusionCue']['operation'] == 'cut'
        assert state['viewport']['gpu']['extrusionCue']['distanceMm'] == -200
        assert state['viewport']['gpu']['previewReplacedBodyId'] == target_id
        assert call('document.inspect')['authored'] == baseline['authored'], 'Preview authored geometry'
        call('command.commit', {'session': session['id']})
        committed = call('document.inspect')
        cut_id = committed['authored']['features'][-1]['id']
        assert len(committed['authored']['features']) == len(baseline['authored']['features']) + 1
        assert committed['undo']['count'] == baseline['undo']['count'] + 1
        inspect_cut(committed, 50, cut_id, -200)
        captures.append(call('capture.screenshot', {'path': str(output / 'five-through-holes.png')}))

        edit = call('sketch.edit', {'sketchId': sketch_id})
        assert edit['resolvedFrame']['origin'][2] == stock_depth
        call('sketch.dimensionUpdate', {'dimensionId': diameter[0]['id'], 'value': 60, 'units': 'mm'})
        call('sketch.finish')
        resized = call('document.inspect')
        inspect_cut(resized, 60, cut_id, -200)
        assert resized['undo']['count'] == committed['undo']['count'] + 1
        cut_session = call('feature.editDialog', {'featureId': cut_id})
        assert cut_session['inputs']['targetBodyIds'] == [target_id]
        call('command.update', {'session': cut_session['id'], 'inputs': {'distance': -100, 'units': 'mm'}})
        cut_preview = call('command.preview', {'session': cut_session['id']})
        assert cut_preview['geometryEvaluated'] and cut_preview['bodyId'] == target_id
        call('command.commit', {'session': cut_session['id']})
        edited = call('document.inspect')
        inspect_cut(edited, 60, cut_id, -100)
        call('history.undo')
        assert call('document.inspect')['authored'] == resized['authored']
        call('history.undo')
        assert call('document.inspect')['authored'] == committed['authored']
        call('history.redo')
        call('history.redo')
        assert call('document.inspect')['authored'] == edited['authored']
        captures.append(call('capture.screenshot', {'path': str(output / 'edited-cut.png')}))
        saved_path = output / 'five-hole-cut.alloy'
        call('document.save', {'path': str(saved_path)})
        saved = call('document.inspect')['authored']
        call('document.new', {'discard': False})
        call('document.open', {'path': str(saved_path)})
        reopened = call('document.inspect')
        assert reopened['authored'] == saved
        inspect_cut(reopened, 60, cut_id, -100)
        result.update(result='passed', finalState=call('app.state'),
                      verifiedCapabilities=['solid.profile_cut', 'sketch.edit_before_modifier'],
                      evidenceDimensions={'api': {'status': 'passed'}, 'geometry': {'status': 'passed'},
                        'save_reopen': {'status': 'passed'}, 'visual': {'status': 'captured'},
                        'physical_input': {'status': 'not_run', 'reason': 'Shared command API; native Q/arrow/pick evidence is separate.'}},
                      limits=['Supplied local native fixture; one explicit target, signed Distance Cut, no full tutorial or Through All claim.'])
        print('PASS: five-profile Cut, retained target, earlier sketch edit, history and storage')
    finally:
        (output / 'profile-cut-api.json').write_text(json.dumps(result, indent=2) + '\n')


if __name__ == '__main__':
    main()
