#!/usr/bin/env python3
"""Bounded real-entrypoint Timeline inspection/captures; no tutorial replay."""
import argparse
import json
from pathlib import Path
from app_harness import application

p = argparse.ArgumentParser()
p.add_argument('--app', required=True)
p.add_argument('--fixture', required=True)
p.add_argument('--output', required=True)
a = p.parse_args()
out = Path(a.output).resolve()
with application(a.app, out) as (call, transcript):
    call('document.open', {'path': str(Path(a.fixture).resolve())})
    source = call('document.inspect')
    call('component.activate', {'definitionId': source['authored']['components'][0]['id']})
    call('view.home')
    evidence = []
    for width in [1400, 1040, 1800]:
        call('window.resize', {'width': width, 'height': 900})
        call('timeline.showLatest')
        state = call('app.state')
        timeline = state['ui']['timeline']
        assert len(timeline['features']) == 24
        assert {f['width'] for f in timeline['features']} == {44}
        assert all(f['name'] in f['tooltip'] and f['name'] in f['accessibleName'] and f['type'] for f in timeline['features'])
        assert timeline['features'][-1]['fullyVisible']
        if width == 1040:
            assert timeline['scrollMaximum'] > 0
        call('capture.screenshot', {'path': str(out / f'timeline-{width}-latest.png')})
        evidence.append({'width': width, 'state': state})
    call('window.resize', {'width': 1040, 'height': 900})
    features = source['authored']['features']
    targets = []
    for index in [0, 12, 23]:
        feature = features[index]
        call('timeline.showFeature', {'featureId': feature['id']})
        state = call('app.state')
        assert next(f for f in state['ui']['timeline']['features'] if f['featureId'] == feature['id'])['fullyVisible']
        edit = call('feature.editDialog', {'featureId': feature['id']})
        edited = edit.get('draftSketch', {}).get('id', edit['inputs'].get('featureId'))
        assert edited == feature['id']
        call('command.cancel', {'session': edit['id']})
        assert call('document.inspect')['authored'] == source['authored']
        targets.append({'featureId': feature['id'], 'name': feature['name'], 'state': state})
    call('timeline.showLatest')
    call('window.resize', {'width': 1400, 'height': 900})
    call('capture.screenshot', {'path': str(out / 'timeline-1400-final.png')})
    (out / 'timeline-report.json').write_text(json.dumps({'passed': True, 'source': source, 'widths': evidence, 'targets': targets}, indent=2))
    (out / 'transcript.json').write_text(json.dumps(transcript, indent=2))
