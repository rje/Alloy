#!/usr/bin/env python3
"""From-blank retained surface iron workflow, using declared independent dimensions."""
import argparse
from contextlib import ExitStack
import json
from pathlib import Path
import time
from app_harness import application


def create_surfaces(call, stage):
    def draft():
        return call('app.state')['activeSession']['draftSketch']

    def spline(points):
        call('sketch.splineAdd', {'points': [{'uMm': u, 'vMm': v} for u, v in points]})
        return draft()['parameters']['entities'][-1]

    heel = call('construction.plane', {'source': {'plane': 'YZ'}, 'offset': 120,
        'units': 'mm', 'name': 'Heel plane'})['planeId']
    call('sketch.begin', {'planeId': heel, 'name': 'Back arc'})
    call('sketch.addArc', {'mode': 'threePoint', 'startX': -35, 'startY': 0,
        'throughX': 0, 'throughY': 45, 'endX': 35, 'endY': 0, 'units': 'mm'})
    back = draft(); arc = back['parameters']['entities'][-1]
    call('sketch.lookAt'); stage('Back section on the offset heel plane', '01-back.png')
    call('sketch.finish')
    call('sketch.begin', {'plane': 'XY', 'name': 'Lower Mirror pair'})
    call('sketch.line', {'x1': 0, 'y1': 0, 'x2': 120, 'y2': 0, 'units': 'mm'})
    axis = draft()['parameters']['entities'][-1]
    call('sketch.construction', {'entityIds': [axis['id']], 'construction': True})
    right = spline([(120, 35), (85, 38), (45, 25), (0, 0)])
    call('sketch.splineTangent', {'entityId': right['id'], 'pointId': right['pointIds'][-1],
        'duMm': 0, 'dvMm': -120})
    call('sketch.mirrorPreview', {'sourceEntityIds': [right['id']], 'axisEntityId': axis['id']})
    call('sketch.mirrorAccept')
    lower = draft()
    pair = lower['parameters']['mirrorGroups'][0]['entityPairs'][0]
    call('sketch.lookAt'); stage('Mirrored spline outline with a shared tip', '02-outline.png')
    call('sketch.finish')
    call('sketch.begin', {'plane': 'XZ', 'name': 'Crown rail'})
    rail_curve = spline([(120, 45), (85, 50), (40, 32), (0, 0)])
    rail = draft(); call('sketch.lookAt')
    stage('Crown rail contacting both exterior sections', '03-rail.png')
    call('sketch.finish'); call('view.home')
    session = call('surface.loftDialog', {'operation': 'newComponent', 'guideType': 'rail',
        'componentName': 'Iron', 'name': 'Outer',
        'sections': [{'sketchId': back['id'], 'entityIds': [arc['id']]},
                     {'sketchId': lower['id'], 'entityIds': [pair['resultEntityId'], right['id']]}],
        'rail': {'sketchId': rail['id'], 'entityId': rail_curve['id']}})
    assert session['previewed'] and session['geometry']['interpolation']['mode'] == 'bilateralInteriorRail'
    stage('Surface Loft: two sections and one retained rail', '04-outer-preview.png')
    call('command.commit', {'session': session['id']})
    exterior, = call('document.inspect')['evaluation']['bodies']
    assert exterior['valid'] and exterior['topology']['faces'] == 2
    for feature in [heel, back['id'], lower['id'], rail['id']]:
        call('feature.visibility', {'featureId': feature, 'visible': False})
    call('component.activate', {'definitionId': exterior['ownerId']})
    call('sketch.begin', {'plane': 'XZ', 'name': 'Handle lens'})
    upper_cut = spline([(30, 15), (60, 30), (90, 15)])
    lower_cut = spline([(30, 15), (60, 10), (90, 15)])
    lens = draft(); call('sketch.lookAt')
    stage('Two spline boundaries define the handle opening', '05-lens.png')
    call('sketch.finish'); call('view.home')
    session = call('surface.trimDialog', {'targetBodyId': exterior['bodyId'],
        'toolSketchId': lens['id'], 'toolEntityIds': [upper_cut['id'], lower_cut['id']],
        'removedRegionIds': [], 'name': 'Handle openings'})
    regions = call('app.state')['activeSession']['regions']
    inside = [r for r in regions if r['inside']]
    assert len(inside) == 2
    for region in inside:
        call('surface.trimRegion', {'regionId': region['regionId']})
    stage('Trim both side openings', '06-opening-preview.png')
    call('command.commit', {'session': session['id']})
    call('feature.visibility', {'featureId': lens['id'], 'visible': False})
    trimmed, = call('document.inspect')['evaluation']['bodies']
    call('sketch.begin', {'plane': 'XZ', 'name': 'Handle middle ellipse'})
    call('sketch.centerPointEllipse', {'centerX': 60, 'centerY': 15, 'firstX': 30,
        'firstY': 15, 'contourX': 60, 'contourY': 27, 'units': 'mm'})
    middle = draft()
    ellipse, = [e for e in middle['parameters']['entities'] if e['type'] == 'ellipse']
    call('sketch.lookAt'); stage('Middle ellipse controls the inner passage', '07-middle.png')
    call('sketch.finish'); call('view.home')
    boundary = []
    for face in trimmed['supportFaces']:
        edge, = [e for e in trimmed['solidEdges'] if e['provenance'].get('role') == 'trimBoundary'
            and e['provenance']['sourceFaceId'] == face['provenance']['sourceFaceId']
            and e['provenance']['sourceEntityId'] == upper_cut['id']]
        boundary.append(edge)
    session = call('surface.loftDialog', {'operation': 'newBody', 'guideType': 'none', 'name': 'Inner'})
    call('solid.loftBoundary', {'role': 'section1', 'bodyId': exterior['bodyId'], 'edgeId': boundary[0]['edgeId']})
    call('solid.loftEntity', {'role': 'section2', 'sketchId': middle['id'], 'entityId': ellipse['id']})
    call('solid.loftBoundary', {'role': 'section3', 'bodyId': exterior['bodyId'], 'edgeId': boundary[1]['edgeId']})
    assert call('app.state')['activeSession']['previewed']
    stage('Loft the retained opening chains through the ellipse', '08-inner-preview.png')
    call('command.commit', {'session': session['id']})
    call('feature.visibility', {'featureId': middle['id'], 'visible': False})
    call('view.home'); stage('Exterior and inner handle surfaces', '09-bridge.png')
    bodies = call('document.inspect')['evaluation']['bodies']
    assert len(bodies) == 2 and all(b['valid'] and b['topology']['solids'] == 0 for b in bodies)
    return {'exteriorId': exterior['bodyId'], 'innerId': bodies[-1]['bodyId'],
            'ownerId': exterior['ownerId'], 'railId': rail['id']}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--app', required=True)
    p.add_argument('--output', required=True, type=Path)
    p.add_argument('--record-video', type=Path)
    a = p.parse_args(); out = a.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    with application(a.app, out, command_timeout=180) as (raw, ledger), ExitStack() as cleanup:
        recording = False; video = {}; stages = []
        def stop():
            nonlocal recording
            if recording:
                video.update(raw('recording.stop')); recording = False
        cleanup.callback(stop)
        raw('window.resize', {'width': 1500, 'height': 950})
        raw('document.new', {'discard': True})
        if a.record_video:
            movie = a.record_video.resolve()
            assert not movie.is_relative_to(Path(__file__).resolve().parents[1])
            raw('recording.start', {'path': str(movie), 'fps': 5}); recording = True
        def call(command, parameters=None):
            result = raw(command, parameters)
            if recording and command.startswith(('sketch.', 'surface.', 'solid.', 'construction.', 'command.')):
                time.sleep(.3)
            return result
        def stage(text, capture=None):
            stages.append(text)
            if recording:
                raw('recording.step', {'text': 'API | ' + text}); time.sleep(1)
            if capture:
                raw('capture.screenshot', {'path': str(out / capture)})
        try:
            result = create_surfaces(call, stage)
            call('document.save', {'path': str(out / 'iron-surfaces.alloy')})
            stop()
            (out / 'result.json').write_text(json.dumps({'passed': True, 'stages': stages,
                'result': result, 'video': video, 'scope': 'From blank through retained outer and inner surfaces. Closing sheets, trial Trim and solid closure pending.'}, indent=2))
        finally:
            (out / 'transcript.json').write_text(json.dumps(ledger, indent=2))


if __name__ == '__main__':
    main()
