#!/usr/bin/env python3
"""Focused analytic circle/arc real-app workflow; native input remains separate."""
import argparse
import hashlib
import json
import math
from pathlib import Path
from app_harness import application


def near(value, expected):
    assert math.isclose(value, expected, rel_tol=1e-8, abs_tol=1e-6), (value, expected)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('app', 'output', 'fixture'):
        parser.add_argument('--' + name, required=True)
    args = parser.parse_args()
    output = Path(args.output).resolve()
    fixture = json.loads(Path(args.fixture).read_text())
    captures = []
    with application(args.app, output) as (call, transcript):
        initial = call('app.state')
        assert initial['viewport']['gpu']['hardwareAccelerated']
        circle_session = call('sketch.begin', {'plane': 'XY', 'name': 'Analytic disk'})

        def preview(session):
            return call('command.preview', {'session': session['id']})

        def pixels(u, v):
            point = call('view.projectSketchPoint', {'plane': 'XY', 'uMm': u, 'vMm': v})
            return {'x': point['x'], 'y': point['y']}

        call('sketch.addCircle', {'centerX': 0, 'centerY': 0, 'diameter': fixture['diameterMm'], 'units': 'mm'})
        circle = preview(circle_session)
        intent = circle['authoredDraft']
        circle_id = intent['parameters']['entities'][0]['id']
        sketch_id = intent['id']
        near(circle['sketchGeometry']['entities'][0]['lengthMm'], fixture['diameterMm'] * math.pi)
        assert circle['sketchGeometry']['solver']['dof'] == 3
        profile_id = circle['sketchGeometry']['profiles'][0]['profileId']
        near(circle['sketchGeometry']['profiles'][0]['areaMm2'], 25 * math.pi)
        # D's researched auto mode is diameter for circles; placement stays disposable.
        call('sketch.selectionSet', {'entityIds': [circle_id]})
        pending = call('sketch.dimensionPreview', {**pixels(9, 8), 'kind': 'auto'})
        assert pending['kind'] == 'diameter'
        assert preview(circle_session)['authoredDraft'] == intent
        call('sketch.dimensionPlace', pixels(9, 8))
        call('sketch.dimensionAccept', {'value': fixture['diameterMm'], 'units': 'mm'})
        diameter_id = preview(circle_session)['authoredDraft']['parameters']['dimensions'][-1]['id']
        call('sketch.dimension', {'kind': 'radius', 'entityId': circle_id, 'driving': False,
                                 'annotation': {'uMm': -10, 'vMm': 9}})
        circle = preview(circle_session)
        assert circle['sketchGeometry']['solver']['dof'] == 2
        annotations = call('app.state')['viewport']['sketchAnnotations']
        assert any(a['label']['text'].startswith('Ø') for a in annotations)
        assert any(a['label']['text'].startswith('(R') for a in annotations)
        captures.append(call('capture.screenshot', {'path': str(output / 'circle-dimensions.png')}))
        call('sketch.finish')
        source = call('document.inspect')
        call('solid.extrude', {'sketchId': sketch_id, 'profileId': profile_id,
                               'distance': fixture['distanceMm'], 'units': 'mm'})
        solid = call('document.inspect')
        body_id = solid['authored']['features'][-1]['bodyId']
        near(solid['evaluation']['bodies'][0]['volumeMm3'], 125 * math.pi)
        call('sketch.edit', {'sketchId': sketch_id})
        call('sketch.dimensionUpdate', {'dimensionId': diameter_id, 'value': fixture['editedDiameterMm'], 'units': 'mm'})
        call('sketch.finish')
        changed = call('document.inspect')
        near(changed['evaluation']['bodies'][0]['volumeMm3'], 180 * math.pi)
        assert changed['authored']['features'][-1]['bodyId'] == body_id
        assert changed['evaluation']['sketches'][0]['profiles'][0]['profileId'] == profile_id
        call('history.undo')
        near(call('document.inspect')['evaluation']['bodies'][0]['volumeMm3'], 125 * math.pi)
        call('history.redo')
        assert call('document.inspect')['authored'] == changed['authored']
        call('view.home')
        captures.append(call('capture.screenshot', {'path': str(output / 'edited-disk-solid.png')}))

        arc_session = call('sketch.begin', {'plane': 'XY', 'name': 'Analytic arcs'})
        call('sketch.addArc', {'mode': 'center', 'centerX': 15, 'centerY': 0,
                              'radius': 5, 'startAngleDeg': 0, 'sweepAngleDeg': 90, 'units': 'mm'})
        arc = preview(arc_session)
        arc_id = arc['authoredDraft']['parameters']['entities'][0]['id']
        near(arc['sketchGeometry']['entities'][0]['lengthMm'], 5 * math.pi / 2)
        assert not arc['sketchGeometry']['profiles']
        assert arc['sketchGeometry']['solver']['dof'] == 5
        call('sketch.selectionSet', {'entityIds': [arc_id]})
        pending = call('sketch.dimensionPreview', {**pixels(24, 8), 'kind': 'auto'})
        assert pending['kind'] == 'radius'
        call('sketch.dimensionPlace', pixels(24, 8))
        call('sketch.dimensionAccept', {'value': 5, 'units': 'mm'})
        radius_id = preview(arc_session)['authoredDraft']['parameters']['dimensions'][-1]['id']
        call('sketch.dimension', {'kind': 'arcSweep', 'entityId': arc_id, 'valueDeg': 90,
                                 'annotation': {'uMm': 20, 'vMm': 10}})
        sweep_id = preview(arc_session)['authoredDraft']['parameters']['dimensions'][-1]['id']
        call('sketch.dimension', {'kind': 'diameter', 'entityId': arc_id, 'driving': False,
                                 'annotation': {'uMm': 25, 'vMm': -4}})
        near(preview(arc_session)['sketchGeometry']['solver']['dof'], 3)
        call('sketch.dimensionEdit', {'dimensionId': sweep_id})
        call('sketch.dimensionAccept', {'text': '120 deg'})
        near(preview(arc_session)['sketchGeometry']['entities'][0]['sweepAngleDeg'], 120)
        call('sketch.dimensionUpdate', {'dimensionId': radius_id, 'value': 6})
        near(preview(arc_session)['sketchGeometry']['entities'][0]['lengthMm'], 4 * math.pi)
        usable = preview(arc_session)['authoredDraft']
        call('sketch.dimension', {'kind': 'radius', 'entityId': arc_id, 'value': 7}, 'sketch_solve_failed')
        assert preview(arc_session)['authoredDraft'] == usable
        call('sketch.annotationMove', {'dimensionId': sweep_id, 'uMm': 21, 'vMm': 11})
        placed = preview(arc_session)['authoredDraft']
        assert placed['parameters']['points'] == usable['parameters']['points']
        assert placed['parameters']['entities'] == usable['parameters']['entities']
        # Named first/end/on-curve inputs choose the documented minor and major branches.
        call('sketch.addArc', {'mode': 'threePoint', 'startX': -10, 'startY': 0, 'endX': -15, 'endY': 5,
                              'throughX': -15 + 5 / math.sqrt(2), 'throughY': 5 / math.sqrt(2)})
        near(preview(arc_session)['sketchGeometry']['entities'][-1]['sweepAngleDeg'], 90)
        call('sketch.addArc', {'mode': 'threePoint', 'startX': -10, 'startY': -15, 'endX': -15, 'endY': -10,
                              'throughX': -20, 'throughY': -15})
        near(preview(arc_session)['sketchGeometry']['entities'][-1]['sweepAngleDeg'], -270)
        call('view.zoom', {'factor': 1.5})
        captures.append(call('capture.screenshot', {'path': str(output / 'arc-dimensions.png')}))
        call('sketch.finish')
        completed = call('document.inspect')
        path = output / 'analytic-curves.alloy'
        call('document.save', {'path': str(path)})
        call('document.new', {'discard': False})
        call('document.open', {'path': str(path)})
        reopened = call('document.inspect')
        assert reopened['authored'] == completed['authored']
        near(reopened['evaluation']['bodies'][0]['volumeMm3'], 180 * math.pi)
        final = call('app.state')
    result = {'result': 'passed', 'fixtureSha256': hashlib.sha256(Path(args.fixture).read_bytes()).hexdigest(),
              'verifiedCapabilities': ['sketch.analytic_curves', 'sketch.curve_dimensions', 'sketch.curve_storage',
                                       'sketch.circle_profile_extrusion', 'sketch.curve_upstream_edit'],
              'initialState': initial, 'finalState': final, 'transcript': transcript, 'captures': captures,
              'evidenceDimensions': {'api': {'status': 'passed'}, 'hardware': {'status': 'passed'},
                                     'geometry': {'status': 'passed'}, 'save_reopen': {'status': 'passed'},
                                     'visual': {'status': 'captured', 'reason': 'Native GPU curves/annotations; physical interaction and visual acceptance require separate review.'}}}
    (output / 'sketch-curves-api.json').write_text(json.dumps(result, indent=2) + '\n')
    print('PASS: analytic curves, diameter/radius/sweep dimensions, disk extrusion/edit, history and storage')


if __name__ == '__main__':
    main()
