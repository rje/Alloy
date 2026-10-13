#!/usr/bin/env python3
"""Prepared #257 blank-to-three-sketch source runner. No runtime pass is implied.

The movie records Alloy shared commands with explicit API captions. Separate
native ingredient tests establish Qt input; this runner is not physical input.
The public step contract and reference provenance live outside the repository.
"""
import argparse
from contextlib import ExitStack
import hashlib
import json
import math
from pathlib import Path
import time

from app_harness import application


SOURCE_ID = 'sketch-slots-splines-20261013'
STRAIGHT_AREA = 7500 + 625 * math.pi
ARC_AREA = 2475 * math.pi
ELLIPSE_AREA = 6875 * math.pi


def near(actual, expected, tolerance=1e-4):
    assert math.isclose(actual, expected, abs_tol=tolerance), (actual, expected)


def by_id(items):
    return {item['id']: item for item in items}


def coincident_group(parameters, seed):
    equivalent = {seed}
    while True:
        expanded = set(equivalent)
        for relation in parameters['constraints']:
            ids = set(relation.get('pointIds', []))
            if relation['type'] == 'coincident' and ids & equivalent:
                expanded.update(ids)
        if expanded == equivalent:
            return equivalent
        equivalent = expanded


def verify_slots(sketch, radius=150):
    parameters = sketch['parameters']
    assert len(parameters['slotGroups']) == len(parameters['arcSlotGroups']) == 1
    straight, group = parameters['slotGroups'][0], parameters['arcSlotGroups'][0]
    entities, points = by_id(parameters['entities']), by_id(parameters['points'])
    guide, path = entities[straight['centerlineId']], entities[group['pathArcId']]
    near(math.dist((points[guide['startPointId']]['uMm'], points[guide['startPointId']]['vMm']),
                   (points[guide['endPointId']]['uMm'], points[guide['endPointId']]['vMm'])), 150)
    near(points[guide['startPointId']]['vMm'], 0, 1e-5)
    near(points[guide['endPointId']]['vMm'], 0, 1e-5)
    assert straight['centerPointId'] in coincident_group(parameters, parameters['originPointId'])
    assert path['centerPointId'] in coincident_group(parameters, parameters['originPointId'])
    for arc_id in straight['arcIds']:
        near(entities[arc_id]['radiusMm'], 25)
    near(path['radiusMm'], radius)
    near(path['sweepAngleDeg'], -90, 1e-5)
    for key, sign in (('startPointId', -1), ('endPointId', 1)):
        point = points[path[key]]
        near(point['uMm'], sign * radius / math.sqrt(2), 1e-4)
        near(point['vMm'], radius / math.sqrt(2), 1e-4)
    near(entities[group['outerArcId']]['radiusMm'], radius + 15)
    near(entities[group['innerArcId']]['radiusMm'], radius - 15)
    for cap in group['capIds']:
        near(entities[cap]['radiusMm'], 15)
    return straight, group


def verify_ellipse(sketch, evaluated):
    parameters = sketch['parameters']
    ellipse = next(e for e in parameters['entities'] if e['type'] == 'ellipse')
    entities, points = by_id(parameters['entities']), by_id(parameters['points'])
    assert ellipse['centerPointId'] in coincident_group(parameters, parameters['originPointId'])
    center = points[ellipse['centerPointId']]
    near(center['uMm'], 0, 1e-5)
    near(center['vMm'], 0, 1e-5)
    for axis_id, length, horizontal in ((ellipse['firstAxisId'], 275, True),
                                        (ellipse['secondAxisId'], 100, False)):
        axis = entities[axis_id]
        assert axis['construction']
        a, b = points[axis['startPointId']], points[axis['endPointId']]
        near(math.hypot(b['uMm'] - a['uMm'], b['vMm'] - a['vMm']), length)
        near(b['vMm'] if horizontal else b['uMm'], 0, 1e-5)
    detail = next(e for e in evaluated['entities'] if e['id'] == ellipse['id'])
    near(detail['firstRadiusMm'], 137.5)
    near(detail['secondRadiusMm'], 50)
    profile = next(p for p in evaluated['profiles'] if ellipse['id'] in p['sourceEntityIds'])
    near(profile['areaMm2'], ELLIPSE_AREA)
    # The sketch-wide bounds also contain the separate open conic above it.
    first = entities[ellipse['firstAxisId']]
    second = entities[ellipse['secondAxisId']]
    for point_id, coordinate, expected in ((first['startPointId'], 'uMm', -137.5),
                                          (first['endPointId'], 'uMm', 137.5),
                                          (second['startPointId'], 'vMm', -50),
                                          (second['endPointId'], 'vMm', 50)):
        near(points[point_id][coordinate], expected)
    return ellipse


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--record-video', type=Path)
    args = parser.parse_args()
    output = Path(args.output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    movie = args.record_video.resolve() if args.record_video else None
    if movie:
        assert not movie.is_relative_to(Path(__file__).resolve().parents[1])
    model = output / 'slots-splines-three-sketches.alloy'
    report = {
        'status': 'incomplete', 'sourceId': SOURCE_ID,
        'scope': 'Three editable sketches; source has no solid or STL',
        'appSha256': hashlib.sha256(Path(args.app).read_bytes()).hexdigest(),
        'inputEvidence': 'Continuous shared-API creation movie with captions; native ingredients separate',
        'sourceChoices': {
            'slotCenter': 'Origin is treated as center, resolving the public prose/diagram discrepancy',
            'fitAndControlCoordinates': 'Arbitrary nondegenerate Alloy fixture coordinates',
            'conicCoordinates': 'Arbitrary open conic above ellipse; 0.683 is a pictured example',
            'conicRho': 'Alloy rational shoulder mapping, not a source kernel identity claim',
            'ellipseConstraints': 'Origin Coincident, Horizontal and two full-axis dimensions explicitly added where prose omits actions',
            'pointerInput': 'Shared projected viewport commands; captions are API actions, not native physical gestures'},
        'stages': []}
    transcript = []
    recording = False
    try:
        with application(args.app, output) as (raw, transcript), ExitStack() as cleanup:
            def stop_recording():
                nonlocal recording
                if recording:
                    recording = False
                    report['recording'] = raw('recording.stop')
            cleanup.callback(stop_recording)
            raw('window.resize', {'width': 1500, 'height': 1000})
            assert not raw('document.inspect')['authored']['features']
            if movie:
                movie.parent.mkdir(parents=True, exist_ok=True)
                raw('recording.start', {'path': str(movie), 'fps': 10})
                recording = True

            def call(command, values=None):
                result = raw(command, values)
                if recording and command.startswith(('sketch.', 'view.', 'feature.', 'history.')):
                    time.sleep(.16)
                return result

            def stage(number, caption):
                assert number == len(report['stages']) + 1, (number, report['stages'])
                report['stages'].append({'step': number, 'caption': caption})
                if recording:
                    raw('recording.step', {'text': f'API | {number:02d}: {caption}'})
                    time.sleep(.45)

            def session():
                return raw('app.state')['activeSession']

            def draft():
                return session()['draftSketch']

            def parameters():
                return draft()['parameters']

            def inspect():
                return raw('document.inspect')

            def point(u, v):
                projected = raw('view.projectSketchPoint', {'sketchId': draft()['id'], 'uMm': u, 'vMm': v})
                return {'x': projected['x'], 'y': projected['y']}

            def click(u, v):
                return call('sketch.placePoint', point(u, v))

            def capture(name):
                raw('capture.screenshot', {'path': str(output / (name + '.png'))})
                (output / (name + '.json')).write_text(json.dumps(raw('app.state'), indent=2) + '\n')

            stage(1, 'Blank metric design; begin XY Sketch1')
            call('sketch.begin', {'plane': 'XY', 'name': 'Slots'})
            call('sketch.lookAt')
            call('view.zoom', {'factor': .45})
            stage(2, 'Create > Slot > Center Point Slot')
            call('sketch.tool', {'tool': 'centerPointSlot'})
            stage(3, 'Origin center and rough diagonal end-center pick')
            click(0, 0)
            click(68, 12)
            stage(4, 'Third width pick; leave the slot tool')
            click(68, 33)
            straight = parameters()['slotGroups'][0]
            assert session()['solverDiagnostics']['dof'] == 3
            call('sketch.tool', {'tool': 'select'})
            capture('01-rough-straight-slot')
            stage(5, 'Drag one underdefined slot endpoint before constraints')
            before = draft()
            guide = by_id(parameters()['entities'])[straight['centerlineId']]
            call('sketch.movePoint', {'pointId': guide['endPointId'], 'x': 70, 'y': 8, 'units': 'mm'})
            assert draft() != before
            assert session()['solverDiagnostics']['dof'] == 3
            capture('02-dragged-underdefined-slot')
            stage(6, 'Horizontal/Vertical on retained dashed centerline')
            call('sketch.constraint', {'type': 'horizontal', 'entityIds': [straight['centerlineId']]})
            assert session()['solverDiagnostics']['dof'] == 2
            stage(7, 'D150 centerline and R25 end cap; straight slot fully defined')
            call('sketch.dimension', {'kind': 'linear', 'entityId': straight['centerlineId'],
                 'value': 150, 'units': 'mm', 'annotation': {'uMm': 0, 'vMm': -45}})
            straight_length_id = parameters()['dimensions'][-1]['id']
            call('sketch.dimension', {'kind': 'radius', 'entityId': straight['arcIds'][0],
                 'value': 25, 'units': 'mm', 'annotation': {'uMm': 105, 'vMm': 32}})
            assert session()['solverDiagnostics']['dof'] == 0
            capture('03-defined-straight-slot')
            stage(8, 'Create > Slot > Center Point Arc Slot')
            call('sketch.tool', {'tool': 'centerPointArcSlot'})
            stage(9, 'Origin path center, rough endpoints, fourth width pick')
            click(0, 0)
            click(-100, 110)
            click(105, 98)
            click(-110, 121)
            arc = parameters()['arcSlotGroups'][0]
            call('sketch.tool', {'tool': 'select'})
            capture('04-rough-two-slots')
            stage(10, 'Path R150, end R15 and horizontal path endpoint pair')
            call('sketch.dimension', {'kind': 'radius', 'entityId': arc['pathArcId'], 'value': 150,
                 'units': 'mm', 'annotation': {'uMm': 0, 'vMm': 175}})
            call('sketch.dimension', {'kind': 'radius', 'entityId': arc['capIds'][0], 'value': 15,
                 'units': 'mm', 'annotation': {'uMm': -115, 'vMm': 122}})
            path = by_id(parameters()['entities'])[arc['pathArcId']]
            call('sketch.constraint', {'type': 'horizontal', 'pointIds': [path['startPointId'], path['endPointId']]})
            stage(11, 'Line from left path end to origin; convert it to Construction')
            left = by_id(parameters()['points'])[path['startPointId']]
            call('sketch.line', {'x1': left['uMm'], 'y1': left['vMm'], 'x2': 0, 'y2': 0, 'units': 'mm'})
            radial = parameters()['entities'][-1]
            assert radial['startPointId'] == path['startPointId']
            assert radial['endPointId'] in coincident_group(parameters(), parameters()['originPointId'])
            call('sketch.construction', {'entityIds': [radial['id']], 'construction': True})
            stage(12, 'D135 between radial line and original slot guide')
            points = by_id(parameters()['points'])
            guide = by_id(parameters()['entities'])[straight['centerlineId']]
            guide_sign = 1 if points[guide['endPointId']]['uMm'] > points[guide['startPointId']]['uMm'] else -1
            call('sketch.dimension', {'kind': 'angle', 'entityIds': [guide['id'], radial['id']],
                 'raySigns': [guide_sign, -1], 'valueDeg': 135,
                 'annotation': {'uMm': -35, 'vMm': 55}})
            stage(13, 'Inspect two fully defined closed slot regions; Finish Sketch1')
            assert session()['solverDiagnostics']['dof'] == 0
            first_id = draft()['id']
            verify_slots(draft())
            capture('05-defined-two-slots')
            call('sketch.finish')
            first_eval = next(s for s in inspect()['evaluation']['sketches'] if s['sketchId'] == first_id)
            areas = sorted(p['areaMm2'] for p in first_eval['profiles'])
            assert len(areas) == 2
            for actual, expected in zip(areas, sorted((STRAIGHT_AREA, ARC_AREA))):
                near(actual, expected)
            assert first_eval['solver']['dof'] == 0
            stage(14, 'Browser: hide Sketch1')
            call('feature.visibility', {'featureId': first_id, 'visible': False})
            assert not inspect()['authored']['features'][0]['visible']

            stage(15, 'New XY Sketch2; activate Fit Point Spline')
            call('sketch.begin', {'plane': 'XY', 'name': 'Splines'})
            call('sketch.lookAt')
            call('view.zoom', {'factor': .7})
            call('sketch.tool', {'tool': 'fitSpline'})
            stage(16, 'Three arbitrary fit picks; Complete check and Escape')
            for u, v in ((-45, -35), (-15, 0), (45, -30)):
                click(u, v)
            assert len(session()['pendingSplinePoints']) == 3
            call('sketch.splineComplete')
            fit = next(e for e in parameters()['entities'] if e['type'] == 'fitSpline')
            fit_id = fit['id']
            original_fit_ids = fit['pointIds'][:]
            call('sketch.tool', {'tool': 'select'})
            stage(17, 'Select fit curve and inspect paired tangent handles')
            call('sketch.selectionSet', {'entityIds': [fit_id]})
            capture('06-fit-three-handles')
            stage(18, 'Select and Delete middle fit point')
            call('sketch.selectionSet', {'pointIds': [original_fit_ids[1]]})
            call('sketch.splineRemove', {'entityId': fit_id, 'pointId': original_fit_ids[1]})
            fit = by_id(parameters()['entities'])[fit_id]
            assert fit['pointIds'] == [original_fit_ids[0], original_fit_ids[2]]
            stage(19, 'Insert Spline Fit Point on the selected curve')
            call('sketch.splineInsertMode', {'entityId': fit_id})
            click(0, -32.5)
            fit = by_id(parameters()['entities'])[fit_id]
            assert len(fit['pointIds']) == 3 and fit['pointIds'][0] == original_fit_ids[0]
            assert fit['pointIds'][-1] == original_fit_ids[-1]
            inserted_id = fit['pointIds'][1]
            stage(20, 'Drag tangent grip to change direction and weight')
            call('sketch.tool', {'tool': 'select'})
            before = draft()
            call('sketch.splineDrag', {'phase': 'begin', 'entityId': fit_id,
                 'pointId': inserted_id, 'kind': 'tangent', 'sign': 1})
            point_value = by_id(parameters()['points'])[inserted_id]
            target = {'uMm': point_value['uMm'] + 20, 'vMm': point_value['vMm'] + 16}
            preview = call('sketch.splineDrag', {'phase': 'move', **target})
            assert draft() == before and preview['previewDraft'] != before
            call('sketch.splineDrag', {'phase': 'end', **target})
            assert draft() != before
            capture('07-fit-edited')
            stage(21, 'Activate Control Point Spline')
            call('sketch.tool', {'tool': 'controlSpline'})
            stage(22, 'Three control picks; Complete check and Escape')
            for u, v in ((-35, 55), (0, 92), (35, 55)):
                click(u, v)
            assert len(session()['pendingSplinePoints']) == 3
            call('sketch.splineComplete')
            control = next(e for e in parameters()['entities'] if e['type'] == 'controlSpline')
            control_id, poles = control['id'], control['pointIds'][:]
            call('sketch.tool', {'tool': 'select'})
            stage(23, 'Drag middle control-cage point; inspect changed curve')
            before = draft()
            call('sketch.splineDrag', {'phase': 'begin', 'entityId': control_id,
                 'pointId': poles[1], 'kind': 'point'})
            preview = call('sketch.splineDrag', {'phase': 'move', 'uMm': 5, 'vMm': 108})
            assert draft() == before and preview['previewDraft'] != before
            call('sketch.splineDrag', {'phase': 'end', 'uMm': 5, 'vMm': 108})
            assert draft() != before and by_id(parameters()['entities'])[control_id]['pointIds'] == poles
            stage(24, 'Select spline and enable Curvature Comb in Sketch Palette')
            call('sketch.selectionSet', {'entityIds': [control_id]})
            comb = call('sketch.curvatureComb', {'entityId': control_id, 'enabled': True,
                 'density': 24, 'scale': .1})
            stage(25, 'Inspect derivative-based comb and handles; Finish Sketch2')
            control_detail = next(e for e in comb['sketchGeometry']['entities'] if e['id'] == control_id)
            assert control_detail['degree'] == 2 and control_detail['curvatureComb']['enabled']
            assert len(control_detail['curvatureComb']['samples']) >= 4
            second_id = draft()['id']
            capture('08-splines-cage-comb')
            call('sketch.finish')
            stage(26, 'Hide Sketch2; new XY Sketch3 and Conic Curve')
            call('feature.visibility', {'featureId': second_id, 'visible': False})
            assert not inspect()['authored']['features'][1]['visible']
            call('sketch.begin', {'plane': 'XY', 'name': 'Conic and Ellipse'})
            call('sketch.lookAt')
            call('view.zoom', {'factor': .45})
            call('sketch.tool', {'tool': 'conic'})
            stage(27, 'Three conic picks; adjust retained Rho to pictured example 0.683')
            for u, v in ((-35, 110), (35, 110), (0, 155)):
                click(u, v)
            assert session()['pendingConic']['valid']
            call('sketch.conicPreview', {'text': '0.683'})
            assert session()['pendingConic']['valid']
            capture('09-conic-rho-preview')
            stage(28, 'Conic OK; retain one open curve')
            call('sketch.conicAccept')
            conic = next(e for e in parameters()['entities'] if e['type'] == 'conic')
            near(conic['rho'], .683, 1e-8)
            call('sketch.tool', {'tool': 'select'})
            stage(29, 'Choose Center Point Ellipse')
            call('sketch.tool', {'tool': 'centerPointEllipse'})
            stage(30, 'Center, first-axis and contour point picks; leave tool')
            click(0, 0)
            click(130, 0)
            click(78, 40)
            ellipse = next(e for e in parameters()['entities'] if e['type'] == 'ellipse')
            assert session()['solverDiagnostics']['dof'] > 0
            call('sketch.tool', {'tool': 'select'})
            stage(31, 'Explicit origin/H/D275/D100 ellipse definition; Finish and Save')
            call('sketch.constraint', {'type': 'horizontal', 'entityIds': [ellipse['firstAxisId']]})
            call('sketch.dimension', {'kind': 'linear', 'entityId': ellipse['firstAxisId'],
                 'value': 275, 'units': 'mm', 'annotation': {'uMm': 0, 'vMm': -80}})
            first_axis_dimension = parameters()['dimensions'][-1]['id']
            call('sketch.dimension', {'kind': 'linear', 'entityId': ellipse['secondAxisId'],
                 'value': 100, 'units': 'mm', 'annotation': {'uMm': 165, 'vMm': 0}})
            third_id = draft()['id']
            assert any(c['type'] == 'coincident' and ellipse['centerPointId'] in c.get('pointIds', [])
                       for c in parameters()['constraints'])
            assert session()['solverDiagnostics']['dof'] > 0, 'Open conic keeps whole sketch underdefined'
            capture('10-dimensioned-ellipse-and-conic')
            call('sketch.finish')
            source = inspect()
            assert len(source['authored']['features']) == 3
            assert [f['id'] for f in source['authored']['features']] == [first_id, second_id, third_id]
            assert [f['visible'] for f in source['authored']['features']] == [False, False, True]
            first, second, third = source['authored']['features']
            verify_slots(first)
            first_eval = next(s for s in source['evaluation']['sketches'] if s['sketchId'] == first_id)
            assert len(first_eval['profiles']) == 2
            for actual, expected in zip(sorted(p['areaMm2'] for p in first_eval['profiles']),
                                        sorted((STRAIGHT_AREA, ARC_AREA))):
                near(actual, expected)
            second_entities = by_id(second['parameters']['entities'])
            assert second_entities[fit_id]['pointIds'][1] == inserted_id
            assert second_entities[control_id]['pointIds'] == poles
            second_eval = next(s for s in source['evaluation']['sketches'] if s['sketchId'] == second_id)
            fit_detail = next(e for e in second_eval['entities'] if e['id'] == fit_id)
            control_detail = next(e for e in second_eval['entities'] if e['id'] == control_id)
            assert fit_detail['degree'] == 5 and fit_detail['continuity'] == 'C2'
            assert control_detail['degree'] == 2 and control_detail['curvatureComb']['enabled']
            third_eval = next(s for s in source['evaluation']['sketches'] if s['sketchId'] == third_id)
            verify_ellipse(third, third_eval)
            assert next(e for e in third['parameters']['entities'] if e['id'] == conic['id'])['rho'] == .683
            assert third_eval['solver']['dof'] > 0
            call('document.save', {'path': str(model)})
            report['sourceAuthored'] = source['authored']
            report['sourceGeometry'] = {
                'slotAreasMm2': [p['areaMm2'] for p in first_eval['profiles']],
                'ellipseAreaMm2': next(p['areaMm2'] for p in third_eval['profiles']
                                       if ellipse['id'] in p['sourceEntityIds']),
                'conicRho': .683, 'fitPointIds': second_entities[fit_id]['pointIds'],
                'controlPointIds': poles, 'firstSketchDof': first_eval['solver']['dof'],
                'thirdSketchDof': third_eval['solver']['dof']}
            stop_recording()

            # Post-source integrity checks are intentionally outside the movie.
            before = source['authored']
            call('sketch.edit', {'sketchId': first_id})
            call('sketch.dimensionUpdate', {'dimensionId': straight_length_id,
                 'value': 170, 'units': 'mm'})
            call('sketch.finish')
            edited = inspect()['authored']
            assert edited != before
            call('history.undo')
            assert inspect()['authored'] == before
            call('history.redo')
            assert inspect()['authored'] == edited
            call('history.undo')
            call('sketch.edit', {'sketchId': second_id})
            call('sketch.splineTangent', {'entityId': fit_id, 'pointId': inserted_id,
                 'duMm': 100, 'dvMm': 50})
            call('sketch.finish')
            curve_edit = inspect()['authored']
            assert curve_edit != before
            call('history.undo')
            assert inspect()['authored'] == before
            call('history.redo')
            assert inspect()['authored'] == curve_edit
            call('history.undo')
            call('document.open', {'path': str(model)})
            reopened = inspect()
            assert reopened['authored'] == before
            assert [f['visible'] for f in reopened['authored']['features']] == [False, False, True]
            verify_slots(reopened['authored']['features'][0])
            verify_ellipse(reopened['authored']['features'][2],
                           next(s for s in reopened['evaluation']['sketches'] if s['sketchId'] == third_id))
            report.update(status='passed', savedModel=str(model), editedAuthored=edited,
                          curveEditedAuthored=curve_edit, reopenedAuthored=reopened['authored'])
    finally:
        report['callCount'] = len(transcript)
        report['transcript'] = transcript
        (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print('PASS: all 31 source steps, three editable sketches, geometry and history')


if __name__ == '__main__':
    main()
