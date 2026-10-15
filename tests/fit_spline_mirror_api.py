#!/usr/bin/env python3
"""Real application retained fit-spline Mirror, edits and persistence."""
import argparse
import json
from pathlib import Path
from app_harness import application


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True, type=Path)
    args = parser.parse_args(); out = args.output.resolve()
    with application(args.app, out) as (call, ledger):
        call('window.resize', {'width': 1500, 'height': 950})
        call('sketch.begin', {'plane': 'XY', 'name': 'Retained spline reflection'})
        call('sketch.lookAt')
        def draft():
            return call('app.state')['activeSession']['draftSketch']
        call('sketch.line', {'x1': -40, 'y1': 0, 'x2': 40, 'y2': 0, 'units': 'mm'})
        axis = draft()['parameters']['entities'][-1]
        call('sketch.construction', {'entityIds': [axis['id']], 'construction': True})
        call('sketch.splineAdd', {'points': [{'uMm': -30, 'vMm': 14}, {'uMm': -12, 'vMm': 28},
                                          {'uMm': 8, 'vMm': 24}, {'uMm': 24, 'vMm': 10}]})
        source = draft()['parameters']['entities'][-1]
        for point, du, dv in [(source['pointIds'][0], 60, 20), (source['pointIds'][-1], 30, -30)]:
            call('sketch.splineTangent', {'entityId': source['id'], 'pointId': point, 'duMm': du, 'dvMm': dv})
        original = draft(); inputs = {'sourceEntityIds': [source['id']], 'axisEntityId': axis['id']}
        call('sketch.mirrorPreview', inputs)
        assert draft() == original
        call('capture.screenshot', {'path': str(out / '01-spline-preview.png')})
        call('sketch.mirrorCancel'); assert draft() == original
        preview = call('sketch.mirrorPreview', inputs)
        call('sketch.mirrorAccept'); accepted = draft()
        group = accepted['parameters']['mirrorGroups'][0]
        assert group == preview['mirrorGroup'] and len(group['pointPairs']) == 4
        result = next(e for e in accepted['parameters']['entities'] if e['id'] == group['entityPairs'][0]['resultEntityId'])
        assert len(result['tangentHandles']) == 2
        call('capture.screenshot', {'path': str(out / '02-spline-accepted.png')})
        call('sketch.finish'); before = call('document.inspect')['authored']
        call('document.save', {'path': str(out / 'spline-mirror.alloy')})
        call('sketch.edit', {'sketchId': accepted['id']})
        call('sketch.movePoint', {'pointId': source['pointIds'][1], 'x': -10, 'y': 32, 'units': 'mm'})
        call('sketch.movePoint', {'pointId': axis['endPointId'], 'x': 38, 'y': 14, 'units': 'mm'})
        changed = draft(); assert changed['parameters']['mirrorGroups'][0] == group
        call('capture.screenshot', {'path': str(out / '03-source-axis-edit.png')})
        call('sketch.finish'); edited = call('document.inspect')['authored']
        call('history.undo'); assert call('document.inspect')['authored'] == before
        call('history.redo'); assert call('document.inspect')['authored'] == edited
        model = out / 'spline-mirror-edited.alloy'; call('document.save', {'path': str(model)})
        call('document.new', {'discard': True}); call('document.open', {'path': str(model)})
        assert call('document.inspect')['authored'] == edited
        state = call('app.state')
        (out / 'report.json').write_text(json.dumps({'passed': True, 'group': group, 'gpu': state['viewport']['gpu'],
            'scope': 'Real-entrypoint shared API preview/Cancel/accept, source and axis edits, undo/redo and authored reopen',
            'calls': len(ledger)}, indent=2))
        (out / 'transcript.json').write_text(json.dumps(ledger, indent=2))
    print('PASS fit-spline Mirror API')

if __name__ == '__main__':
    main()
