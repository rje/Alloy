#!/usr/bin/env python3
"""Bounded construction conversion checkpoint, not complete SKT-016 acceptance."""
import argparse
import hashlib
import json
import math
from pathlib import Path
from app_harness import application


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--fixture', default=str(Path(__file__).parent / 'workflows/fixtures/sketch-construction.json'))
    args = parser.parse_args()
    fixture = json.loads(Path(args.fixture).read_text())
    output = Path(args.output).resolve()
    width, height, edited_width = fixture['widthMm'], fixture['heightMm'], fixture['editedWidthMm']
    captures = []
    with application(args.app, output) as (call, transcript):
        initial = call('app.state')
        baseline = call('document.inspect')
        session = call('sketch.begin', {'plane': 'XY', 'name': 'Retained construction reference'})

        def preview():
            return call('command.preview', {'session': session['id']})

        def draft():
            return preview()['authoredDraft']

        def convert(ids, construction):
            call('sketch.selectionSet', {'entityIds': ids})
            call('sketch.construction', {'construction': construction})

        def area(expected):
            profiles = preview()['sketchGeometry']['profiles']
            assert len(profiles) == 1
            assert math.isclose(profiles[0]['areaMm2'], expected, abs_tol=1e-6)
            return profiles[0]

        call('sketch.rectangle', {'x1': 0, 'y1': 0, 'x2': width, 'y2': height, 'units': 'mm'})
        boundary = draft()['parameters']['entities']
        edge_id = boundary[0]['id']
        call('sketch.dimension', {'kind': 'horizontal', 'entityId': edge_id, 'value': width, 'units': 'mm'})
        width_id = draft()['parameters']['dimensions'][-1]['id']
        call('sketch.dimension', {'kind': 'vertical', 'entityId': boundary[1]['id'], 'value': height, 'units': 'mm'})
        call('sketch.line', {'x1': 0, 'y1': 0, 'x2': width, 'y2': height, 'units': 'mm'})
        diagonal_id = draft()['parameters']['entities'][-1]['id']
        convert([diagonal_id], True)
        normal = draft()
        profile = area(width * height)
        assert diagonal_id not in profile['sourceEntityIds']
        convert([edge_id], True)
        converted = draft()
        assert converted['parameters']['constraints'] == normal['parameters']['constraints']
        assert converted['parameters']['dimensions'] == normal['parameters']['dimensions']
        assert [e['id'] for e in converted['parameters']['entities']] == [e['id'] for e in normal['parameters']['entities']]
        assert not preview()['sketchGeometry']['profiles']
        call('sketch.tool', {'tool': 'select'})
        call('history.undo')
        assert draft() == normal
        area(width * height)
        call('history.redo')
        assert draft() == converted and not preview()['sketchGeometry']['profiles']
        call('sketch.dimensionUpdate', {'dimensionId': width_id, 'value': edited_width, 'units': 'mm'})
        convert([edge_id], False)
        restored = draft()
        restored_profile = area(edited_width * height)
        assert restored_profile['profileId'] == profile['profileId']
        assert restored['parameters']['entities'][0].get('construction', False) is False
        captures.append(call('capture.screenshot', {'path': str(output / 'construction-draft.png')}))
        gpu = call('app.state')['viewport']['gpu']
        assert gpu['constructionDraftVisible']
        assert gpu['constructionDraftLineIds'] == [diagonal_id], gpu
        finished_draft = draft()
        call('sketch.finish')
        committed = call('document.inspect')
        assert committed['undo']['count'] == baseline['undo']['count'] + 1
        call('history.undo')
        assert call('document.inspect')['authored'] == baseline['authored']
        call('history.redo')
        assert call('document.inspect')['authored'] == committed['authored']
        path = output / 'construction.alloy'
        call('document.save', {'path': str(path)})
        call('document.new', {'discard': False})
        call('document.open', {'path': str(path)})
        reopened = call('document.inspect')
        assert reopened['authored'] == committed['authored']
        saved = next(f for f in reopened['authored']['features'] if f['id'] == finished_draft['id'])
        assert saved == finished_draft
        assert math.isclose(reopened['evaluation']['sketches'][0]['profiles'][0]['areaMm2'], edited_width * height, abs_tol=1e-6)
        final = call('app.state')
    result = {
        'result': 'passed', 'testSha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'fixtureSha256': hashlib.sha256(Path(args.fixture).read_bytes()).hexdigest(),
        'verifiedCapabilities': ['sketch.construction_conversion'], 'covers': [],
        'initialState': initial, 'finalState': final, 'transcript': transcript, 'captures': captures,
        'evidenceDimensions': {'api': {'status': 'passed'}, 'geometry': {'status': 'passed'},
                               'save_reopen': {'status': 'passed'},
                               'visual': {'status': 'captured', 'reason': 'Construction GPU capture; independent styling review remains separate.'},
                               'physical_input': {'status': 'not_run', 'reason': 'API selection does not establish native Shift-pick/context-menu behavior.'}}
    }
    (output / 'sketch-construction-api.json').write_text(json.dumps(result, indent=2) + '\n')
    print('PASS: retained construction conversion, profile exclusion, driving edit, history and storage')


if __name__ == '__main__':
    main()
