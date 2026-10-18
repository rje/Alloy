#!/usr/bin/env python3
"""Two-body Browser identity, visibility, selection and reopen workflow."""

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
        call('solid.box', {'width': 12, 'depth': 10, 'height': 8, 'x': -20, 'units': 'mm'})
        call('solid.box', {'width': 6, 'depth': 7, 'height': 9, 'x': 20, 'units': 'mm'})
        initial = call('document.inspect')
        features = initial['authored']['features']
        left_id, right_id = [feature['bodyId'] for feature in features]
        left_feature = features[0]['id']
        initial_geometry = {
            body['bodyId']: (body['volumeMm3'], body['boundsMm'])
            for body in initial['evaluation']['bodies']
        }
        assert set(initial_geometry) == {left_id, right_id}

        call('body.rename', {'bodyId': left_id, 'name': 'Left bracket stock'})
        named = call('document.inspect')
        assert named['authored']['bodyNames'][left_id] == 'Left bracket stock'
        assert [feature['id'] for feature in named['authored']['features']] == [feature['id'] for feature in features]
        assert {
            body['bodyId']: (body['volumeMm3'], body['boundsMm'])
            for body in named['evaluation']['bodies']
        } == initial_geometry

        call('selection.set', {'bodyId': right_id})
        assert call('app.state')['selection']['bodyId'] == right_id
        call('feature.visibility', {'featureId': left_feature, 'visible': False})
        hidden = call('document.inspect')
        assert hidden['authored']['features'][0]['visible'] is False
        assert hidden['authored']['bodyNames'][left_id] == 'Left bracket stock'
        assert next(body for body in hidden['evaluation']['bodies'] if body['bodyId'] == left_id)['visible'] is False
        assert call('app.state')['selection']['bodyId'] == right_id
        call('selection.set', {'bodyId': left_id}, error='stale_entity')
        assert call('app.state')['selection']['bodyId'] == right_id
        call('capture.screenshot', {'path': str(output / 'hidden-left.png')})

        call('history.undo')
        assert call('document.inspect')['authored'] == named['authored']
        call('history.redo')
        assert call('document.inspect')['authored'] == hidden['authored']
        path = output / 'browser-two-body.alloy'
        call('document.save', {'path': str(path)})
        call('document.new', {'discard': True})
        call('document.open', {'path': str(path)})
        reopened = call('document.inspect')
        assert reopened['authored'] == hidden['authored']
        assert reopened['path'] == str(path)
        assert not reopened['dirty']
        assert {body['bodyId'] for body in reopened['evaluation']['bodies']} == {left_id, right_id}
        call('capture.screenshot', {'path': str(output / 'reopened.png')})

        (output / 'report.json').write_text(json.dumps({
            'result': 'passed', 'model': str(path), 'bodyIds': [left_id, right_id],
            'transcript': transcript,
        }, indent=2) + '\n')


if __name__ == '__main__':
    main()
