#!/usr/bin/env python3
"""From-blank source-shaped shoehorn workflow; current gate ends at guided sheet."""
import argparse
from contextlib import ExitStack
import json
from pathlib import Path
import time
from app_harness import application


def create_sheet(call, stage):
    call('sketch.begin', {'plane': 'YZ', 'name': 'Dimensioned spine'})
    call('sketch.splineAdd', {'points': [{'uMm': 0, 'vMm': 0},
        {'uMm': 50, 'vMm': 16}, {'uMm': 130, 'vMm': 0}]})
    path = call('app.state')['activeSession']['draftSketch']
    spline = next(e for e in path['parameters']['entities'] if e['type'] == 'fitSpline')
    first, middle, last = spline['pointIds']
    # Fixed origin endpoint and endpoint alignment are explicit Alloy anchoring;
    # public source establishes dimensions, not these exact constraint gestures.
    call('sketch.constraint', {'type': 'fixed', 'pointIds': [first]})
    call('sketch.constraint', {'type': 'horizontal', 'pointIds': [first, last]})
    for kind, pair, value, at in [('horizontal', [first, last], 130, (65, -18)),
            ('horizontal', [first, middle], 50, (25, 30)),
            ('vertical', [first, middle], 16, (-12, 8))]:
        call('sketch.dimension', {'kind': kind, 'pointIds': pair, 'value': value,
            'units': 'mm', 'annotation': {'uMm': at[0], 'vMm': at[1]}})
    path = call('app.state')['activeSession']['draftSketch']
    assert len(path['parameters']['dimensions']) == 3
    call('sketch.lookAt')
    stage('YZ spine: driving 130, 50 and 16 mm', '01-dimensioned-spine.png')
    call('sketch.finish')
    planes = [call('construction.pathPlane', {'path': {'sketchId': path['id'],
        'entityId': spline['id'], 'endpoint': end},
        'name': name})['planeId'] for end, name in [(0, 'Front plane'), (1, 'Rear plane')]]
    stage('Normal planes at both path endpoints')
    call('sketch.begin', {'planeId': planes[0], 'name': 'Front arc'})
    call('sketch.addArc', {'mode': 'threePoint', 'startX': -25, 'startY': 10,
        'throughX': 0, 'throughY': 0, 'endX': 25, 'endY': 10, 'units': 'mm'})
    front = call('app.state')['activeSession']['draftSketch']
    arc = next(e for e in front['parameters']['entities'] if e['type'] == 'arc')
    call('sketch.constraint', {'type': 'midpoint', 'entityIds': [arc['id']], 'origin': True})
    origin = call('app.state')['activeSession']['draftSketch']['parameters']['originPointId']
    call('sketch.dimension', {'kind': 'horizontal', 'pointIds': [arc['startPointId'], arc['endPointId']],
        'value': 50, 'units': 'mm', 'annotation': {'uMm': 0, 'vMm': 20}})
    for end, x in [('startPointId', -33), ('endPointId', 33)]:
        call('sketch.dimension', {'kind': 'vertical', 'pointIds': [origin, arc[end]],
            'value': 10, 'units': 'mm', 'annotation': {'uMm': x, 'vMm': 5}})
    call('sketch.lookAt')
    stage('Midpoint arc: 50 mm span, 10 mm endpoint heights', '02-front-arc.png')
    call('sketch.finish')
    call('sketch.begin', {'planeId': planes[1], 'name': 'Rear line'})
    call('sketch.line', {'x1': -15, 'y1': 0, 'x2': 15, 'y2': 0, 'units': 'mm'})
    rear = call('app.state')['activeSession']['draftSketch']
    line = next(e for e in rear['parameters']['entities'] if e['type'] == 'line')
    call('sketch.constraint', {'type': 'midpoint', 'entityIds': [line['id']], 'origin': True})
    call('sketch.constraint', {'type': 'horizontal', 'entityIds': [line['id']]})
    call('sketch.dimension', {'kind': 'linear', 'pointIds': [line['startPointId'], line['endPointId']],
        'value': 30, 'units': 'mm', 'annotation': {'uMm': 0, 'vMm': 10}})
    call('sketch.lookAt')
    stage('Rear line: midpoint at path end, length 30 mm', '03-rear-line.png')
    call('sketch.finish')
    call('view.home')
    session = call('solid.loftDialog', {'operation': 'newComponent', 'componentName': 'Shoehorn'})
    for role, sketch, entity in [('section1', front, arc), ('section2', rear, line),
                               ('centerline', path, spline)]:
        call('solid.loftEntity', {'role': role, 'sketchId': sketch['id'], 'entityId': entity['id']})
    preview = call('command.preview', {'session': session['id']})
    assert preview['geometry']['type'] == 'surface'
    stage('Surface Loft: arc to line along Centerline, New Component', '04-loft-preview.png')
    call('command.commit', {'session': session['id']})
    for feature in [path['id'], front['id'], rear['id'], *planes]:
        call('feature.visibility', {'featureId': feature, 'visible': False})
    call('view.home')
    stage('Editable guided sheet', '05-guided-sheet.png')
    inspection = call('document.inspect')
    sheet = next(b for b in inspection['evaluation']['bodies'] if b['bodyId'] == preview['bodyId'])
    assert sheet['valid'] and sheet['topology']['faces'] == 1 and sheet['topology']['solids'] == 0
    return {'pathId': path['id'], 'splineId': spline['id'],
            'bendDimensionId': path['parameters']['dimensions'][2]['id'], 'planeIds': planes,
            'frontId': front['id'], 'rearId': rear['id'], 'sheet': sheet}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--app', required=True)
    p.add_argument('--output', required=True, type=Path)
    p.add_argument('--record-video', type=Path)
    a = p.parse_args(); out = a.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    with application(a.app, out, command_timeout=120) as (raw, ledger), ExitStack() as cleanup:
        recording = False
        video = {}
        stages = []
        def stop():
            nonlocal recording
            if recording:
                video.update(raw('recording.stop')); recording = False
        cleanup.callback(stop)
        raw('window.resize', {'width': 1400, 'height': 1000})
        if a.record_video:
            movie = a.record_video.resolve()
            assert not movie.is_relative_to(Path(__file__).resolve().parents[1])
            raw('recording.start', {'path': str(movie), 'fps': 5}); recording = True
        def call(command, inputs=None):
            result = raw(command, inputs)
            if recording and command.startswith(('sketch.', 'solid.', 'construction.', 'command.', 'view.')):
                time.sleep(.3)
            return result
        def stage(text, capture=None):
            stages.append(text)
            if recording:
                raw('recording.step', {'text': 'API | ' + text}); time.sleep(1)
            if capture: raw('capture.screenshot', {'path': str(out / capture)})
        result = create_sheet(call, stage)
        before = call('document.inspect')['authored']
        model = out / 'shoehorn-guided-sheet.alloy'
        call('document.save', {'path': str(model)})
        stop()  # Creation footage ends at the authored model; edits are validation.
        call('sketch.edit', {'sketchId': result['pathId']})
        call('sketch.dimensionUpdate', {'dimensionId': result['bendDimensionId'], 'value': 18, 'units': 'mm'})
        call('sketch.finish')
        edited = call('document.inspect')
        changed = next(b for b in edited['evaluation']['bodies'] if b['bodyId'] == result['sheet']['bodyId'])
        assert abs(changed['areaMm2'] - result['sheet']['areaMm2']) > 1
        assert changed['supportFaces'][0]['faceId'] == result['sheet']['supportFaces'][0]['faceId']
        call('history.undo'); assert call('document.inspect')['authored'] == before
        call('history.redo'); assert call('document.inspect')['authored'] == edited['authored']
        variant = out / 'shoehorn-bend-18.alloy'
        call('document.save', {'path': str(variant)})
        call('document.new', {'discard': True})
        call('document.open', {'path': str(variant)})
        assert call('document.inspect')['authored'] == edited['authored']
        (out / 'report.json').write_text(json.dumps({'passed': True, 'scope': 'From blank through guided sheet; solid finishing remains pending',
            'stages': stages, 'result': result, 'video': video,
            'adaptations': ['Fixed first spline endpoint at origin; endpoint horizontal alignment. Public source constraint gestures unspecified.']}, indent=2))
        (out / 'transcript.json').write_text(json.dumps(ledger, indent=2))
    print('PASS: from-blank dimensioned spine, endpoint sections and guided surface')

if __name__ == '__main__':
    main()
