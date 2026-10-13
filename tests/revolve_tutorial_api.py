#!/usr/bin/env python3
"""Illustrated Revolve workflow through the real app; native child evidence is separate."""
import argparse
from contextlib import ExitStack
import json
import math
from pathlib import Path
import time
from app_harness import application
from stl_export_api import inspect_stl

POLYGON = [(50, 0), (67.5, 0), (72.5, 7), (72.5, 22),
           (82.5, 22), (82.5, 32), (69, 50), (50, 50)]
FULL_VOLUME = (479338 / 3) * math.pi


def pappus_volume(vertices):
    """Independent polygon first moment about the vertical sketch origin axis."""
    pairs = zip(vertices, vertices[1:] + vertices[:1])
    moment = sum((a[0] + b[0]) * (a[0] * b[1] - b[0] * a[1]) for a, b in pairs) / 6
    return 2 * math.pi * abs(moment)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--record-video', type=Path)
    args = parser.parse_args()
    output = Path(args.output).resolve()
    report = {'result': 'incomplete', 'sourceId': 'tutorial.next-revolve-feature-checkpoint',
              'sourceSteps': list(range(39, 51)), 'stages': [],
              'fixture': {'axis': [[0, 0], [0, 55]], 'polygonMm': POLYGON,
                          'diametersMm': [100, 165], 'partialAngleDeg': 70,
                          'choice': 'Alloy-authored rough silhouette, dimensions and constraints; source does not prescribe coordinates'}}
    recording = False
    transcript = []
    try:
        with application(args.app, output) as (raw, transcript), ExitStack() as cleanup:
            def stop():
                nonlocal recording
                if recording:
                    recording = False
                    report['recording'] = raw('recording.stop')
            cleanup.callback(stop)
            raw('window.resize', {'width': 1400, 'height': 1000})
            assert not raw('document.inspect')['authored']['features'], 'Gate must start blank'
            if args.record_video:
                movie = args.record_video.resolve()
                assert not movie.is_relative_to(Path(__file__).resolve().parents[1]), 'Movie remains external'
                movie.parent.mkdir(parents=True, exist_ok=True)
                raw('recording.start', {'path': str(movie), 'fps': 10})
                recording = True
                time.sleep(.8)  # Preserve a visible blank starting frame.

            def call(command, parameters=None):
                value = raw(command, parameters)
                if recording and command.startswith(('sketch.', 'solid.', 'command.', 'view.')):
                    time.sleep(.4)
                return value

            def step(number, text):
                if recording:
                    raw('recording.step', {'text': f'{number}: {text}'})
                    time.sleep(.6)
                report['stages'].append({'step': number, 'caption': text})

            def state(): return call('app.state')
            def inspect(): return call('document.inspect')
            def draft(): return state()['activeSession']['draftSketch']
            def body():
                rows = inspect()['evaluation']['bodies']
                assert len(rows) == 1
                assert rows[0]['valid'] and rows[0]['closed']
                return rows[0]
            def capture(name):
                call('capture.screenshot', {'path': str(output / (name + '.png'))})
            def project(u, v):
                return call('view.projectSketchPoint', {'sketchId': draft()['id'], 'uMm': u, 'vMm': v})
            def ensure_visible(vertices):
                # Geometry is visible throughout creation, not revealed only by
                # a final fit. Include mirrored witnesses/labels in this check.
                for _ in range(5):
                    viewport = state()['viewport']
                    pixels = [project(*point) for point in vertices]
                    if all(12 < p['x'] < viewport['width'] - 12 and 12 < p['y'] < viewport['height'] - 12 for p in pixels):
                        return
                    call('view.zoom', {'factor': .8})
                raise AssertionError('Required creation/dimension geometry is outside the viewport')

            def select_diameter(edge, label):
                call('sketch.tool', {'tool': 'dimension'})  # Shared command behind D; native key proven by #215.
                call('sketch.selectionSet', {'entityIds': [], 'pointIds': []})
                point = project(0, 27.5)
                call('sketch.selectionPick', {'x': point['x'], 'y': point['y']})
                points = {p['id']: p for p in draft()['parameters']['points']}
                a, b = points[edge['startPointId']], points[edge['endPointId']]
                point = project((a['uMm'] + b['uMm']) / 2, (a['vMm'] + b['vMm']) / 2)
                call('sketch.selectionPick', {'x': point['x'], 'y': point['y'], 'additive': True})
                point = project(*label)
                pending = call('sketch.dimensionPlace', {'x': point['x'], 'y': point['y']})
                assert pending['kind'] == 'centerlineDiameter'
                return pending

            step(39, 'Create a Front / XZ sketch from a blank design')
            call('sketch.create', {'plane': 'XZ', 'name': 'Stepped radial profile — Alloy fixture'})
            step(40, 'Line tool: create an origin-up 55 mm axis (Alloy choice)')
            call('sketch.tool', {'tool': 'line'})
            call('sketch.line', {'x1': 0, 'y1': 0, 'x2': 0, 'y2': 55, 'units': 'mm'})
            axis = draft()['parameters']['entities'][0]
            call('sketch.constraint', {'type': 'fixed', 'pointIds': [axis['startPointId']]})
            call('sketch.constraint', {'type': 'vertical', 'entityIds': [axis['id']]})
            call('sketch.dimension', {'kind': 'linear', 'entityId': axis['id'], 'value': 55, 'units': 'mm',
                                      'annotation': {'uMm': -15, 'vMm': 27.5}})
            call('sketch.tool', {'tool': 'select'})
            call('sketch.lookAt')
            capture('01-axis')
            step(41, 'Select the axis and convert its linetype to Centerline')
            call('sketch.selectionSet', {'entityIds': [axis['id']]})
            call('sketch.centerline', {'entityIds': [axis['id']], 'centerline': True})
            assert draft()['parameters']['entities'][0]['centerline'] is True
            assert not draft()['parameters']['entities'][0].get('construction', False)
            step(42, 'Centerline retains its identity and supports full diameter dimensions')
            capture('02-centerline')
            step(43, 'Line tool: draw a closed stepped outline on one side of the axis')
            call('view.zoom', {'factor': .55})
            ensure_visible(POLYGON + [(0, 0), (0, 55)])
            call('sketch.tool', {'tool': 'line'})
            for index, (a, b) in enumerate(zip(POLYGON, POLYGON[1:] + POLYGON[:1])):
                if recording:
                    raw('recording.step', {'text': f'43: Profile edge {index + 1}/8 — independently chosen rough outline'})
                call('sketch.line', {'x1': a[0], 'y1': a[1], 'x2': b[0], 'y2': b[1], 'units': 'mm'})
                edge = draft()['parameters']['entities'][-1]
                if a[0] == b[0]:
                    call('sketch.constraint', {'type': 'vertical', 'entityIds': [edge['id']]})
                elif a[1] == b[1]:
                    call('sketch.constraint', {'type': 'horizontal', 'entityIds': [edge['id']]})
            call('sketch.tool', {'tool': 'select'})
            call('sketch.lookAt')
            sketch = draft()
            edges = sketch['parameters']['entities'][1:]
            assert len(edges) == 8
            point_ids = {p['id'] for p in sketch['parameters']['points']}
            entity_ids = {e['id'] for e in sketch['parameters']['entities']}
            capture('03-closed-profile')
            call('view.zoom', {'factor': .65})
            ensure_visible(POLYGON + [(-82.5, 69), (82.5, 69), (-15, 27.5)])
            step(44, 'D workflow: select Centerline and inner vertical edge; place full Ø100')
            select_diameter(edges[7], (-4, 61))
            call('sketch.dimensionAccept', {'text': '100 mm'})
            inner_dimension = draft()['parameters']['dimensions'][-1]
            assert inner_dimension['entityIds'] == [axis['id'], edges[7]['id']]
            step(45, 'Add full Ø165 on the outer shoulder; retain explicit orientation constraints')
            select_diameter(edges[4], (-4, 69))
            call('sketch.dimensionAccept', {'text': '165 mm'})
            outer_dimension = draft()['parameters']['dimensions'][-1]
            assert outer_dimension['entityIds'] == [axis['id'], edges[4]['id']]
            call('sketch.tool', {'tool': 'select'})
            report['sketchSolver'] = state()['activeSession']['solverDiagnostics']
            assert report['sketchSolver']['dof'] > 0, 'Do not invent a fully constrained fixture'
            capture('04-diameters')
            call('sketch.finish')
            sketch_geometry = next(s for s in inspect()['evaluation']['sketches'] if s['sketchId'] == sketch['id'])
            assert len(sketch_geometry['profiles']) == 1
            profile_id = sketch_geometry['profiles'][0]['profileId']
            call('view.home')
            call('view.zoom', {'factor': .65})
            step(46, 'Create > Revolve: open the real controls')
            session = call('solid.revolveDialog')
            step(47, 'Inspect automatic sole Profile and Centerline Axis selections')
            assert session['inputs']['sketchId'] == sketch['id']
            assert session['inputs']['profileId'] == profile_id
            assert session['inputs']['axisEntityId'] == axis['id']
            assert session['selectionRole'] == 'ready'
            assert session['nativeControlsAvailable'] is True
            report['automaticSelections'] = session['inputs']
            full_preview = call('command.preview', {'session': session['id']})
            assert math.isclose(full_preview['geometry']['volumeMm3'], FULL_VOLUME, rel_tol=1e-10, abs_tol=1e-5)
            capture('05-automatic-revolve')
            step(48, 'Choose Partial and drag the onscreen angular cue to about 70 degrees')
            call('command.update', {'session': session['id'], 'inputs': {'extent': 'partial', 'angleDeg': 90}})
            call('command.preview', {'session': session['id']})
            cue = state()['viewport']['gpu']['revolveCue']
            assert cue['dragAvailable']
            center, u, v, end = (cue[k] for k in ('centerScreen', 'basisUScreen', 'basisVScreen', 'endScreen'))
            radians = math.radians(70)
            target = {k: center[k] + math.cos(radians) * (u[k] - center[k]) + math.sin(radians) * (v[k] - center[k]) for k in ('x', 'y')}
            call('solid.revolveDrag', {'x': end['x'], 'y': end['y'], 'phase': 'begin'})
            for intermediate_angle in (86, 82, 78, 74, 70):
                radians = math.radians(intermediate_angle)
                position = {k: center[k] + math.cos(radians) * (u[k] - center[k]) + math.sin(radians) * (v[k] - center[k]) for k in ('x', 'y')}
                call('solid.revolveDrag', {**position, 'phase': 'move'})
            call('solid.revolveDrag', {**target, 'phase': 'end'})
            angle = state()['activeSession']['inputs']['angleDeg']
            assert math.isclose(angle, 70, abs_tol=1e-5), angle
            partial = call('command.preview', {'session': session['id']})
            assert math.isclose(partial['geometry']['volumeMm3'], FULL_VOLUME * angle / 360, rel_tol=1e-10, abs_tol=1e-5)
            report['partialDrag'] = {'cue': cue, 'targetScreen': target, 'angleDeg': angle, 'geometry': partial['geometry'], 'input': 'shared native-pointer angular drag command, coordinate-derived; not numeric angle entry'}
            capture('06-partial-angular-drag')
            step(49, 'Choose Full / 360 degrees and OK / commit New Body')
            call('command.update', {'session': session['id'], 'inputs': {'extent': 'full', 'angleDeg': 360}})
            call('command.preview', {'session': session['id']})
            capture('07-full-preview')
            call('command.commit', {'session': session['id']})
            full = inspect()
            original_body = body()
            feature = next(f for f in full['authored']['features'] if f['type'] == 'revolve')
            assert math.isclose(original_body['volumeMm3'], FULL_VOLUME, rel_tol=1e-10, abs_tol=1e-5)
            assert feature['parameters']['profileId'] == profile_id and feature['parameters']['axisEntityId'] == axis['id']
            report['fullBody'] = original_body
            step(50, 'Rotate and explore the completed stepped body')
            call('view.home')
            for orientation in ('top', 'right', 'isometric'):
                call('view.orientation', {'name': orientation})
                if recording:
                    time.sleep(.6)
            capture('08-complete-revolved-body')
            step('Alloy extension', 'Save the editable design and export a millimeter STL')
            model = output / 'stepped-revolve.alloy'
            call('document.save', {'path': str(model)})
            mesh_path = output / 'stepped-revolve-mm.stl'
            report['export'] = call('document.exportStl', {'path': str(mesh_path), 'bodyId': original_body['bodyId'], 'units': 'mm', 'format': 'binary',
                                                        'surfaceDeviationMm': .05, 'normalDeviationDeg': 8})
            report['mesh'] = inspect_stl(mesh_path, True)  # Unchanged strict serialized-facet reader.
            assert len(report['mesh']['components']) == 1
            assert report['export']['triangles'] == report['mesh']['triangles']
            assert report['export']['bytes'] == mesh_path.stat().st_size
            assert report['export']['bodyIds'] == [original_body['bodyId']]
            assert report['export']['authoredChanged'] is False
            assert inspect()['authored'] == full['authored']
            assert math.isclose(report['mesh']['signedVolume'], FULL_VOLUME, rel_tol=.003)
            for actual, expected in zip(report['mesh']['bounds'][0] + report['mesh']['bounds'][1], [-82.5, -82.5, 0, 82.5, 82.5, 50]):
                assert abs(actual - expected) < .06
            if recording:
                time.sleep(2)
            stop()
            # Retained upstream editing is an Alloy gate extension, separate from source movie.
            call('sketch.edit', {'sketchId': sketch['id']})
            call('sketch.dimensionUpdate', {'dimensionId': inner_dimension['id'], 'value': 110, 'units': 'mm'})
            edited_sketch = draft()
            assert {p['id'] for p in edited_sketch['parameters']['points']} == point_ids
            assert {e['id'] for e in edited_sketch['parameters']['entities']} == entity_ids
            solved_points = {p['id']: (p['uMm'], p['vMm']) for p in edited_sketch['parameters']['points']}
            vertices = [solved_points[e['startPointId']] for e in edges]
            edited_volume = pappus_volume(vertices)
            call('sketch.finish')
            edited = inspect()
            edited_body = body()
            assert edited_body['bodyId'] == original_body['bodyId']
            assert math.isclose(edited_body['volumeMm3'], edited_volume, rel_tol=1e-10, abs_tol=1e-5)
            assert not math.isclose(edited_volume, FULL_VOLUME, rel_tol=1e-6)
            assert next(f for f in edited['authored']['features'] if f['type'] == 'revolve') == feature
            call('document.save', {'path': str(output / 'stepped-revolve-edited.alloy')})
            call('history.undo')
            assert inspect()['authored'] == full['authored']
            call('history.redo')
            assert inspect()['authored'] == edited['authored']
            call('history.undo')
            call('document.open', {'path': str(model), 'discard': True})
            assert inspect()['authored'] == full['authored']
            assert math.isclose(body()['volumeMm3'], FULL_VOLUME, rel_tol=1e-10, abs_tol=1e-5)
            report.update(result='api-pass', model=str(model), retainedEdit={'polygonMm': vertices, 'pappusVolumeMm3': edited_volume,
                          'body': edited_body, 'stableProfileId': profile_id, 'stableFeatureId': feature['id']},
                          scope='complete shared-API source-step movie; native keyboard/pointer controls independently qualified in child #215/#216; no physical OS or exact-source geometry claim')
    finally:
        output.mkdir(parents=True, exist_ok=True)
        report['transcript'] = transcript
        (output / 'revolve-tutorial-api.json').write_text(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
