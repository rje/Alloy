#!/usr/bin/env python3
"""One real-app New, Save As, Open, and dirty-close document workflow."""

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
    first, second = output / 'first.alloy', output / 'second.alloy'
    bad = output / 'missing-directory' / 'unwritable.alloy'

    with application(args.app, output) as (call, transcript):
        call('solid.box', {'width': 18, 'height': 12, 'depth': 5, 'units': 'mm'})
        original = call('document.inspect')
        tab = call('document.tabs')['activeTabId']
        call('document.new', error='unsaved_changes')
        call('document.tabClose', {'tabId': tab}, error='unsaved_changes')
        assert call('document.inspect') == original
        call('document.save', {'path': str(first)})
        assert first.is_file()
        assert call('document.inspect')['path'] == str(first)
        assert not call('document.tabs')['tabs'][0]['dirty']

        call('solid.cylinder', {'radius': 3, 'height': 7, 'units': 'mm'})
        changed = call('document.inspect')
        assert changed['path'] == str(first)
        call('document.save', {'path': str(bad)}, error='save_failed')
        assert call('document.inspect') == changed
        assert not bad.exists()
        call('document.save', {'path': str(second)})
        assert second.is_file() and first.read_bytes() != second.read_bytes()
        saved = call('document.inspect')
        assert saved['path'] == str(second)
        assert not call('document.tabs')['tabs'][0]['dirty']

        call('solid.box', {'width': 2, 'height': 3, 'depth': 4, 'units': 'mm'})
        dirty = call('document.inspect')
        call('document.open', {'path': str(first)}, error='unsaved_changes')
        call('document.new', error='unsaved_changes')
        assert call('document.inspect') == dirty
        call('document.open', {'path': str(first), 'discard': True})
        reopened = call('document.inspect')
        assert reopened['authored'] == original['authored']
        assert reopened['path'] == str(first)
        assert not call('document.tabs')['tabs'][0]['dirty']
        call('document.open', {'path': str(second)})
        assert call('document.inspect')['authored'] == saved['authored']

        call('solid.box', {'width': 1, 'height': 1, 'depth': 1, 'units': 'mm'})
        before_bad_open = call('document.inspect')
        malformed = output / 'malformed.alloy'
        malformed.write_text('{not json')
        call('document.open', {'path': str(malformed), 'discard': True}, error='open_failed')
        assert call('document.inspect') == before_bad_open
        call('document.new', {'discard': True})
        assert call('document.inspect')['features'] == 0

        report = {
            'result': 'passed', 'firstPath': str(first), 'secondPath': str(second),
            'transcript': transcript,
        }
        (output / 'report.json').write_text(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
