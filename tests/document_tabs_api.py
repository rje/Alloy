#!/usr/bin/env python3
"""Bounded real-app independent-document and retained sketch-session checkpoint."""
import argparse
import hashlib
import json
from pathlib import Path
from app_harness import application


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--fixture', default=str(Path(__file__).parent / 'workflows/fixtures/document-tabs.json'))
    args = parser.parse_args()
    output = Path(args.output).resolve()
    fixture = json.loads(Path(args.fixture).read_text())
    captures = []
    with application(args.app, output) as (call, transcript):
        initial = call('app.state')
        tab_a = call('document.tabs')['activeTabId']
        call('solid.box', fixture['boxA'])
        doc_a = call('document.inspect')
        body_a = doc_a['authored']['features'][0]['bodyId']
        call('selection.set', {'bodyId': body_a})
        call('view.zoom', {'factor': 0.7})
        call('view.pan', {'x': 27, 'y': -13})
        state_a = call('app.state')
        tab_b = call('document.tabNew')['activeTabId']
        assert tab_a != tab_b
        call('solid.box', fixture['boxB'])
        doc_b = call('document.inspect')
        assert doc_a['revision'] == doc_b['revision'], 'Exercise equal-revision cache invalidation'
        assert doc_a['authored']['documentId'] != doc_b['authored']['documentId']
        body_b = doc_b['authored']['features'][0]['bodyId']
        call('selection.set', {'bodyId': body_b})
        call('view.zoom', {'factor': 1.3})
        state_b = call('app.state')
        assert state_b['viewport']['gpu']['camera'] != state_a['viewport']['gpu']['camera']
        tabs = call('document.tabs')
        assert tabs['count'] == 2 and all(tab['dirty'] for tab in tabs['tabs'])

        def activate(tab, expected, state):
            assert call('document.tabActivate', {'tabId': tab})['activeTabId'] == tab
            assert call('document.inspect') == expected
            restored = call('app.state')
            assert restored['selection'] == state['selection']
            assert restored['viewport']['gpu']['camera'] == state['viewport']['gpu']['camera']
            assert restored['viewport']['gpu']['displayedBodies'] == 1
            expected_body = expected['authored']['features'][0]['bodyId']
            assert restored['viewport']['gpu']['displayedBodyIds'] == [expected_body], restored['viewport']['gpu']
            assert restored['document']['evaluation']['bodies'][0]['boundsMm'] == expected['evaluation']['bodies'][0]['boundsMm']

        activate(tab_a, doc_a, state_a)
        call('history.undo')
        assert call('document.inspect')['features'] == 0
        activate(tab_b, doc_b, state_b)
        call('document.tabActivate', {'tabId': tab_a})
        call('history.redo')
        assert call('document.inspect')['authored'] == doc_a['authored']
        session = call('sketch.begin', {'plane': 'XY', 'name': 'Tab-local draft'})
        call('sketch.rectangle', {'x1': 0, 'y1': 0, 'x2': fixture['draftWidthMm'],
                                  'y2': fixture['draftHeightMm'], 'units': 'mm'})
        preview = call('command.preview', {'session': session['id']})
        line = preview['authoredDraft']['parameters']['entities'][0]['id']
        call('sketch.dimension', {'kind': 'horizontal', 'entityId': line,
                                  'value': fixture['draftWidthMm'], 'units': 'mm'})
        accepted = call('app.state')['activeSession']
        call('document.save', {'path': str(output / 'unfinished.alloy')}, error='active_sketch_draft')
        dimension = accepted['draftSketch']['parameters']['dimensions'][-1]['id']
        call('sketch.tool', {'tool': 'dimension'})
        call('sketch.dimensionEdit', {'dimensionId': dimension})
        pending = call('app.state')['activeSession']
        call('document.tabActivate', {'tabId': tab_b}, error='pending_input_active')
        assert call('app.state')['activeSession'] == pending
        assert call('document.tabs')['activeTabId'] == tab_a
        call('sketch.tool', {'tool': 'line'})
        pixel = call('view.projectSketchPoint', {'plane': 'XY', 'uMm': 30, 'vMm': 30})
        call('sketch.placePoint', {'x': pixel['x'], 'y': pixel['y']})
        pending = call('app.state')['activeSession']
        call('document.tabActivate', {'tabId': tab_b}, error='pending_input_active')
        assert call('app.state')['activeSession'] == pending
        call('sketch.tool', {'tool': 'select'})
        accepted = call('app.state')['activeSession']
        call('document.tabActivate', {'tabId': tab_b})
        assert not call('app.state')['activeSession']
        assert call('document.inspect') == doc_b
        call('document.tabActivate', {'tabId': tab_a})
        restored = call('app.state')['activeSession']
        assert restored['id'] == accepted['id']
        assert restored['draftSketch'] == accepted['draftSketch']
        assert restored['draftHistory'] == accepted['draftHistory']
        call('history.undo')
        assert not call('app.state')['activeSession']['draftSketch']['parameters'].get('dimensions', [])
        call('history.redo')
        assert call('app.state')['activeSession']['draftSketch'] == accepted['draftSketch']
        captures.append(call('capture.screenshot', {'path': str(output / 'restored-tab-draft.png')}))
        call('document.tabClose', {'tabId': tab_a}, error='unsaved_changes')
        assert call('app.state')['activeSession']['draftSketch'] == accepted['draftSketch']
        call('sketch.finish')
        finished_a = call('document.inspect')['authored']
        path_a, path_b = output / 'tab-a.alloy', output / 'tab-b.alloy'
        call('document.save', {'path': str(path_a)})
        call('app.quit', error='unsaved_changes')  # inactive B is still dirty
        assert call('document.tabs')['count'] == 2
        call('document.tabActivate', {'tabId': tab_b})
        call('document.save', {'path': str(path_b)})
        clean_b = call('document.inspect')
        pending_solid = call('command.begin', {'command': 'solid.box', 'inputs': fixture['boxA']})
        call('document.tabClose', {'tabId': tab_b, 'discard': True}, error='pending_input_active')
        assert call('document.tabs')['activeTabId'] == tab_b
        assert call('app.state')['activeSession']['id'] == pending_solid['id']
        assert call('document.inspect') == clean_b
        call('command.cancel', {'session': pending_solid['id']})
        call('solid.box', fixture['boxA'])
        call('document.tabClose', {'tabId': tab_b}, error='unsaved_changes')
        assert call('document.inspect')['features'] == 2
        call('document.tabClose', {'tabId': tab_b, 'discard': True})
        assert call('document.tabs')['activeTabId'] == tab_a
        assert call('document.inspect')['authored'] == finished_a
        call('document.open', {'path': str(path_a)})
        assert call('document.inspect')['authored'] == finished_a
        call('document.tabNew')
        call('document.open', {'path': str(path_b)})
        assert call('document.inspect')['authored'] == doc_b['authored']
        final = call('app.state')
    result = {
        'result': 'passed', 'testSha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'fixtureSha256': hashlib.sha256(Path(args.fixture).read_bytes()).hexdigest(),
        'verifiedCapabilities': ['document.independent_tabs', 'document.retained_tab_sketch_session', 'document.all_tab_dirty_guard'],
        'covers': [], 'initialState': initial, 'finalState': final,
        'transcript': transcript, 'captures': captures,
        'evidenceDimensions': {
            'api': {'status': 'passed'}, 'geometry': {'status': 'passed'}, 'save_reopen': {'status': 'passed'},
            'visual': {'status': 'captured', 'reason': 'Retained draft GPU capture; independent tab appearance review remains separate.'},
            'physical_input': {'status': 'not_run', 'reason': 'API workflow does not establish native tab clicks or close dialogs.'}}
    }
    (output / 'document-tabs-api.json').write_text(json.dumps(result, indent=2) + '\n')
    print('PASS: independent tabs, retained draft history, pending guards, dirty close/quit and storage')


if __name__ == '__main__':
    main()
