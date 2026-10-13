#!/usr/bin/env python3
"""Real Alloy command contract for retained six-curve sketch reflection."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    output = Path(args.output).resolve()
    with application(args.app, output) as (call, transcript):
        call('sketch.begin', {'plane': 'XY', 'name': 'Rough cutout mirror'})
        draft = lambda: call('app.state')['activeSession']['draftSketch']
        ids = []
        for x1, y1, x2, y2 in [(-14, 10, 12, 10), (12, 10, 12, 36),
                               (12, 36, -14, 36), (-14, 36, -14, 10),
                               (-25, -3, 25, -3)]:
            call('sketch.line', {'x1': x1, 'y1': y1, 'x2': x2, 'y2': y2, 'units': 'mm'})
            ids.append(draft()['parameters']['entities'][-1]['id'])
        call('sketch.construction', {'entityIds': [ids[1], ids[3], ids[4]], 'construction': True})
        for start, end, through in [((-14, 36), (-14, 10), (-14 - math.sqrt(56), 23)),
                                    ((12, 10), (12, 36), (12 + math.sqrt(56), 23))]:
            call('sketch.addArc', {'mode': 'threePoint', 'startX': start[0], 'startY': start[1],
                                  'endX': end[0], 'endY': end[1],
                                  'throughX': through[0], 'throughY': through[1], 'units': 'mm'})
            ids.append(draft()['parameters']['entities'][-1]['id'])
        original = draft()
        assert len(original['parameters']['entities']) == 7
        source = ids[:4] + ids[5:]
        input_ = {'sourceEntityIds': source, 'axisEntityId': ids[4]}
        call('sketch.mirrorPreview', {'sourceEntityIds': source, 'axisEntityId': '00000000-0000-4000-8000-000000000000'},
             error='sketch_mirror_failed')
        assert draft() == original
        preview = call('sketch.mirrorPreview', input_)
        assert preview['pending'] and not preview['authoredChange']
        group = preview['mirrorGroup']
        assert len(group['entityPairs']) == 6 and len(group['pointPairs']) == 6
        assert draft() == original
        call('view.zoom', {'factor': 0.7})
        call('capture.screenshot', {'path': str(output / 'mirror-preview.png')})
        call('sketch.mirrorCancel')
        assert draft() == original and 'pendingSketchMirror' not in call('app.state')['activeSession']
        call('sketch.mirrorAccept', error='mirror_preview_required')
        preview = call('sketch.mirrorPreview', input_)
        accepted = call('sketch.mirrorAccept')
        authored_group = accepted['draftSketch']['parameters']['mirrorGroups'][0]
        assert authored_group['id'] == preview['mirrorGroup']['id']
        assert len(authored_group['entityPairs']) == 6 and len(authored_group['pointPairs']) == 6
        assert accepted['diagnostics']['dof'] > 0
        call('capture.screenshot', {'path': str(output / 'mirror-accepted.png')})
        call('sketch.finish')
        authored = call('document.inspect')['authored']
        assert len(authored['features'][0]['parameters']['mirrorGroups']) == 1
        (output / 'report.json').write_text(json.dumps({
            'scope': 'Real Alloy API controls and viewport captures; native UI selection belongs to SKT-054.',
            'sourceEntityCount': len(source), 'resultEntityCount': len(authored_group['entityPairs']),
            'deduplicatedPointCount': len(authored_group['pointPairs']),
            'dof': accepted['diagnostics']['dof'], 'groupId': authored_group['id'],
            'callCount': len(transcript)}, indent=2) + '\n')
    print('PASS: six-curve sketch Mirror preview, cancel and accept through real Alloy API')


if __name__ == '__main__':
    main()
