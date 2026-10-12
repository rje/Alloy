#!/usr/bin/env python3
"""Pointer-inferred closed sketch, constrained extrusion, history and storage."""
import argparse
import hashlib
import json
import math
from pathlib import Path
from app_harness import application


def near(value, expected):
    assert math.isclose(value, expected, rel_tol=1e-7, abs_tol=1e-5), (value, expected)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--fixture', default=str(Path(__file__).parent / 'workflows/fixtures/sketch-snaps.json'))
    args = parser.parse_args()
    fixture = json.loads(Path(args.fixture).read_text())
    output = Path(args.output).resolve()
    captures = []
    with application(args.app, output) as (call, transcript):
        initial = call('app.state')
        assert initial['viewport']['gpu']['hardwareAccelerated']
        session = call('sketch.begin', {'plane': 'XY', 'name': 'Inferred closed plate'})
        call('sketch.tool', {'tool': 'line'})
        def preview():
            return call('command.preview', {'session': session['id']})
        def parameters():
            return preview()['authoredDraft']['parameters']
        def pixel(u, v):
            p = call('view.projectSketchPoint', {'plane': 'XY', 'uMm': u, 'vMm': v})
            return {'x': p['x'], 'y': p['y']}
        origin = pixel(0, 0)
        inside = {'x': origin['x'] + 3, 'y': origin['y'] + 2}
        original = preview()['authoredDraft']
        hovered = call('sketch.pointerMove', inside)
        assert hovered['snap']['status'] == 'acquired' and hovered['snap']['target']['kind'] == 'origin'
        radius = hovered['snap']['radiusPx']
        assert preview()['authoredDraft'] == original
        cue = call('app.state')['viewport']['gpu']['sketchSnapCue']
        assert cue['visible'] and cue['target']['kind'] == 'origin'
        captures.append(call('capture.screenshot', {'path': str(output / 'origin-cue.png')}))
        outside = {'x': origin['x'] + radius + 4, 'y': origin['y']}
        assert call('sketch.pointerMove', outside)['snap']['status'] == 'none'
        assert not call('app.state')['viewport']['gpu']['sketchSnapCue']['visible']
        assert preview()['authoredDraft'] == original
        call('sketch.placePoint', inside)
        # A point gesture alone is not a committed segment or document feature.
        for u, v in ((30, 0), (30, 20), (0, 20)):
            call('sketch.placePoint', pixel(u, v))
        call('sketch.placePoint', inside)
        call('sketch.tool', {'tool': 'select'})
        p = parameters()
        origin_id = p['originPointId']
        assert len(p['entities']) == 4
        origin_point = next(pt for pt in p['points'] if pt['id'] == origin_id)
        near(origin_point['uMm'], 0); near(origin_point['vMm'], 0)
        assert any(c['type'] == 'fixed' and c['pointIds'] == [origin_id] for c in p['constraints'])
        assert any(c['type'] == 'coincident' and origin_id in c['pointIds'] for c in p['constraints'])
        assert preview()['sketchGeometry']['profiles'][0]['valid']
        edges = p['entities']
        for index, edge in enumerate(edges):
            call('sketch.constraint', {'type': 'horizontal' if index % 2 == 0 else 'vertical', 'entityIds': [edge['id']]})
        call('sketch.dimension', {'kind': 'horizontal', 'entityId': edges[0]['id'], 'value': fixture['widthMm']})
        width_id = parameters()['dimensions'][-1]['id']
        call('sketch.dimension', {'kind': 'vertical', 'entityId': edges[1]['id'], 'value': fixture['heightMm']})
        info = preview()
        near(info['sketchGeometry']['profiles'][0]['areaMm2'], fixture['widthMm'] * fixture['heightMm'])
        assert info['sketchGeometry']['solver']['dof'] == 0
        sketch_id = info['authoredDraft']['id']
        profile_id = info['sketchGeometry']['profiles'][0]['profileId']
        retained = parameters()['constraints']
        captures.append(call('capture.screenshot', {'path': str(output / 'snapped-closed-profile.png')}))
        call('sketch.finish')
        committed = call('document.inspect')['authored']
        call('history.undo'); assert call('document.inspect')['sketches'] == 0
        call('history.redo'); assert call('document.inspect')['authored'] == committed
        extrude = call('command.begin', {'command': 'solid.extrude', 'inputs': {
            'sketchId': sketch_id, 'profileId': profile_id, 'distance': fixture['depthMm'], 'units': 'mm'}})
        call('command.preview', {'session': extrude['id']}); call('command.commit', {'session': extrude['id']})
        before_edit = call('document.inspect')
        near(before_edit['evaluation']['bodies'][0]['volumeMm3'], fixture['widthMm'] * fixture['heightMm'] * fixture['depthMm'])
        session = call('sketch.edit', {'sketchId': sketch_id})
        call('sketch.dimensionUpdate', {'dimensionId': width_id, 'value': fixture['editedWidthMm']})
        assert parameters()['constraints'] == retained and parameters()['originPointId'] == origin_id
        call('sketch.finish')
        edited = call('document.inspect')
        near(edited['evaluation']['bodies'][0]['volumeMm3'], fixture['editedWidthMm'] * fixture['heightMm'] * fixture['depthMm'])
        call('history.undo'); assert call('document.inspect')['authored'] == before_edit['authored']
        call('history.redo'); assert call('document.inspect')['authored'] == edited['authored']
        path = str(output / 'snapped-plate.alloy')
        call('document.save', {'path': path}); call('document.new', {'discard': False}); call('document.open', {'path': path})
        assert call('document.inspect')['authored'] == edited['authored']
        call('view.home')
        captures.append(call('capture.screenshot', {'path': str(output / 'snapped-plate-reopened.png')}))
    (output / 'sketch-snaps-api.json').write_text(json.dumps({
        'result': 'passed', 'fixtureSha256': hashlib.sha256(Path(args.fixture).read_bytes()).hexdigest(),
        'verifiedCapabilities': ['sketch.pointer_snap_preview', 'sketch.retained_origin_coincidence', 'sketch.snapped_profile_solid_history'],
        'initialState': initial, 'transcript': transcript, 'captures': captures,
        'evidenceDimensions': {key: {'status': 'passed'} for key in ('api', 'hardware', 'geometry', 'save_reopen')},
        'limits': ['Pointer commands exercise shared native path; physical input and source visual comparison require independent evidence.']}, indent=2) + '\n')
    print('PASS: pointer-inferred closed profile, extrusion, upstream edit and storage')


if __name__ == '__main__':
    main()
