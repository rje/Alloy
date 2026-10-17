#!/usr/bin/env python3
"""Source-informed iron reassessment checkpoints; corrected full closure is guarded.

See docs/v1/iron-reassessment.md. The bridge checkpoint uses explicit retained-section correspondence; the
trimmed-panel checkpoint remains the default visual preparation gate.
Legacy closing-sheet helpers are retained below for redesign, not accepted use.
"""
import argparse
import copy
from contextlib import ExitStack
import json
import math
from pathlib import Path
import time
from app_harness import application


def setup_canvases(call, stage, directory):
    """Display permitted public references at a declared, non-metric scale."""
    references = []
    for kind, plane, view, center in [('side', 'XZ', 'front', (70, 40)),
                                      ('bottom', 'XY', 'top', (70, 0))]:
        path = directory / f'{kind}-reference.jpg'
        call('canvas.set', {'path': str(path), 'planeSource': {'plane': plane},
            'name': f'Public {kind} photograph', 'widthMm': 160, 'opacity': .8,
            'centerUMm': center[0], 'centerVMm': center[1]})
        canvas = call('canvas.inspect')['canvases'][-1]
        call('view.orientation', {'name': view}); call('view.fit')
        stage(f'Public {kind} reference; sketch dimensions are independently chosen', f'00-{kind}-reference.png')
        references.append({'canvasId': canvas['id'], 'path': str(path),
            'displayWidthMm': 160, 'metricCalibration': False,
            'creator': 'Kevin Kennedy / Product Design Online'})
        call('canvas.set', {'canvasId': canvas['id'], 'visible': False})
    call('view.home')
    return references


def create_surfaces(call, stage, checkpoint="trimmed-panel", out=None):
    def draft():
        return call('app.state')['activeSession']['draftSketch']

    def spline(points):
        call('sketch.splineAdd', {'points': [{'uMm': u, 'vMm': v} for u, v in points]})
        return draft()['parameters']['entities'][-1]

    heel = call('construction.plane', {'source': {'plane': 'YZ'}, 'offset': 220,
        'units': 'mm', 'name': 'Heel plane'})['planeId']
    call('sketch.begin', {'planeId': heel, 'name': 'Back arc'})
    call('sketch.addArc', {'mode': 'threePoint', 'startX': -47, 'startY': 0,
        'throughX': 0, 'throughY': 84, 'endX': 47, 'endY': 0, 'units': 'mm'})
    back = draft(); arc = back['parameters']['entities'][-1]
    call('sketch.lookAt'); stage('Back section on the offset heel plane', '01-back.png')
    call('sketch.finish')
    call('sketch.begin', {'plane': 'XY', 'name': 'Lower Mirror pair'})
    call('sketch.line', {'x1': 0, 'y1': 0, 'x2': 220, 'y2': 0, 'units': 'mm'})
    axis = draft()['parameters']['entities'][-1]
    call('sketch.construction', {'entityIds': [axis['id']], 'construction': True})
    right = spline([(220, 47), (140, 50), (70, 36), (0, 0)])
    call('sketch.splineTangent', {'entityId': right['id'], 'pointId': right['pointIds'][-1],
        'duMm': 0, 'dvMm': -200})
    call('sketch.mirrorPreview', {'sourceEntityIds': [right['id']], 'axisEntityId': axis['id']})
    call('sketch.mirrorAccept')
    lower = draft()
    pair = lower['parameters']['mirrorGroups'][0]['entityPairs'][0]
    call('sketch.lookAt'); stage('Mirrored spline outline with a shared tip', '02-outline.png')
    call('sketch.finish')
    call('sketch.begin', {'plane': 'XZ', 'name': 'Crown rail'})
    rail_curve = spline([(220, 84), (120, 92), (50, 68), (0, 0)])
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
    call('view.fit')
    for view, capture in [('front', '04b-exterior-front.png'), ('top', '04c-exterior-top.png'),
                          ('isometric', '04d-exterior-oblique.png'), ('rearIsometric', '04e-exterior-rear.png')]:
        call('view.orientation', {'name': view}); call('view.fit')
        stage('Exterior shape checkpoint: ' + view, capture)
    if checkpoint == 'exterior':
        return {'exteriorId': exterior['bodyId'], 'ownerId': exterior['ownerId'],
                'railId': rail['id'], 'exterior': exterior, 'checkpoint': 'exterior'}
    call('component.activate', {'definitionId': exterior['ownerId']})
    call('sketch.begin', {'plane': 'XZ', 'name': 'Handle side panel'})
    upper_cut = spline([(21.2, 20.6), (46, 57.4), (70.6, 73.6), (122.4, 84.4), (153.4, 85.6)])
    lower_cut = spline([(21.2, 20.6), (113.8, 23.6), (168.4, 35), (168.4, 58.6), (153.4, 85.6)])
    # The unconstrained degree-five fit makes a tiny loop at the low front
    # junction; retain the observed shallow lower run with an explicit handle.
    call('sketch.splineTangent', {'entityId': lower_cut['id'],
        'pointId': lower_cut['pointIds'][0], 'duMm': 200, 'dvMm': 0})
    for endpoint in (0, -1):
        call('sketch.constraint', {'type': 'coincident',
            'pointIds': [upper_cut['pointIds'][endpoint], lower_cut['pointIds'][endpoint]]})
    lens = draft(); call('sketch.lookAt')
    stage('Large asymmetric side panel surrounds a smaller upper-rear grip', '05-panel.png')
    call('sketch.finish'); call('view.home')
    call('sketch.begin', {'plane': 'XZ', 'name': 'Handle middle ellipse'})
    call('sketch.centerPointEllipse', {'centerX': 114, 'centerY': 63, 'firstX': 148.5690919208298,
        'firstY': 68.47520627640808, 'contourX': 111.96635195447699, 'contourY': 75.83994802773679, 'units': 'mm'})
    middle = draft()
    ellipse, = [e for e in middle['parameters']['entities'] if e['type'] == 'ellipse']
    call('sketch.lookAt'); stage('Middle ellipse controls the inner passage', '07-middle.png')
    call('sketch.finish'); call('view.home')
    if out:
        call('document.save', {'path': str(out / 'iron-pretrim-panel.alloy')})
    session = call('surface.trimDialog', {'targetBodyId': exterior['bodyId'],
        'toolSketchId': lens['id'], 'toolEntityIds': [upper_cut['id'], lower_cut['id']],
        'removedRegionIds': [], 'name': 'Handle openings'})
    active_trim = call('app.state')['activeSession']
    if 'regions' not in active_trim:
        stage('Corrected panel Trim refused; preserve the actual preview diagnostic', '06-trim-refused.png')
        raise AssertionError(active_trim.get('previewError', active_trim))
    regions = active_trim['regions']
    inside = [r for r in regions if r['inside']]
    assert len(inside) == 2
    for region in inside:
        call('surface.trimRegion', {'regionId': region['regionId']})
    stage('Trim both side openings', '06-opening-preview.png')
    call('command.commit', {'session': session['id']})
    call('feature.visibility', {'featureId': lens['id'], 'visible': False})
    trimmed, = call('document.inspect')['evaluation']['bodies']
    if checkpoint == 'trimmed-panel':
        call('view.orientation', {'name': 'front'}); call('view.fit')
        stage('Retained broad panel openings before the high-grip Loft', '07b-trimmed-panel-front.png')
        return {'exteriorId': exterior['bodyId'], 'ownerId': exterior['ownerId'],
                'railId': rail['id'], 'exterior': trimmed, 'checkpoint': 'trimmed-panel'}
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
    call('view.home'); call('view.fit')
    stage('Exterior and inner handle surfaces', '09-bridge.png')
    call('view.orientation', {'name': 'front'}); call('view.fit')
    stage('The retained inner surface leaves a through passage', '09b-handle-passage.png')
    for view, capture in [('top', '09c-bridge-top.png'), ('rearIsometric', '09d-bridge-rear.png')]:
        call('view.orientation', {'name': view}); call('view.fit')
        stage('Panel and high grip shape checkpoint: ' + view, capture)
    call('view.home'); call('view.fit')
    bodies = call('document.inspect')['evaluation']['bodies']
    assert len(bodies) == 2 and all(b['valid'] and b['topology']['solids'] == 0 for b in bodies)
    return {'exteriorId': exterior['bodyId'], 'innerId': bodies[-1]['bodyId'],
            'ownerId': exterior['ownerId'], 'railId': rail['id']}


def create_trial_sheets(call, stage):
    """Independent dimensions; the lower middle fit point is on the future bottom."""
    call('sketch.begin', {'plane': 'XZ', 'name': 'Back and notch sheets'})
    call('sketch.splineAdd', {'points': [{'uMm': u, 'vMm': v}
        for u, v in [(82, 20), (102, 38), (135, 60)]]})
    upper = call('app.state')['activeSession']['draftSketch']['parameters']['entities'][-1]
    call('sketch.line', {'x1': 82, 'y1': 20, 'x2': 78, 'y2': 16, 'units': 'mm',
        'startSnapTarget': {'kind': 'point', 'pointId': upper['pointIds'][0]}})
    call('sketch.splineAdd', {'points': [{'uMm': u, 'vMm': v}
        for u, v in [(80, 14), (110, 0), (135, -8)]]})
    sketch = call('app.state')['activeSession']['draftSketch']
    lower = sketch['parameters']['entities'][-1]
    call('sketch.lookAt'); stage('Upper and lower crossing profiles', '10-tool-sketch.png')
    call('sketch.finish'); call('view.home')
    session = call('surface.extrudeDialog', {'sideOneMm': 60, 'sideTwoMm': 65, 'name': 'Closing sheets'})
    call('surface.extrudeEntity', {'sketchId': sketch['id'], 'entityId': upper['id']})
    call('surface.extrudeEntity', {'sketchId': sketch['id'], 'entityId': lower['id']})
    active = call('app.state')['activeSession']
    assert active['previewed'] and len(active['resultBodyIds']) == 2
    assert [len(p['entityIds']) for p in active['inputs']['profiles']] == [2, 1]
    stage('One Two Sides Extrude produces separate tool bodies', '11-tools-preview.png')
    call('command.commit', {'session': session['id']})
    call('feature.visibility', {'featureId': sketch['id'], 'visible': False})
    state = call('document.inspect')
    feature = state['authored']['features'][-1]
    tools = [b for b in state['evaluation']['bodies'] if b['featureId'] == feature['id']]
    assert len(tools) == 2 and len(state['evaluation']['bodies']) == 4
    return {'sketchId': sketch['id'], 'extrudeId': feature['id'], 'tools': tools}


def trim_trials(call, stage, surfaces, sheets):
    """Explicit rear portions selected by source-edge ancestry, then exact undo."""
    before = call('document.inspect')
    target, = [b for b in before['evaluation']['bodies'] if b['bodyId'] == surfaces['innerId']]
    rear, = [e['edgeId'] for e in target['solidEdges'] if e['provenance']['role'] == 'seam.end']
    front, = [e['edgeId'] for e in target['solidEdges'] if e['provenance']['role'] == 'seam.start']
    tools = sorted(sheets['tools'], key=lambda b: b['topology']['faces'], reverse=True)
    for index, tool in enumerate(tools):
        session = call('surface.trimDialog', {'name': ('Upper' if index == 0 else 'Lower') + ' sheet trial'})
        call('surface.trimBody', {'bodyId': tool['bodyId']})
        call('surface.trimTarget', {'bodyId': target['bodyId']})
        regions = call('app.state')['activeSession']['regions']
        selected = [r for r in regions if r['removable'] and rear in r['signature'] and front not in r['signature']]
        assert selected, 'No uniquely identified rear portion in the declared fixture'
        for region in selected:
            call('surface.trimRegion', {'regionId': region['regionId']})
        assert call('app.state')['activeSession']['previewed']
        stage(('Upper' if index == 0 else 'Lower') + ' sheet trims the inner rear portions',
              f'{12+index}-trim-preview.png')
        call('command.commit', {'session': session['id']})
    for tool in tools:
        call('body.remove', {'bodyId': tool['bodyId']})
    assert len(call('document.inspect')['evaluation']['bodies']) == 2
    stage('Remove the two tools while preserving authored history', '14-tools-removed.png')
    for _ in range(4):
        call('history.undo')
    assert call('document.inspect')['authored'] == before['authored']
    stage('Undo Remove and both Trim trials before final closure', '15-trials-reverted.png')


def close_and_export(call, stage, sheets, out):
    authored = call('document.inspect')['authored']
    source = next(f for f in authored['features'] if f['id'] == sheets['sketchId'])
    parameters = copy.deepcopy(source['parameters'])
    upper, connector, lower = parameters['entities'][:3]
    moves = {upper['pointIds'][0]: (100, 20), upper['pointIds'][1]: (110, 38),
             connector['startPointId']: (100, 20), connector['endPointId']: (100, 15),
             lower['pointIds'][0]: (100, 15)}
    for point in parameters['points']:
        if point['id'] in moves:
            point['uMm'], point['vMm'] = moves[point['id']]
    call('feature.edit', {'featureId': source['id'], 'parameters': parameters})
    call('sketch.edit', {'sketchId': sheets['sketchId']})
    call('sketch.constraint', {'type': 'coincident',
        'pointIds': [connector['endPointId'], lower['pointIds'][0]]})
    call('sketch.line', {'x1': -15, 'y1': 0, 'x2': 145, 'y2': 0, 'units': 'mm'})
    sketch = call('app.state')['activeSession']['draftSketch']
    bottom = sketch['parameters']['entities'][-1]['id']
    call('sketch.lookAt'); stage('Move retained closing curves behind the handle and add the bottom line', '16-bottom-line.png')
    call('sketch.finish'); call('view.home')
    call('feature.visibility', {'featureId': sheets['sketchId'], 'visible': True})
    session = call('feature.editDialog', {'featureId': sheets['extrudeId']})
    call('surface.extrudeProfile', {'sketchId': sheets['sketchId'], 'entityIds': [bottom]})
    assert len(call('app.state')['activeSession']['resultBodyIds']) == 3
    stage('Edit the same Extrude to include the bottom sheet', '17-bottom-sheet.png')
    call('command.commit', {'session': session['id']})
    call('feature.visibility', {'featureId': sheets['sketchId'], 'visible': False})
    return fill_and_export(call, stage, out)


def fill_and_export(call, stage, out):
    from stl_export_api import inspect_stl
    before = call('document.inspect'); tools = before['evaluation']['bodies']
    assert len(tools) == 5 and all(b['type'] == 'surface' for b in tools)
    assert len({tool['ownerId'] for tool in tools}) == 1
    call('component.activate', {'definitionId': tools[0]['ownerId']})
    call('document.save', {'path': str(out / 'iron-five-sheet-checkpoint.alloy')})
    session = call('surface.boundaryFillDialog', {'name': 'Iron solid'})
    assert not session['inputs']['removeTools']
    for tool in tools:
        call('surface.boundaryFillTool', {'bodyId': tool['bodyId']})
    state = call('surface.boundaryFillSelectCells')
    cell, = state['cells']
    stage('Boundary Fill: five tools define one bounded cell', '18-available-cell.png')
    call('surface.boundaryFillCell', {'cellId': cell['cellId']})
    call('command.update', {'session': session['id'], 'inputs': {'removeTools': True}})
    active = call('app.state')['activeSession']
    assert active['previewed'] and active['partitionBuildCount'] == 1
    stage('Select the material cell and explicitly enable Remove Tools', '19-selected-cell.png')
    call('command.commit', {'session': session['id']})
    solid, = call('document.inspect')['evaluation']['bodies']
    assert solid['valid'] and solid['closed'] and solid['topology']['solids'] == 1
    call('view.home'); call('view.fit')
    stage('One retained solid from the five surface tools', '20-iron-solid.png')
    call('view.orientation', {'name': 'front'}); call('view.fit')
    stage('The solid retains the open handle passage', '21-solid-handle.png')
    call('view.home'); call('view.fit')
    path = out / 'iron-mm.stl'
    export = call('document.exportStl', {'bodyId': solid['bodyId'], 'path': str(path),
        'units': 'mm', 'format': 'binary', 'surfaceDeviationMm': .05, 'normalDeviationDeg': 8})
    mesh = inspect_stl(path, True)
    assert mesh['eulerCharacteristic'] == 0, 'The final solid must retain one enclosed handle tunnel'
    assert len(mesh['components']) == 1 and math.isclose(mesh['signedVolume'], solid['volumeMm3'], rel_tol=.01)
    stage('Export a checked closed millimeter printing mesh', '22-printable-iron.png')
    return {'solid': solid, 'cell': cell, 'export': export, 'mesh': mesh}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--app', required=True)
    p.add_argument('--output', required=True, type=Path)
    p.add_argument('--record-video', type=Path)
    p.add_argument('--checkpoint', choices=['exterior', 'trimmed-panel', 'bridge'], default='trimmed-panel')
    p.add_argument('--through-trials', action='store_true')
    p.add_argument('--complete', action='store_true')
    p.add_argument('--reference-directory', type=Path)
    a = p.parse_args(); out = a.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    if a.complete or a.through_trials:
        p.error('Reassessment currently qualifies staged exterior/panel/bridge only; corrected closing sheets and trials must pass before --through-trials or --complete')
    if a.complete:
        assert a.reference_directory, 'Complete workflow requires the external public reference directory'
    if a.reference_directory:
        a.reference_directory = a.reference_directory.resolve()
        assert all((a.reference_directory / f'{kind}-reference.jpg').is_file() for kind in ('side', 'bottom'))
    with application(a.app, out, command_timeout=180) as (raw, ledger), ExitStack() as cleanup:
        recording = False; video = {}; stages = []
        def stop():
            nonlocal recording
            if recording:
                video.update(raw('recording.stop')); recording = False
        cleanup.callback(stop)
        raw('window.resize', {'width': 1500, 'height': 950})
        raw('document.new', {'discard': True})
        raw('ui.designTab', {'tab': 'surface'})
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
            references = setup_canvases(call, stage, a.reference_directory) if a.reference_directory else []
            result = create_surfaces(call, stage, a.checkpoint, out)
            result['referenceCanvases'] = references
            result['checkpoint'] = a.checkpoint
            result['adaptations'] = [
                'Source starts with prepared sketches; dimensions and fit points are independently authored.',
                'Central ellipse is authored in a separate sketch before opening Trim; the panel keeps coincident endpoint constraints.',
                'Exterior rail contact uses matching authored coordinates, not retained Project references.',
                'Public photos are perspective references at display scale, not calibrated metric drawings.',
                'Trim targets and removed rear regions are declared fixture choices; full source target inventory is unknown.']
            result['gpu'] = call('app.state')['viewport']['gpu']
            if a.through_trials or a.complete:
                sheets = create_trial_sheets(call, stage)
                trim_trials(call, stage, result, sheets)
            if a.complete:
                call('document.save', {'path': str(out / 'iron-surface-checkpoint.alloy')})
                result['closure'] = close_and_export(call, stage, sheets, out)
            model = out / ('iron-finished.alloy' if a.complete else 'iron-exterior.alloy' if a.checkpoint == 'exterior' else 'iron-trimmed-panel.alloy' if a.checkpoint == 'trimmed-panel' else 'iron-surfaces.alloy')
            call('document.save', {'path': str(model)})
            stop()
            if a.complete:
                original = call('document.inspect')['authored']
                source = next(f for f in original['features'] if f['id'] == sheets['sketchId'])
                parameters = copy.deepcopy(source['parameters'])
                middle = parameters['entities'][0]['pointIds'][1]
                for point in parameters['points']:
                    if point['id'] == middle:
                        point['vMm'] += .5
                call('feature.edit', {'featureId': source['id'], 'parameters': parameters})
                edited = call('document.inspect'); changed, = edited['evaluation']['bodies']
                initial = result['closure']['solid']
                assert changed['valid'] and changed['closed'] and abs(changed['volumeMm3'] - initial['volumeMm3']) > .01
                assert {f['faceId'] for f in changed['supportFaces']} == {f['faceId'] for f in initial['supportFaces']}
                call('document.save', {'path': str(out / 'iron-upper-profile-edited.alloy')})
                call('history.undo'); assert call('document.inspect')['authored'] == original
                call('history.redo'); assert call('document.inspect')['authored'] == edited['authored']
                call('document.open', {'path': str(model), 'discard': True})
                assert call('document.inspect')['authored'] == original
            (out / 'result.json').write_text(json.dumps({'passed': True, 'stages': stages,
                'result': result, 'video': video, 'scope': ('From blank through reversible trials, same-sketch/Extrude bottom addition, selected Boundary Fill cell and checked STL; source edit/history/reopen. Native gesture parity remains qualified.' if a.complete else 'From blank through retained surfaces, separate tools and reversible Trim/Remove trials. Solid closure pending.' if a.through_trials else 'Corrected exterior only; panel/grip and closure pending.' if a.checkpoint == 'exterior' else 'Corrected exterior, large retained bilateral panel Trim and preauthored tilted ellipse; bridge preview is separately crash-blocked.' if a.checkpoint == 'trimmed-panel' else 'Corrected outer and inner surfaces only; closing sheet/trial recipe and final closure unqualified.')}, indent=2))
        finally:
            (out / 'transcript.json').write_text(json.dumps(ledger, indent=2))


if __name__ == '__main__':
    main()
