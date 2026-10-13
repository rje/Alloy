#!/usr/bin/env python3
"""Prepared #249 blank-to-plate source runner. Do not claim a pass before execution.

The movie uses shared Alloy commands. Native input is established by separate
ingredient receipts. Reference provenance remains in the external knowledge record.
"""
import argparse
from contextlib import ExitStack
import hashlib
import json
import math
from pathlib import Path
import time

from app_harness import application
from countersunk_plate_source import create_stock_and_point
from stl_export_api import inspect_stl

SOURCE_ID = 'plate-countersink-pattern-20261013'
SOURCE_VOLUME = 36500 - 353 * math.pi
GRID = sorted((x, y, 10) for x in (-27.5, 0, 27.5) for y in (15, 0, -15))


def near(actual, expected, tolerance=1e-5):
    assert math.isclose(actual, expected, abs_tol=tolerance), (actual, expected)


def pattern_axes(body):
    """Choose actual upper straight rims; signs follow retained oriented roles."""
    result = []
    for axis, distance in ((0, 27.5), (1, -15)):
        matches = []
        for edge in body['solidEdges']:
            if not edge['available'] or edge['provenance']['role'] != 'endRim':
                continue
            start, end = edge['startMm'], edge['endMm']
            if not all(math.isclose(p[2], 10, abs_tol=1e-6) for p in (start, end)):
                continue
            vector = [end[i] - start[i] for i in range(3)]
            if abs(vector[axis]) <= 1 or any(abs(vector[i]) > 1e-6 for i in range(3) if i != axis):
                continue
            matches.append((edge, vector))
        assert matches, f'Missing upper retained straight rim for axis {axis}'
        edge, vector = matches[0]
        result.append({'edgeId': edge['edgeId'], 'quantity': 3,
                       'spacingMm': math.copysign(abs(distance), vector[axis]) * (1 if distance > 0 else -1),
                       'direction': 'one'})
    assert result[0]['edgeId'] != result[1]['edgeId']
    return result


def check_pattern(document, target, seed, axes, expected_volume=SOURCE_VOLUME,
                  expected_centers=GRID):
    authored = document['authored']
    feature = authored['features'][-1]
    assert feature['type'] == 'rectangularPattern'
    parameters = feature['parameters']
    assert parameters['sourceFeatureId'] == seed and parameters['targetBodyId'] == target
    assert parameters['distribution'] == 'spacing' and parameters['computeType'] == 'adjust'
    assert parameters['axes'] == axes
    bodies = document['evaluation']['bodies']
    assert len(bodies) == 1 and bodies[0]['bodyId'] == target
    body = bodies[0]
    assert body['valid'] and body['closed'] and body['topology']['solids'] == 1
    assert body['instanceCount'] == 9 and len(body['instances']) == 9
    centers = sorted(tuple(round(v, 6) for v in item['centerMm']) for item in body['instances'])
    assert centers == expected_centers, (centers, expected_centers)
    near(body['volumeMm3'], expected_volume)
    return feature, body


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--record-video', type=Path)
    parser.add_argument('--geometry-verifier', type=Path,
                        help='Compiled countersunk_plate_verify executable for saved BRep checks.')
    args = parser.parse_args()
    output = Path(args.output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    movie = args.record_video.resolve() if args.record_video else None
    if movie:
        assert not movie.is_relative_to(Path(__file__).resolve().parents[1])
    model = output / 'countersunk-plate.alloy'
    mesh = output / 'countersunk-plate-mm.stl'
    report = {'result': 'incomplete', 'sourceId': SOURCE_ID, 'stages': [],
              'sourceChoices': {
                  'drawing': 'Legible embedded page-1 drawing stands in for separately named companion drawing',
                  'directions': 'Alloy selects two retained upper straight rims; their oriented signs yield +X and -Y',
                  'suppression': 'All nine occurrences; source screenshot shows no omitted occurrence',
                  'exportAndEdits': 'STL, parameter edits and history are Alloy validation after source Save'},
              'evidenceKinds': {'movie': 'actual Alloy shared-command creation, not physical native input',
                                'geometry': 'evaluated OCCT solid plus independent saved-BRep classifier',
                                'mesh': 'strict stdlib STL readback'},
              'dependencies': '#248 Hole Pattern and #250 dialog capture must be integrated and accepted'}
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
            raw('window.resize', {'width': 1400, 'height': 1000})
            assert not raw('document.inspect')['authored']['features']
            if movie:
                movie.parent.mkdir(parents=True, exist_ok=True)
                raw('recording.start', {'path': str(movie), 'fps': 10})
                recording = True
                time.sleep(.5)
            def call(command, inputs=None):
                result = raw(command, inputs)
                if recording and command.startswith(('sketch.', 'solid.', 'selection.', 'command.', 'view.')):
                    time.sleep(.2)
                return result
            def stage(caption):
                report['stages'].append(caption)
                if recording:
                    raw('recording.step', {'text': 'API | ' + caption})
                    time.sleep(.45)
            def inspect():
                return call('document.inspect')
            def capture(name):
                call('capture.screenshot', {'path': str(output / (name + '.png'))})
                (output / (name + '.json')).write_text(json.dumps(call('app.state'), indent=2) + '\n')

            stage('Inspect embedded drawing; Change Parameters: Length 75, Width 50, Thickness 10 mm')
            opened = call('document.parametersDialog')
            assert opened['visible']
            for name, value in (('Length', 75), ('Width', 50), ('Thickness', 10)):
                call('document.parameterAdd', {'name': name, 'value': value, 'units': 'mm'})
                if recording:
                    time.sleep(.28)
            table = call('capture.dialog', {'path': str(output / '00-parameter-table.png')})
            report['parameterDialog'] = table
            call('document.parametersDialog', {'visible': False})
            parameters = {p['name']: p for p in inspect()['userParameters']}
            assert {name: parameters[name]['valueMm'] for name in ('Length', 'Width', 'Thickness')} == {
                'Length': 75, 'Width': 50, 'Thickness': 10}

            prefix = create_stock_and_point(call, stage, capture, existing_parameters=parameters)
            assert {name: prefix['parameters'][name]['id'] for name in parameters} == {
                name: parameters[name]['id'] for name in parameters}
            target = prefix['targetBodyId']
            before_hole = inspect()['authored']

            stage('Hole from retained sketch Point: Countersink bore Ø4, sink Ø10, 90°, All')
            hole_inputs = {'sourceSketchId': prefix['sourceSketchId'],
                           'sourceEntityId': prefix['sourceEntityId'],
                           'sourcePointId': prefix['sourcePointId'], 'targetBodyId': target,
                           'placement': 'fromSketch', 'holeType': 'countersink', 'extent': 'all',
                           'boreDiameterMm': 4, 'sinkDiameterMm': 10, 'includedAngleDeg': 90}
            hole_session = call('solid.holeDialog', hole_inputs)
            hole_preview = call('command.preview', {'session': hole_session['id']})
            assert hole_preview['previewed'] and hole_preview['geometryEvaluated']
            near(hole_preview['geometry']['sinkDepthMm'], 3, 1e-7)
            call('capture.dialog', {'path': str(output / '04-hole-panel.png')})
            capture('04-hole-preview')
            call('command.commit', {'session': hole_session['id']})
            drilled = inspect()
            hole = drilled['authored']['features'][-1]
            assert hole['type'] == 'hole' and hole['parameters'] == hole_inputs
            near(drilled['evaluation']['bodies'][0]['volumeMm3'], 36500 + 183*math.pi)

            stage('Rectangular Pattern: Hole Features, two upper edges, Spacing 3×3 at 27.5/15, One Direction, Adjust')
            axes = pattern_axes(drilled['evaluation']['bodies'][0])
            pattern_session = call('solid.rectangularPatternDialog')
            call('selection.featureSet', {'featureId': hole['id']})
            for index, axis in enumerate(axes):
                call('selection.patternAxisSet', {'edgeId': axis['edgeId'], 'axis': index})
            call('command.update', {'session': pattern_session['id'], 'inputs': {'axes': axes}})
            pattern_preview = call('command.preview', {'session': pattern_session['id']})
            assert pattern_preview['previewed'] and pattern_preview['geometryEvaluated']
            assert pattern_preview['geometry']['instanceCount'] == 9
            assert pattern_preview['geometry']['distribution'] == 'Spacing'
            assert pattern_preview['geometry']['computeType'] == 'Adjust'
            call('capture.dialog', {'path': str(output / '05-pattern-panel.png')})
            capture('05-pattern-preview')
            call('command.commit', {'session': pattern_session['id']})
            finished = inspect()
            pattern, body = check_pattern(finished, target, hole['id'], axes)
            report['sourceState'] = {'volumeMm3': body['volumeMm3'], 'bodyId': target,
                                     'holeId': hole['id'], 'patternId': pattern['id'],
                                     'seedCenterMm': body['seedCenterMm'], 'axes': axes,
                                     'centersMm': GRID}
            stage('Verify drawing and Save editable plate')
            call('view.orientation', {'name': 'isometric'})
            call('view.home')
            capture('06-final-plate')
            call('document.save', {'path': str(model)})
            report['savedAuthored'] = finished['authored']
            stop_recording()

            stage('Alloy validation: export millimeter STL; Length and Thickness edits with exact history')
            export = call('document.exportStl', {'path': str(mesh), 'bodyId': target,
                                                 'units': 'mm', 'format': 'binary',
                                                 'surfaceDeviationMm': .08,
                                                 'normalDeviationDeg': 8})
            assert not export['authoredChanged'] and inspect()['authored'] == finished['authored']
            report['export'] = export
            for name, value, seed_x, seed_z, volume in (
                    ('Length', 85, -32.5, 10, 41500-353*math.pi),
                    ('Thickness', 12, -27.5, 12, 43800-375*math.pi)):
                before = inspect()['authored']
                call('document.parameterUpdate', {'parameterId': parameters[name]['id'],
                                                  'value': value, 'units': 'mm'})
                changed = inspect()
                shifted_grid = sorted((x + (seed_x + 27.5), y, seed_z)
                                      for x in (-27.5, 0, 27.5) for y in (15, 0, -15))
                _, changed_body = check_pattern(changed, target, hole['id'], axes,
                                                volume, shifted_grid)
                near(changed_body['seedCenterMm'][0], seed_x, 1e-6)
                near(changed_body['seedCenterMm'][2], seed_z, 1e-6)
                assert changed['authored'] != before
                call('history.undo')
                assert inspect()['authored'] == before
                call('history.redo')
                assert inspect()['authored'] == changed['authored']
                call('history.undo')
                assert inspect()['authored'] == before
                report.setdefault('edits', {})[name] = {
                    'valueMm': value, 'seedCenterMm': changed_body['seedCenterMm'],
                    'volumeMm3': changed_body['volumeMm3'], 'undoRedo': 'exact'}
            call('document.open', {'path': str(model), 'discard': True})
            reopened = inspect()
            assert reopened['authored'] == finished['authored']
            check_pattern(reopened, target, hole['id'], axes)
            report['reopen'] = 'exact authored source state'
        mesh_readback = inspect_stl(mesh, True)
        assert len(mesh_readback['components']) == 1
        assert mesh_readback['triangles'] == report['export']['triangles']
        for actual, expected in zip(mesh_readback['bounds'][0], [-37.5, -25, 0]):
            near(actual, expected, .03)
        for actual, expected in zip(mesh_readback['bounds'][1], [37.5, 25, 10]):
            near(actual, expected, .03)
        assert math.isclose(mesh_readback['signedVolume'], SOURCE_VOLUME, rel_tol=.003)
        report['mesh'] = mesh_readback
        if args.geometry_verifier:
            import subprocess
            completed = subprocess.run([str(args.geometry_verifier.resolve()), str(model)],
                                       check=True, text=True, capture_output=True)
            report['savedBrepVerification'] = completed.stdout.strip()
        else:
            report['savedBrepVerification'] = 'pending: pass --geometry-verifier for full gate'
        report['sha256'] = {path.name: hashlib.sha256(path.read_bytes()).hexdigest()
                            for path in (model, mesh)}
        report['result'] = 'api-pass' if args.geometry_verifier else 'incomplete'
    finally:
        report['transcript'] = transcript
        (output / 'countersunk-plate-tutorial-api.json').write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
