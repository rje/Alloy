#!/usr/bin/env python3
"""Source-step initials keyring from blank through Save; API movie and integrity tail.

Preparation for #233. Requires accepted rounded-loop Offset and 2-Point Circle
integrations. Do not cite this runner as passed until a real-app ledger, movie,
model, mesh, captures and independent review exist.
"""
import argparse
from contextlib import ExitStack
import hashlib
import json
import math
from pathlib import Path
import time

from app_harness import application
from stl_export_api import inspect_stl

SOURCE_ID = 'warwick-initials-keyring-20261013'
OUTER_AREA = 1800 - 36 * (4 - math.pi)
INNER_AREA = 57 * 27 - 4.5**2 * (4 - math.pi)
RIM_AREA = OUTER_AREA - INNER_AREA
BASE_VOLUME = 2 * OUTER_AREA
RIM_VOLUME = 1.5 * RIM_AREA
HOLE_VOLUME = 2 * math.pi * 2**2
SIDES = [((30, 0), (60, -15)), ((60, -15), (30, -30)),
         ((30, -30), (0, -15)), ((0, -15), (30, 0))]


def close(actual, expected, tolerance=1e-5):
    assert math.isclose(actual, expected, abs_tol=tolerance), (actual, expected)


def support_identity(face):
    return {key: face[key] for key in ('producerFeatureId', 'bodyId', 'faceId')}


def floor_face(body, original):
    """Only the retained original z=2 broad cap can support post-fillet text."""
    matches = [face for face in body['supportFaces']
               if support_identity(face) == original
               and face['frame']['normal'] == [0, 0, 1]
               and math.isclose(face['frame']['origin'][2], 2, abs_tol=1e-6)]
    assert len(matches) == 1, 'Broad floor lost unique retained face lineage'
    return matches[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--record-video', type=Path)
    args = parser.parse_args()
    output = Path(args.output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    report = {
        'result': 'incomplete', 'sourceId': SOURCE_ID, 'sourceSteps': list(range(1, 22)),
        'sourceChoices': {
            'fixture': 'XY x0..60, y-30..0; four ordinary R6 quarter-arcs',
            'annotations': '60/30 measured overall reference spans after corner fillets; drivenness retired',
            'rim': 'explicit sketch on actual z2 cap, source transition left sketch creation implicit',
            'hole': 'written diameter 4 over later apparent screenshot diameter 5; first endpoint (5,-15), second direction +X, center (7,-15); placement is Alloy choice',
            'text': 'personalized DSB, pinned licensed DejaVu Sans 2.37 Bold, cap height 14; handles move baseline to (55,-5) and rotate about 180 degrees; source examples, not mandatory font/default',
            'exportAndEdit': 'Save is source step 21; STL and supported same-slot text edit/undo/reopen are Alloy checks'},
        'evidenceKinds': {'movie': 'shared API through actual Alloy commands', 'native': 'reuse accepted child tests',
                          'geometry': 'evaluated OCCT bodies/profiles', 'mesh': 'independent strict STL readback'},
        'stages': []}
    transcript = []
    recording = False
    model = output / 'initials-keyring.alloy'
    mesh = output / 'initials-keyring-mm.stl'
    try:
        with application(args.app, output) as (raw, transcript), ExitStack() as cleanup:
            def stop_recording():
                nonlocal recording
                if recording:
                    recording = False
                    report['recording'] = raw('recording.stop')
            cleanup.callback(stop_recording)
            raw('window.resize', {'width': 1400, 'height': 1000})
            assert not raw('document.inspect')['authored']['features'], 'Must start with a blank design'
            if args.record_video:
                movie = args.record_video.resolve()
                assert not movie.is_relative_to(Path(__file__).resolve().parents[1]), 'Movie stays outside the repository'
                movie.parent.mkdir(parents=True, exist_ok=True)
                raw('recording.start', {'path': str(movie), 'fps': 10})
                recording = True
                time.sleep(.8)
            def call(command, parameters=None):
                answer = raw(command, parameters)
                if recording and command.startswith(('sketch.', 'solid.', 'view.', 'selection.', 'command.')):
                    time.sleep(.24)
                return answer
            def stage(steps, caption, source='illustrated'):
                report['stages'].append({'steps': steps, 'caption': caption, 'source': source})
                if recording:
                    raw('recording.step', {'text': f'API | {steps}: {caption}'})
                    time.sleep(.6)
            def state(): return raw('app.state')
            def inspect(): return raw('document.inspect')
            def active(): return state()['activeSession']
            def draft(): return active()['draftSketch']
            def pixel(u, v):
                return call('view.projectSketchPoint', {'sketchId': draft()['id'], 'uMm': u, 'vMm': v})
            def capture(name):
                call('capture.screenshot', {'path': str(output / (name + '.png'))})
                (output / (name + '.json')).write_text(json.dumps(state(), indent=2) + '\n')
            def sole_body():
                bodies = inspect()['evaluation']['bodies']
                assert len(bodies) == 1 and bodies[0]['valid'] and bodies[0]['closed']
                assert bodies[0]['topology']['solids'] == 1
                return bodies[0]
            def click_pair(pair):
                call('sketch.tool', {'tool': 'fillet'})
                call('sketch.placePoint', pixel(*pair[0]))
                call('sketch.placePoint', pixel(*pair[1]))
                pending = active()['pendingFillet']
                assert pending['sidePair'] and len(pending['corners']) == 1
            def measured_span(sketch, dimension):
                points = {p['id']: p for p in sketch['parameters']['points']}
                a, b = (points[i] for i in dimension['pointIds'])
                axis = 'uMm' if dimension['kind'] == 'horizontal' else 'vMm'
                return abs(a[axis] - b[axis])

            stage('1–5', 'Choose 2-Point Rectangle, XY; origin, 60 by 30 typed fields')
            command = call('sketch.twoPointRectangle')
            assert command['pendingTool'] == 'rectangle'
            call('sketch.begin', {'plane': 'XY', 'units': 'mm', 'name': '60 by 30 rounded blank'})
            call('sketch.lookAt')
            call('sketch.placePoint', pixel(0, 0))
            call('sketch.pointerMove', pixel(20, -10))
            blank = draft()
            preview = call('sketch.twoPointRectanglePreview', {'widthText': '60', 'heightText': '30'})
            assert preview['pendingTwoPointRectangle']['valid'] and draft() == blank
            capture('01-typed-rectangle-preview')
            call('sketch.twoPointRectangleAccept')
            call('sketch.tool', {'tool': 'select'})
            source = draft()
            lines = [e['id'] for e in source['parameters']['entities']]
            dimensions = [d['id'] for d in source['parameters']['dimensions']]
            assert len(lines) == 4 and len(dimensions) == 2
            assert [d['valueMm'] for d in source['parameters']['dimensions']] == [60, 30]
            call('view.zoom', {'factor': .7})
            center = pixel(30, -15)
            viewport = state()['viewport']
            call('view.pan', {'x': viewport['width']/2 - center['x'],
                              'y': viewport['height']/2 - center['y']})
            capture('02-rectangle-60-by-30')
            report['rectangleSketchId'] = source['id']

            stage('6–7', 'Pick each adjacent side pair and apply four R6 sketch fillets')
            for index, pair in enumerate(SIDES):
                before = draft()
                click_pair(pair)
                preview = call('sketch.filletPreview', {'value': 6, 'units': 'mm'})
                assert preview['previewed'] and draft() == before
                result = call('sketch.filletAccept', {'value': 6, 'units': 'mm'})
                assert len(result['createdArcIds']) == 1
                current = draft()
                assert len([e for e in current['parameters']['entities'] if e['type'] == 'arc']) == index+1
                for dimension_id, expected in zip(dimensions, (60, 30)):
                    dim = next(d for d in current['parameters']['dimensions'] if d['id'] == dimension_id)
                    assert not dim['driving'] and 'valueMm' not in dim
                    close(measured_span(current, dim), expected)
                if index in (0, 3): capture(f'03-fillet-{index+1}')
            rounded = draft()
            assert {e['id'] for e in rounded['parameters']['entities'] if e['type'] == 'line'} == set(lines)
            radius_ids = [d['id'] for d in rounded['parameters']['dimensions'] if d['kind'] == 'radius']
            assert len(radius_ids) == 4
            call('sketch.finish')
            source_profile = inspect()['evaluation']['sketches'][0]['profiles'][0]
            close(source_profile['areaMm2'], OUTER_AREA)
            report['roundedSource'] = {'profileId': source_profile['profileId'],
                'referenceDimensionIds': dimensions, 'radiusDimensionIds': radius_ids}

            stage('8', 'Extrude rounded profile 2 mm as New Body')
            call('solid.extrude', {'sketchId': rounded['id'], 'profileId': source_profile['profileId'],
                                  'distance': 2, 'units': 'mm', 'name': 'Keyring floor'})
            base = sole_body()
            close(base['volumeMm3'], BASE_VOLUME)
            body_id = base['bodyId']
            cap = next(face for face in base['supportFaces'] if face['role'] == 'extrude.endCap')
            close(cap['frame']['origin'][2], 2)
            floor_identity = support_identity(cap)
            capture('04-rounded-floor')

            stage('9', 'On actual top cap, select whole rounded boundary; Offset inward 1.5')
            call('sketch.begin', {'support': floor_identity, 'name': 'Inward rounded boundary', 'units': 'mm'})
            call('sketch.tool', {'tool': 'offset'})
            before = draft()
            candidates = [e for e in base['solidEdges'] if e['available'] and
                          e['provenance']['role'] == 'endRim']
            assert candidates, 'Base cap perimeter lacks selectable retained edges'
            point = call('view.projectEdge', {'bodyId': body_id, 'edgeId': candidates[0]['edgeId']})
            call('sketch.placePoint', {'x': point['x'], 'y': point['y']})
            pending = active()['pendingLoopOffset']
            assert pending['valid'] and pending['chain'] and draft() == before
            call('sketch.loopOffsetPreview', {'distance': -1.5, 'units': 'mm', 'chain': True})
            capture('05-inward-loop-preview')
            accepted = call('sketch.loopOffsetAccept')
            annulus = accepted['sketchGeometry']['profiles']
            assert len(annulus) == 1 and annulus[0]['innerLoopCount'] == 1
            close(annulus[0]['areaMm2'], RIM_AREA)
            assert all(math.isclose(r, 4.5, abs_tol=1e-6) for r in annulus[0]['innerArcRadiiMm'])
            for actual, expected in zip(annulus[0]['innerBoundsMm']['size'][:2], (57, 27)):
                close(actual, expected)
            offset = draft()
            offset_id = offset['parameters']['loopOffsets'][0]['id']
            call('sketch.finish')
            report['offset'] = {'sketchId': offset['id'], 'offsetId': offset_id,
                                'profileId': annulus[0]['profileId'], 'areaMm2': annulus[0]['areaMm2']}

            stage('10–12', 'Select annular border and Join it 1.5 mm above the floor')
            call('solid.extrude', {'sketchId': offset['id'], 'profileId': annulus[0]['profileId'],
                                  'distance': 1.5, 'operation': 'join',
                                  'targetBodyIds': [body_id], 'units': 'mm', 'name': 'Raised rim'})
            rim = sole_body()
            close(rim['volumeMm3'], BASE_VOLUME+RIM_VOLUME, 1e-4)
            floor_face(rim, floor_identity)
            capture('06-annular-rim-joined')

            stage('13–15', 'On broad floor, 2-Point Circle: chosen endpoint/direction, typed diameter 4')
            call('sketch.twoPointCircle')
            call('sketch.begin', {'support': floor_identity, 'name': 'Diameter 4 keyring bore', 'units': 'mm'})
            assert active()['tool'] == 'twoPointCircle'
            call('sketch.placePoint', pixel(5, -15))
            call('sketch.pointerMove', pixel(9, -15))
            before = draft()
            call('sketch.circleDiameterPreview', {'text': '4'})
            assert draft() == before
            capture('07-hole-diameter-preview')
            call('sketch.circleDiameterAccept')
            hole = draft()
            circle = hole['parameters']['entities'][0]
            assert circle['type'] == 'circle' and circle['diameterStartPointId']
            close(circle['radiusMm'], 2)
            assert len(hole['parameters']['dimensions']) == 1
            close(hole['parameters']['dimensions'][0]['valueMm'], 4)
            call('sketch.finish')
            hole_profile = inspect()['evaluation']['sketches'][-1]['profiles'][0]
            close(hole_profile['areaMm2'], 4*math.pi)
            report['hole'] = {'sketchId': hole['id'], 'profileId': hole_profile['profileId'],
                              'dimensionId': hole['parameters']['dimensions'][0]['id']}

            stage('16–17', 'Cut the chosen Ø4 region down 10 mm through the 2 mm floor')
            call('solid.extrude', {'sketchId': hole['id'], 'profileId': hole_profile['profileId'],
                                  'distance': -10, 'operation': 'cut',
                                  'targetBodyIds': [body_id], 'units': 'mm', 'name': 'Keyring bore'})
            cut = sole_body()
            close(cut['volumeMm3'], BASE_VOLUME+RIM_VOLUME-HOLE_VOLUME, 1e-4)
            floor_face(cut, floor_identity)
            capture('08-through-bore')

            stage('18', 'Select retained upper circular hole edge; solid Fillet R1')
            hole_edges = [edge for edge in cut['solidEdges'] if edge['available']
                          and edge['provenance']['sourceSketchId'] == hole['id']
                          and edge['provenance']['role'] == 'baseRim'
                          and math.isclose(edge['midpointMm'][2], 2, abs_tol=1e-6)]
            assert len(hole_edges) == 1, 'Upper Cut-hole rim lacks unique retained edge provenance'
            edge_id = hole_edges[0]['edgeId']
            point = call('view.projectEdge', {'bodyId': body_id, 'edgeId': edge_id})
            session = call('solid.filletDialog', {'targetBodyId': body_id})
            picked = call('selection.pick', {'x': point['x'], 'y': point['y']})
            assert picked['edgeId'] == edge_id
            call('command.update', {'session': session['id'], 'inputs': {'radiusMm': 1}})
            preview = call('command.preview', {'session': session['id']})
            assert preview['geometry']['valid']
            capture('09-hole-fillet-preview')
            call('command.commit', {'session': session['id']})
            filleted = sole_body()
            assert filleted['volumeMm3'] < cut['volumeMm3']
            floor_face(filleted, floor_identity)
            report['holeFillet'] = {'selectedEdgeId': edge_id,
                                    'volumeBeforeMm3': cut['volumeMm3'],
                                    'volumeAfterMm3': filleted['volumeMm3']}

            stage('19–20', 'Text on broad floor: DSB Bold, height 14; move and rotate real handles')
            call('sketch.text', {'support': floor_identity, 'name': 'Raised initials', 'units': 'mm'})
            call('sketch.textDialog')
            before = draft()
            call('sketch.textPreview', {'text': 'DSB', 'fontId': 'dejavu-sans-2.37-bold',
                                        'heightMm': 14, 'uMm': 5, 'vMm': -20})
            assert draft() == before
            capture('10-text-controls-preview')
            call('capture.dialog', {'path': str(output / '10-text-dialog.png')})
            text_sketch_id = draft()['id']
            project = lambda u, v: call('view.projectSketchPoint',
                {'sketchId': text_sketch_id, 'uMm': u, 'vMm': v})
            def handle(phase, mode, start, end):
                return call('sketch.textHandle', {'mode': mode, 'phase': phase,
                    'startX': start['x'], 'startY': start['y'],
                    'x': end['x'], 'y': end['y']})
            start, end = project(5, -20), project(55, -5)
            handle('begin', 'move', start, start)
            handle('move', 'move', start, end)
            handle('end', 'move', start, end)
            pending_text = active()['pendingText']
            assert math.isclose(pending_text['uMm'], 55, abs_tol=.25)
            assert math.isclose(pending_text['vMm'], -5, abs_tol=.25)
            arm = max(8, 14*1.35)
            start, end = project(pending_text['uMm']+arm, pending_text['vMm']), project(pending_text['uMm']-arm, pending_text['vMm'])
            handle('begin', 'rotate', start, start)
            handle('move', 'rotate', start, end)
            handle('end', 'rotate', start, end)
            pending_text = active()['pendingText']
            assert math.isclose(abs(pending_text['angleDeg']), 180, abs_tol=1.5)
            assert draft() == before
            capture('11-text-moved-rotated')
            accepted = call('sketch.textAccept')
            assert accepted['textId'] == pending_text['id']
            text_sketch = draft()
            assert text_sketch['parameters']['texts'][0]['id'] == pending_text['id']
            call('sketch.finish')
            text_evaluation = inspect()['evaluation']['sketches'][-1]
            text_bounds = text_evaluation['boundsMm']
            # Keep every glyph right of the R1-rounded bore and inside the
            # straight-sided inner-floor strip; avoid additive-volume overlap.
            assert text_bounds['min'][0] > 10.5 and text_bounds['max'][0] < 54
            assert text_bounds['min'][1] > -28.5 and text_bounds['max'][1] < -1.5
            report['textBoundsMm'] = text_bounds
            text_profiles = text_evaluation['profiles']
            assert len(text_profiles) == 3
            assert [p['innerLoopCount'] for p in text_profiles] == [1, 0, 2]
            text_ids = [p['profileId'] for p in text_profiles]
            report['text'] = {'sketchId': text_sketch['id'], 'textId': pending_text['id'],
                              'fontId': pending_text['fontId'], 'heightMm': pending_text['heightMm'],
                              'angleDeg': pending_text['angleDeg'], 'profileIds': text_ids,
                              'innerLoops': [p['innerLoopCount'] for p in text_profiles]}

            stage('21', 'Select all three text regions, Join 1.5 mm, then Save')
            call('solid.extrude', {'sketchId': text_sketch['id'], 'profileIds': text_ids,
                                  'distance': 1.5, 'operation': 'join',
                                  'targetBodyIds': [body_id], 'units': 'mm', 'name': 'Raised DSB'})
            final = inspect()
            body = sole_body()
            close(body['volumeMm3'], filleted['volumeMm3'] +
                  1.5*sum(p['areaMm2'] for p in text_profiles), 1e-3)
            close(body['boundsMm']['min'][2], 0)
            close(body['boundsMm']['max'][2], 3.5)
            call('view.home')
            capture('12-finished-keyring')
            call('document.save', {'path': str(model)})
            report['sourceAuthored'] = final['authored']
            report['finalBody'] = body
            report['gpu'] = state()['viewport']['gpu']
            if recording: time.sleep(1.0)
            stop_recording()

            stage('Alloy integrity', 'Strict millimeter STL readback; source does not prescribe export')
            exported = call('document.exportStl', {'path': str(mesh), 'bodyId': body_id,
                'units': 'mm', 'format': 'binary',
                'surfaceDeviationMm': .01, 'normalDeviationDeg': 4})
            assert not exported['authoredChanged'] and inspect()['authored'] == final['authored']
            report['export'] = exported
            stage('Alloy integrity', 'Supported same-slot text edit, history, exact reopen')
            call('sketch.edit', {'sketchId': text_sketch['id']})
            call('sketch.textUpdate', {'textId': pending_text['id'], 'text': 'DS8'})
            call('sketch.finish')
            edited = inspect()
            assert edited['evaluation']['bodies'][0]['bodyId'] == body_id
            edited_text = edited['evaluation']['sketches'][-1]['profiles']
            assert [p['profileId'] for p in edited_text] == text_ids
            assert [p['innerLoopCount'] for p in edited_text] == [1, 0, 2]
            assert edited['authored'] != final['authored']
            call('history.undo')
            assert inspect()['authored'] == final['authored']
            call('history.redo')
            assert inspect()['authored'] == edited['authored']
            call('document.open', {'path': str(model), 'discard': True})
            assert inspect()['authored'] == final['authored']
            assert sole_body()['bodyId'] == body_id
            report['integrity'] = {'edit': 'DSB to DS8, same three profile slots',
                                   'undo': 'exact', 'redo': 'exact', 'reopen': 'exact'}
        mesh_readback = inspect_stl(mesh, True)
        assert len(mesh_readback['components']) == 1
        assert mesh_readback['triangles'] == report['export']['triangles']
        assert math.isclose(mesh_readback['signedVolume'], report['finalBody']['volumeMm3'], rel_tol=.003)
        for actual, expected in zip(mesh_readback['bounds'][0], [0, -30, 0]):
            assert math.isclose(actual, expected, abs_tol=.03)
        for actual, expected in zip(mesh_readback['bounds'][1], [60, 0, 3.5]):
            assert math.isclose(actual, expected, abs_tol=.03)
        report['mesh'] = mesh_readback
        report['sha256'] = {path.name: hashlib.sha256(path.read_bytes()).hexdigest()
                            for path in (model, mesh)}
        report['result'] = 'api-pass'
    finally:
        report['transcript'] = transcript
        (output / 'keyring-tutorial-api.json').write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
