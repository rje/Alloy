#!/usr/bin/env python3
"""Source-informed iron reassessment with explicit visual and geometry checkpoints.

See docs/v1/iron-reassessment.md. The bridge checkpoint uses explicit retained-section correspondence; the
trimmed-panel checkpoint remains the default visual preparation gate.
Prepared dimensions are independent source-informed choices, not measured source CAD.
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
    for kind, plane, view, center in [('side', 'XZ', 'front', (100, 47)),
                                      ('bottom', 'XY', 'top', (100, 0))]:
        path = directory / f'{kind}-reference.jpg'
        call('canvas.set', {'path': str(path), 'planeSource': {'plane': plane},
            'name': f'Public {kind} photograph', 'widthMm': 200, 'opacity': .8,
            'centerUMm': center[0], 'centerVMm': center[1]})
        canvas = call('canvas.inspect')['canvases'][-1]
        call('view.orientation', {'name': view}); call('view.fit')
        stage(f'Public {kind} perspective support; prepared landmarks are source-frame-informed', f'00-{kind}-reference.png')
        references.append({'canvasId': canvas['id'], 'path': str(path),
            'displayWidthMm': 200, 'metricCalibration': False,
            'creator': 'Kevin Kennedy / Product Design Online'})
        call('canvas.set', {'canvasId': canvas['id'], 'visible': False})
    call('view.home')
    return references


def create_surfaces(call, stage, checkpoint="trimmed-panel", out=None, tip_weight=200):
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
        'duMm': 0, 'dvMm': -tip_weight})
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
    lower_cut = spline([(21.2, 20.6), (52.0666666667, 21.6), (82.9333333333, 22.6),
        (113.8, 23.6), (168.4, 35), (168.4, 58.6), (153.4, 85.6)])
    # Preserve the source's shallow lower run without altering the interpolator.
    for point, du, dv in [(0, 200, 6.48), (1, 200, 6.48), (2, 200, 6.48), (3, 200, 10)]:
        call('sketch.splineTangent', {'entityId': lower_cut['id'],
            'pointId': lower_cut['pointIds'][point], 'duMm': du, 'dvMm': dv})
    call('sketch.curvatureComb', {'entityId': lower_cut['id'], 'enabled': True,
        'density': 256, 'scale': .01})
    comb, = [c for c in call('app.state')['viewport']['gpu']['splineHandles']['curvatureCombs']
        if c['entityId'] == lower_cut['id']]
    run = [p for p in comb['samples'] if p['uMm'] <= 113.8 + 1e-6]
    lower_quality = {'samples': run, 'sampleCount': len(run),
        'minimumStepX': min(b['uMm']-a['uMm'] for a, b in zip(run, run[1:])),
        'minimumStepZ': min(b['vMm']-a['vMm'] for a, b in zip(run, run[1:])),
        'rangeZ': [min(p['vMm'] for p in run), max(p['vMm'] for p in run)],
        'scope': '256 uniform-parameter samples; lower front A to first low-run landmark, not an analytic monotonicity proof'}
    lower_quality['maximumDrawdownMm'] = max(max(p['vMm'] for p in run[:i+1])-point['vMm']
        for i, point in enumerate(run))
    lower_quality['resolutionAllowanceMm'] = .1
    lower_quality['passed'] = lower_quality['minimumStepX'] > 0 and lower_quality['maximumDrawdownMm'] <= .1
    lower_quality['visualStatus'] = 'candidate for independent front comparison; sub-resolution deviation is unknown, not an exact source match'
    call('sketch.curvatureComb', {'entityId': lower_cut['id'], 'enabled': False})
    if out:
        (out / 'lower-panel-quality.json').write_text(json.dumps(lower_quality, indent=2))
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
    call('view.orientation', {'name': 'front'}); call('view.fit')
    stage('Prepared panel and high tilted ellipse before Trim', '07a-panel-pretrim-front.png')
    call('feature.visibility', {'featureId': exterior['featureId'], 'visible': False})
    call('view.fit'); stage('Unobscured prepared source panel and high ellipse', '07b-isolated-panel.png')
    call('feature.visibility', {'featureId': exterior['featureId'], 'visible': True})
    assert lower_quality['passed'], {'lowerPanelShapeGate': {k: v for k, v in lower_quality.items() if k != 'samples'}}
    if checkpoint == 'panel':
        return {'exteriorId': exterior['bodyId'], 'ownerId': exterior['ownerId'],
            'railId': rail['id'], 'panelId': lens['id'], 'middleId': middle['id'],
            'lowerPanelQuality': lower_quality, 'checkpoint': 'panel'}
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
    call('feature.visibility', {'featureId': trimmed['featureId'], 'visible': False})
    call('view.orientation', {'name': 'front'}); call('view.fit')
    stage('Isolated corrected shallow panel and high-grip bridge', '09e-isolated-bridge-front.png')
    call('feature.visibility', {'featureId': trimmed['featureId'], 'visible': True})
    call('view.home'); call('view.fit')
    bodies = call('document.inspect')['evaluation']['bodies']
    assert len(bodies) == 2 and all(b['valid'] and b['topology']['solids'] == 0 for b in bodies)
    return {'exteriorId': exterior['bodyId'], 'innerId': bodies[-1]['bodyId'],
            'ownerId': exterior['ownerId'], 'railId': rail['id'], 'panelId': lens['id'], 'lowerPanelQuality': lower_quality}


def resume_surfaces(call, source, connected_trial=False):
    """Resume a reviewed bridge checkpoint; never describe this as from blank."""
    call('document.open', {'path': str(source.resolve()), 'discard': True})
    state = call('document.inspect'); bodies = state['evaluation']['bodies']
    assert len(bodies) in ((2, 3, 4) if connected_trial else (2,)) and all(b['valid'] and b['type'] == 'surface' for b in bodies)
    exterior, = [b for b in bodies if b['name'] == 'Outer']
    inner, = [b for b in bodies if b['name'] == 'Inner']
    assert exterior['ownerId'] == inner['ownerId']
    call('component.activate', {'definitionId': exterior['ownerId']})
    rail, = [f for f in state['authored']['features'] if f['name'] == 'Crown rail']
    panel, = [f for f in state['authored']['features'] if f['name'] == 'Handle side panel']
    return {'exteriorId': exterior['bodyId'], 'innerId': inner['bodyId'],
        'ownerId': exterior['ownerId'], 'railId': rail['id'], 'panelId': panel['id'],
        'resumedFrom': str(source.resolve()), 'sourceFeatures': copy.deepcopy(state['authored']['features'])}


def create_trial_sheets(call, stage, surfaces, out, connected=False):
    """Fixed source-informed heel/notch/underside network, before any trials."""
    before = call('document.inspect')
    source_bodies = [b for b in before['evaluation']['bodies']
        if b['bodyId'] in (surfaces['exteriorId'], surfaces['innerId'])]
    reach = math.ceil(max(abs(b['boundsMm'][side][1]) for b in source_bodies
        for side in ('min', 'max')) + 10)
    call('sketch.begin', {'plane': 'XZ', 'name': 'Back and notch sheets'})
    call('sketch.splineAdd', {'points': [{'uMm': u, 'vMm': v}
        for u, v in [(202, 14), (192, 40), (180, 74), (176, 110)]]})
    upper = call('app.state')['activeSession']['draftSketch']['parameters']['entities'][-1]
    call('sketch.line', {'x1': 202, 'y1': 14, 'x2': 198, 'y2': 14, 'units': 'mm',
        'startSnapTarget': {'kind': 'point', 'pointId': upper['pointIds'][0]}})
    connector = call('app.state')['activeSession']['draftSketch']['parameters']['entities'][-1]
    call('sketch.splineAdd', {'points': [{'uMm': u, 'vMm': v}
        for u, v in [(198, 14), (168, 8), (140, -6)]]})
    lower = call('app.state')['activeSession']['draftSketch']['parameters']['entities'][-1]
    call('sketch.constraint', {'type': 'coincident',
        'pointIds': [connector['endPointId'], lower['pointIds'][0]]})
    sketch = call('app.state')['activeSession']['draftSketch']
    call('sketch.lookAt'); stage('Fixed sloping heel, low notch and shallow underside', '10-tool-sketch.png')
    call('sketch.finish'); call('view.home')
    call('document.save', {'path': str(out / 'iron-fixed-closing-sketch.alloy')})
    session = call('surface.extrudeDialog', {'sideOneMm': reach, 'sideTwoMm': reach, 'name': 'Closing sheets'})
    # Explicit source memberships preserve the demonstrated two separately
    # selectable tools even though their authored endpoint is coincident.
    if connected:
        assert reach == 70, 'Candidate09 preserves the accepted ±70mm reach'
        call('surface.extrudeProfile', {'sketchId': sketch['id'],
            'entityIds': [upper['id'], connector['id'], lower['id']]})
    else:
        call('surface.extrudeProfile', {'sketchId': sketch['id'], 'entityIds': [upper['id'], connector['id']]})
        call('surface.extrudeProfile', {'sketchId': sketch['id'], 'entityIds': [lower['id']]})
    active = call('app.state')['activeSession']
    assert active['previewed'] and len(active['resultBodyIds']) == (1 if connected else 2)
    assert [len(p['entityIds']) for p in active['inputs']['profiles']] == ([3] if connected else [2, 1])
    stage('Connected three-face sheet; public separate-tool parity unknown' if connected else 'Two Sides overrun actual lateral bounds; separate upper and lower tools', '11-tools-preview.png')
    call('command.commit', {'session': session['id']})
    call('feature.visibility', {'featureId': sketch['id'], 'visible': False})
    state = call('document.inspect'); feature = state['authored']['features'][-1]
    tools = [b for b in state['evaluation']['bodies'] if b['featureId'] == feature['id']]
    assert len(tools) == (1 if connected else 2) and len(state['evaluation']['bodies']) == (3 if connected else 4)
    if connected:
        assert tools[0]['valid'] and tools[0]['topology']['faces'] == 3
    call('document.save', {'path': str(out / 'iron-closing-sheets-checkpoint.alloy')})
    for view, capture in [('front', '11b-tools-front.png'), ('isometric', '11c-tools-oblique.png')]:
        call('view.orientation', {'name': view}); call('view.fit')
        stage('Closing sheet relation to the preserved high grip: ' + view, capture)
    return {'sketchId': sketch['id'], 'extrudeId': feature['id'], 'tools': tools,
        'sourceParameters': copy.deepcopy(sketch['parameters']), 'reachEachSideMm': reach}



def connected_trial_sheets(call, stage, surfaces, out):
    """Candidate09 adaptation: same finite network, one connected profile."""
    source = surfaces['sourceFeatures']
    assert len(source) in (9, 11), 'Use candidate07 bridge or candidate08 sheet checkpoint'
    if len(source) == 9:
        sheets = create_trial_sheets(call, stage, surfaces, out, connected=True)
    else:
        sketch, extrude = source[-2:]
        assert sketch['name'] == 'Back and notch sheets' and extrude['type'] == 'surfaceExtrude'
        points = [(p['uMm'], p['vMm']) for p in sketch['parameters']['points']]
        assert points == [(202,14),(192,40),(180,74),(176,110),(202,14),(198,14),(198,14),(168,8),(140,-6)]
        parameters = copy.deepcopy(extrude['parameters'])
        assert parameters['sideOneMm'] == parameters['sideTwoMm'] == 70
        assert [len(p['entityIds']) for p in parameters['profiles']] in ([2, 1], [3])
        connected_profiles = [{'entityIds': [e['id'] for e in sketch['parameters']['entities']]}]
        if parameters['profiles'] != connected_profiles:
            parameters['profiles'] = connected_profiles
            call('feature.edit', {'featureId': extrude['id'], 'parameters': parameters})
        state = call('document.inspect')
        tools = [b for b in state['evaluation']['bodies'] if b['featureId'] == extrude['id']]
        assert len(tools) == 1 and tools[0]['valid'] and tools[0]['topology']['faces'] == 3
        assert state['authored']['features'][-2] == sketch
        sheets = {'sketchId': sketch['id'], 'extrudeId': extrude['id'], 'tools': tools,
            'sourceParameters': copy.deepcopy(sketch['parameters']), 'reachEachSideMm': 70}
    state = call('document.inspect')
    assert state['authored']['features'][:9] == source[:9], 'All nine accepted features must remain exact'
    call('document.save', {'path': str(out / 'iron-connected-sheet-checkpoint.alloy')})
    for view in ('front', 'rearIsometric'):
        call('view.orientation', {'name': view}); call('view.fit')
        stage('Candidate09 connected-tool adaptation: ' + view, 'connected-tool-' + view + '.png')
    return sheets


def connected_trim_trial(call, stage, surfaces, sheets, out, commit_undo=False):
    """Select bilateral rear portions only by unambiguous boundary ancestry."""
    before = call('document.inspect')
    target, = [b for b in before['evaluation']['bodies'] if b['bodyId'] == surfaces['exteriorId']]
    tool, = sheets['tools']
    rear = [e['edgeId'] for e in target['solidEdges'] if e['provenance'].get('role') == 'back']
    panel = [e['edgeId'] for e in target['solidEdges']
        if e['provenance'].get('sourceSketchId') == surfaces['panelId']]
    assert len(rear) == 2 and panel

    def preview():
        session = call('surface.trimDialog', {'name': 'Connected sheet trial'})
        call('surface.trimBody', {'bodyId': tool['bodyId']})
        call('surface.trimTarget', {'bodyId': target['bodyId']})
        active = call('app.state')['activeSession']
        inventory = {'adaptation': 'One connected three-face profile replaces public separate tools; parity unknown',
            'targetBodyId': target['bodyId'], 'toolBodyId': tool['bodyId'],
            'targetBoundsMm': target['boundsMm'], 'toolBoundsMm': tool['boundsMm'],
            'rearEdgeIds': rear, 'panelEdgeIds': panel, 'regions': active.get('regions', []),
            'previewError': active.get('previewError')}
        (out / 'connected-trial-region-inventory.json').write_text(json.dumps(inventory, indent=2))
        if not inventory['regions']:
            stage('Connected sheet refused for actual Outer; stop', 'connected-trial-refused.png')
            call('command.cancel', {'session': session['id']})
            after = call('document.inspect')
            assert after['authored'] == before['authored'] and after['evaluation']['bodies'] == before['evaluation']['bodies']
            inventory['cancelExact'] = True
            (out / 'connected-trial-result.json').write_text(json.dumps(inventory, indent=2))
            raise AssertionError(inventory)
        regions = inventory['regions']
        source_ids = {r['sourceFaceId'] for r in regions}
        selected = []
        def refuse_ambiguous(message):
            stage(message, 'connected-trial-ambiguous.png')
            call('command.cancel', {'session': session['id']})
            after = call('document.inspect')
            assert after['authored'] == before['authored'] and after['evaluation']['bodies'] == before['evaluation']['bodies']
            inventory.update(cancelExact=True, ambiguity=message)
            (out / 'connected-trial-result.json').write_text(json.dumps(inventory, indent=2))
            raise AssertionError(message)
        if len(source_ids) != 2 or len(regions) != 4:
            refuse_ambiguous('Ambiguous bilateral partition; stop without guessing')
        for source_id in source_ids:
            portions = [r for r in regions if r['sourceFaceId'] == source_id]
            back = [r for r in portions if r['removable'] and any(e in r['signature'] for e in rear)
                and not any(e in r['signature'] for e in panel)]
            front = [r for r in portions if r['removable'] and any(e in r['signature'] for e in panel)
                and not any(e in r['signature'] for e in rear)]
            if len(back) != 1 or len(front) != 1:
                refuse_ambiguous('Cannot identify front/rear by retained ancestry; stop')
            selected.append(back[0]['regionId'])
        for region in selected:
            call('surface.trimRegion', {'regionId': region})
        active = call('app.state')['activeSession']
        assert active['previewed'], active.get('previewError')
        inventory.update(selectedRegionIds=selected, previewed=True,
            selectedPreviewError=active.get('previewError'), previewGeometry=active.get('geometry'))
        for view in ('front', 'rearIsometric'):
            call('view.orientation', {'name': view}); call('view.fit')
            stage('Bilateral rear excess selected by boundary ancestry: ' + view, 'connected-trim-' + view + '.png')
        return session, inventory

    session, inventory = preview()
    call('command.cancel', {'session': session['id']})
    after = call('document.inspect')
    assert after['authored'] == before['authored'] and after['evaluation']['bodies'] == before['evaluation']['bodies']
    inventory['cancelExact'] = True
    if commit_undo:
        session, committed_inventory = preview()
        assert {r['signature'] for r in committed_inventory['regions']} == {r['signature'] for r in inventory['regions']}
        inventory['committedSelectedRegionIds'] = committed_inventory['selectedRegionIds']
        call('command.commit', {'session': session['id']})
        committed = call('document.inspect')
        retained, = [b for b in committed['evaluation']['bodies'] if b['bodyId'] == target['bodyId']]
        assert retained['valid'] and retained['boundsMm']['max'][0] < target['boundsMm']['max'][0]
        assert [f for f in committed['authored']['features'] if f['id'] in {f['id'] for f in before['authored']['features']}] == before['authored']['features']
        inner_before, = [b for b in before['evaluation']['bodies'] if b['bodyId'] == surfaces['innerId']]
        inner_after, = [b for b in committed['evaluation']['bodies'] if b['bodyId'] == surfaces['innerId']]
        assert inner_after == inner_before, 'Inner geometry must remain exact'
        retained_panel = [e['edgeId'] for e in retained['solidEdges']
            if e['provenance'].get('sourceSketchId') == surfaces['panelId']]
        assert set(retained_panel) == set(panel), 'All original panel boundary IDs must survive'
        assert len(committed['evaluation']['bodies']) == 3 and retained['topology']['faces'] == 2
        call('document.save', {'path': str(out / 'iron-connected-trim-trial.alloy')})
        call('feature.visibility', {'featureId': sheets['extrudeId'], 'visible': False})
        for view in ('front', 'rearIsometric'):
            call('view.orientation', {'name': view}); call('view.fit')
            stage('Committed bilateral Trim; tool hidden: ' + view, 'committed-clean-' + view + '.png')
        call('document.save', {'path': str(out / 'iron-connected-trim-clean.alloy')})
        call('history.undo')  # undo only the capture visibility transaction
        restored = call('document.inspect')
        assert restored['authored'] == committed['authored'] and restored['evaluation']['bodies'] == committed['evaluation']['bodies']
        call('history.undo')  # undo the sole Trim commit
        inventory.update(innerExact=True, panelBoundaryIdsExact=True,
            committedTarget=retained, committedToolHiddenCapture=True)
        after = call('document.inspect')
        assert after['authored'] == before['authored'] and after['evaluation']['bodies'] == before['evaluation']['bodies']
        inventory['commitUndoExact'] = True
    (out / 'connected-trial-result.json').write_text(json.dumps(inventory, indent=2))
    return inventory


def trim_trials(call, stage, surfaces, sheets, out):
    """Inspect actual Outer rear portions, preserve tools, then exact trial undo."""
    before = call('document.inspect'); trials = []
    tools = sorted(sheets['tools'], key=lambda b: b['topology']['faces'], reverse=True)
    for index, tool in enumerate(tools):
        label = 'Upper' if index == 0 else 'Lower'
        current = call('document.inspect')
        target, = [b for b in current['evaluation']['bodies'] if b['bodyId'] == surfaces['exteriorId']]
        rear = [e['edgeId'] for e in target['solidEdges'] if e['provenance'].get('role') == 'back']
        panel = [e['edgeId'] for e in target['solidEdges']
            if e['provenance'].get('sourceSketchId') == surfaces['panelId']]
        session = call('surface.trimDialog', {'name': label + ' sheet trial'})
        call('surface.trimBody', {'bodyId': tool['bodyId']})
        call('surface.trimTarget', {'bodyId': target['bodyId']})
        active = call('app.state')['activeSession']
        inventory = {'targetBodyId': target['bodyId'], 'targetName': target['name'],
            'targetBoundsMm': target['boundsMm'], 'toolBodyId': tool['bodyId'],
            'toolBoundsMm': tool['boundsMm'], 'rearEdgeIds': rear, 'panelEdgeIds': panel,
            'regions': active.get('regions', []), 'previewError': active.get('previewError')}
        (out / f'{label.lower()}-trial-region-inventory.json').write_text(json.dumps(inventory, indent=2))
        if not active.get('regions'):
            stage(label + ' trial refused for actual Outer target; retain the diagnostic', f'{12+index}-trial-refused.png')
            raise AssertionError(inventory)
        selected = [r for r in active['regions'] if r['removable']
            and any(edge in r['signature'] for edge in rear)
            and not any(edge in r['signature'] for edge in panel)]
        assert selected, 'No uniquely ancestry-identified rear excess outside the retained panel'
        for region in selected:
            call('surface.trimRegion', {'regionId': region['regionId']})
        active = call('app.state')['activeSession']; assert active['previewed'], active.get('previewError')
        stage(label + ' actual Outer rear excess, selected by retained boundary ancestry', f'{12+index}-trim-preview.png')
        call('command.commit', {'session': session['id']}); trials.append(inventory)
        call('document.save', {'path': str(out / f'iron-{label.lower()}-trial.alloy')})
    for tool in tools:
        call('body.remove', {'bodyId': tool['bodyId']})
    assert len(call('document.inspect')['evaluation']['bodies']) == 2
    stage('Remove both tools while preserving authored history', '14-tools-removed.png')
    for _ in range(len(trials) + len(tools)):
        call('history.undo')
    assert call('document.inspect')['authored'] == before['authored']
    stage('Undo exactly the Remove and Trim trials; fixed cutters are unchanged', '15-trials-reverted.png')
    return trials


def add_bottom_sheet(call, stage, sheets, out, connected=False):
    """Add only the bottom line to the existing sketch and the same Extrude."""
    call('sketch.edit', {'sketchId': sheets['sketchId']})
    call('sketch.line', {'x1': -20, 'y1': 0, 'x2': 240, 'y2': 0, 'units': 'mm'})
    sketch = call('app.state')['activeSession']['draftSketch']
    bottom = sketch['parameters']['entities'][-1]['id']
    for group in ('entities', 'points', 'constraints'):
        after = {row['id']: row for row in sketch['parameters'].get(group, [])}
        assert all(after[row['id']] == row for row in sheets['sourceParameters'].get(group, [])), 'Closing source changed after trials'
    call('sketch.lookAt'); stage('Add only the bottom line; preserve the original heel and notch', '16-bottom-line.png')
    call('sketch.finish'); call('view.home')
    call('feature.visibility', {'featureId': sheets['sketchId'], 'visible': True})
    session = call('feature.editDialog', {'featureId': sheets['extrudeId']})
    call('surface.extrudeProfile', {'sketchId': sheets['sketchId'], 'entityIds': [bottom]})
    active = call('app.state')['activeSession']
    assert active['previewed'] and len(active['resultBodyIds']) == (2 if connected else 3)
    if connected:
        assert [len(p['entityIds']) for p in active['inputs']['profiles']] == [3, 1]
        assert active['inputs']['sideOneMm'] == active['inputs']['sideTwoMm'] == 70
    stage('Edit the same Extrude to include the bottom sheet', '17-bottom-sheet.png')
    call('command.commit', {'session': session['id']})
    call('feature.visibility', {'featureId': sheets['sketchId'], 'visible': False})
    state = call('document.inspect')
    assert len(state['evaluation']['bodies']) == (4 if connected else 5)
    call('document.save', {'path': str(out / ('iron-four-tool-prefill.alloy' if connected else 'iron-five-sheet-checkpoint.alloy'))})
    for view in (('front', 'rearIsometric') if connected else ('front',)):
        call('view.orientation', {'name': view}); call('view.fit')
        stage('Four fixed tools before Boundary Fill' if connected else 'Five fixed closing tools before selecting any Boundary Fill cell', 'prefill-' + view + '.png')



def connected_fill_preview(call, stage, surfaces, sheets, out, commit_solid=False):
    """Inventory one bounded cell before any closure commit; preserve authored tools."""
    before = call('document.inspect')
    assert before['authored']['features'][:9] == surfaces['sourceFeatures'][:9]
    tools = before['evaluation']['bodies']
    assert len(tools) == 4 and all(b['type'] == 'surface' and b['valid'] for b in tools)
    assert {b['name'] for b in tools} >= {'Outer', 'Inner'}
    extruded = [b for b in tools if b['featureId'] == sheets['extrudeId']]
    assert sorted(b['topology']['faces'] for b in extruded) == [1, 3]
    session = call('surface.boundaryFillDialog', {'name': 'Connected closure candidate'})
    for tool in tools:
        call('surface.boundaryFillTool', {'bodyId': tool['bodyId']})
    state = call('surface.boundaryFillSelectCells')
    cells = state.get('cells', [])
    inventory = {'adaptation': 'Four tools: Outer, Inner, connected heel/notch sheet, bottom; public separate-tool parity unknown',
        'tools': tools, 'cells': cells, 'previewError': state.get('previewError')}
    (out / 'boundary-fill-cell-inventory.json').write_text(json.dumps(inventory, indent=2))
    stage('Inventory all bounded cells from the four fixed tools', 'boundary-fill-cell-inventory.png')
    if len(cells) != 1:
        call('command.cancel', {'session': session['id']})
        after = call('document.inspect')
        assert after['authored'] == before['authored'] and after['evaluation']['bodies'] == before['evaluation']['bodies']
        inventory['cancelExact'] = True
        (out / 'boundary-fill-result.json').write_text(json.dumps(inventory, indent=2))
        raise AssertionError('Expected exactly one intended handle-bearing cell; stop without choosing a different cell')
    cell, = cells
    assert cell['geometry']['valid'] and cell['geometry']['closed'] and cell['geometry']['topology']['solids'] == 1
    call('surface.boundaryFillCell', {'cellId': cell['cellId']})
    call('command.update', {'session': session['id'], 'inputs': {'removeTools': True}})
    active = call('app.state')['activeSession']
    assert active['previewed'] and active['partitionBuildCount'] == 1, active.get('previewError')
    inventory.update(selectedCellId=cell['cellId'], selectedGeometry=cell['geometry'], previewed=True,
        selectedPreviewError=active.get('previewError'), partitionBuildCount=active['partitionBuildCount'],
        diagnostics=active.get('geometry', {}).get('diagnostics', {}))
    for view in ('front', 'isometric', 'rearIsometric'):
        call('view.orientation', {'name': view}); call('view.fit')
        stage('Selected connected closure cell; inspect handle and silhouette: ' + view, 'selected-cell-' + view + '.png')
    if commit_solid:
        inventory['committed'] = connected_solid_commit(call, stage, before, session, out)
    else:
        call('command.cancel', {'session': session['id']})
        after = call('document.inspect')
        assert after['authored'] == before['authored'] and after['evaluation']['bodies'] == before['evaluation']['bodies']
        inventory['cancelExact'] = True
    (out / 'boundary-fill-result.json').write_text(json.dumps(inventory, indent=2))
    return inventory



def connected_solid_commit(call, stage, before, session, out):
    """One accepted-cell commit, exact history/native reopen, independent export readback."""
    call('command.commit', {'session': session['id']})
    return qualify_committed_solid(call, stage, before, out)


def qualify_committed_solid(call, stage, before, out, source_change=False):
    from stl_export_api import inspect_stl
    committed = call('document.inspect')
    solid, = committed['evaluation']['bodies']
    assert solid['valid'] and solid['closed'] and solid['topology']['solids'] == 1
    if not source_change:
        assert committed['authored']['features'][:-1] == before['authored']['features']
    for view in ('front', 'top', 'isometric', 'rearIsometric', 'bottom'):
        call('view.orientation', {'name': view}); call('view.fit')
        stage('Committed connected closure, clean geometry: ' + view, 'solid-' + view + '.png')
    model = out / 'iron-committed-solid.alloy'
    call('document.save', {'path': str(model)})
    call('history.undo')
    undone = call('document.inspect')
    assert undone['authored'] == before['authored'] and undone['evaluation']['bodies'] == before['evaluation']['bodies']
    call('history.redo')
    redone = call('document.inspect')
    assert redone['authored'] == committed['authored'] and redone['evaluation']['bodies'] == committed['evaluation']['bodies']
    call('document.open', {'path': str(model), 'discard': True})
    reopened = call('document.inspect')
    assert reopened['authored'] == committed['authored'] and reopened['evaluation']['bodies'] == committed['evaluation']['bodies']
    meshpath = out / 'iron-mm.stl'
    export = call('document.exportStl', {'bodyId': solid['bodyId'], 'path': str(meshpath),
        'units': 'mm', 'format': 'binary', 'surfaceDeviationMm': .05, 'normalDeviationDeg': 8})
    mesh = inspect_stl(meshpath, True)
    assert len(mesh['components']) == 1 and mesh['eulerCharacteristic'] == 0
    assert math.isclose(mesh['signedVolume'], solid['volumeMm3'], rel_tol=.01)
    step = call('document.exportStep', {'bodyId': solid['bodyId'], 'path': str(out / 'iron-mm.step'), 'units': 'mm'})
    result = {'solid': solid, 'undoRedoAuthoredAndBodiesExact': True, 'nativeReopenAuthoredAndBodiesExact': True,
        'exportStl': export, 'mesh': mesh, 'exportStep': step,
        'witnesses': {'status': 'pending independent BRep qualification', 'rearWebMaterialMm': [180, 0, 45],
            'apertureRayFromMm': [114, -100, 63], 'apertureRayToMm': [114, 100, 63]}}
    (out / 'committed-solid-result.json').write_text(json.dumps(result, indent=2))
    return result


def fill_and_export(call, stage, out, connected=False, accepted_solid=None):
    from stl_export_api import inspect_stl
    before = call('document.inspect'); tools = before['evaluation']['bodies']
    assert len(tools) == (4 if connected else 5) and all(b['type'] == 'surface' for b in tools)
    assert len({tool['ownerId'] for tool in tools}) == 1
    call('component.activate', {'definitionId': tools[0]['ownerId']})
    call('document.save', {'path': str(out / ('iron-four-tool-prefill.alloy' if connected else 'iron-five-sheet-checkpoint.alloy'))})
    session = call('surface.boundaryFillDialog', {'name': 'Connected closure candidate' if connected else 'Iron solid'})
    assert not session['inputs']['removeTools']
    for tool in tools:
        call('surface.boundaryFillTool', {'bodyId': tool['bodyId']})
    state = call('surface.boundaryFillSelectCells')
    (out / 'boundary-fill-cell-inventory.json').write_text(json.dumps(state.get('cells', []), indent=2))
    stage('Inspect every actual bounded cell from the four fixed tools' if connected else 'Inspect every actual bounded cell from the five fixed tools', '18-available-cell.png')
    assert len(state.get('cells', [])) == 1, 'Corrected source needs independent inspection of its available cells; do not choose by screen position'
    cell, = state['cells']
    call('surface.boundaryFillCell', {'cellId': cell['cellId']})
    call('command.update', {'session': session['id'], 'inputs': {'removeTools': True}})
    active = call('app.state')['activeSession']
    assert active['previewed'] and active['partitionBuildCount'] == 1
    stage('Select the material cell and explicitly enable Remove Tools', '19-selected-cell.png')
    call('command.commit', {'session': session['id']})
    solid, = call('document.inspect')['evaluation']['bodies']
    assert solid['valid'] and solid['closed'] and solid['topology']['solids'] == 1
    if accepted_solid:
        assert math.isclose(solid['volumeMm3'], accepted_solid['volumeMm3'], rel_tol=1e-7)
        assert math.isclose(solid['areaMm2'], accepted_solid['areaMm2'], rel_tol=1e-7)
        for side in ('min', 'max'):
            assert all(math.isclose(a,b,abs_tol=1e-6) for a,b in zip(solid['boundsMm'][side], accepted_solid['boundsMm'][side]))
    call('view.home'); call('view.fit')
    stage('One retained solid from four fixed tools' if connected else 'One retained solid from the five surface tools', '20-iron-solid.png')
    call('view.orientation', {'name': 'front'}); call('view.fit')
    stage('The solid retains the open handle passage', '21-solid-handle.png')
    if connected:
        for view in ('top', 'isometric', 'rearIsometric', 'bottom'):
            call('view.orientation', {'name': view}); call('view.fit')
            stage('Accepted pointed-tip solid: ' + view, '21-solid-' + view + '.png')
    call('view.home'); call('view.fit')
    path = out / 'iron-mm.stl'
    export = call('document.exportStl', {'bodyId': solid['bodyId'], 'path': str(path),
        'units': 'mm', 'format': 'binary', 'surfaceDeviationMm': .05, 'normalDeviationDeg': 8})
    mesh = inspect_stl(path, True)
    assert mesh['eulerCharacteristic'] == 0, 'The final solid must retain one enclosed handle tunnel'
    assert len(mesh['components']) == 1 and math.isclose(mesh['signedVolume'], solid['volumeMm3'], rel_tol=.01)
    stage('Export a checked closed millimeter printing mesh', '22-printable-iron.png')
    return {'solid': solid, 'cell': cell, 'export': export, 'mesh': mesh}



def nose_weight_admission(call, stage, source, out):
    """One bounded source-tip hypothesis: 200→40, no point or other feature edits."""
    native = json.loads(source.resolve().read_text())
    assert len(native['features']) == 12 and native['features'][-1]['type'] == 'boundaryFill'
    prefix = copy.deepcopy(native); prefix['features'] = prefix['features'][:5]
    prefixpath = out / 'source-exterior-prefix.alloy'
    prefixpath.write_text(json.dumps(prefix, indent=2))
    call('document.open', {'path': str(prefixpath), 'discard': True})
    before = call('document.inspect')
    lower, = [f for f in before['authored']['features'] if f['name'] == 'Lower Mirror pair']
    group, = lower['parameters']['mirrorGroups']; pair, = group['entityPairs']
    entities = {e['id']: e for e in lower['parameters']['entities']}
    original = entities[pair['sourceEntityId']]; reflected = entities[pair['resultEntityId']]
    tip = original['pointIds'][-1]
    assert reflected['pointIds'][-1] == tip
    assert original['tangentHandles'][tip] == {'duMm': 0, 'dvMm': -200}
    call('sketch.edit', {'sketchId': lower['id']})
    call('sketch.splineTangent', {'entityId': original['id'], 'pointId': tip, 'duMm': 0, 'dvMm': -40})
    changed = call('app.state')['activeSession']['draftSketch']
    expected = copy.deepcopy(lower)
    for entity in expected['parameters']['entities']:
        if entity['id'] == original['id']: entity['tangentHandles'][tip]['dvMm'] = -40
        if entity['id'] == reflected['id']: entity['tangentHandles'][tip]['dvMm'] = 40
    assert changed['parameters'] == expected['parameters'], 'Only the retained source and derived reflected tangent may differ'
    points = {p['id']: p for p in changed['parameters']['points']}
    for row in group['pointPairs']:
        a, b = points[row['sourcePointId']], points[row['resultPointId']]
        assert a['uMm'] == b['uMm'] and a['vMm'] == -b['vMm']
    assert points[tip]['uMm'] == points[tip]['vMm'] == 0
    call('sketch.curvatureComb', {'entityId': original['id'], 'enabled': True, 'density': 256, 'scale': .01})
    comb, = [c for c in call('app.state')['viewport']['gpu']['splineHandles']['curvatureCombs'] if c['entityId'] == original['id']]
    samples = comb['samples']; nose = [p for p in samples if p['uMm'] <= 70 + 1e-6]
    quality = {'sourceTangentWeightBeforeMm': 200, 'sourceTangentWeightAfterMm': 40,
        'samples': samples, 'sampleCount': len(samples), 'noseSampleCount': len(nose),
        'minimumX': min(p['uMm'] for p in samples), 'minimumY': min(p['vMm'] for p in samples),
        'maximumForwardStepX': max(b['uMm']-a['uMm'] for a,b in zip(samples,samples[1:])),
        'maximumNoseStepY': max(b['vMm']-a['vMm'] for a,b in zip(nose,nose[1:])),
        'exactMirrorAndSharedTip': True, 'scope': '256 uniform-parameter actual-curve samples; bounded lobe/overshoot admission, not an analytic proof'}
    quality['passed'] = quality['minimumX'] >= -1e-7 and quality['minimumY'] >= -1e-7 and quality['maximumForwardStepX'] <= 1e-7 and quality['maximumNoseStepY'] <= 1e-7
    (out / 'nose-admission-quality.json').write_text(json.dumps(quality, indent=2))
    call('sketch.lookAt'); stage('Only source transverse tip tangent weight 40; reflected side derived', 'nose-sketch-admission.png')
    call('history.undo')  # remove diagnostic-only comb settings completely
    assert call('app.state')['activeSession']['draftSketch']['parameters'] == expected['parameters']
    if not quality['passed']:
        call('sketch.cancel')
        raise AssertionError('First bounded nose-weight hypothesis has a sampled lobe/overshoot; stop without sweeping')
    call('sketch.finish')
    state = call('document.inspect')
    assert state['authored']['features'] == [expected if f['id'] == lower['id'] else f for f in before['authored']['features']]
    outer, = state['evaluation']['bodies']
    assert outer['valid'] and outer['topology']['faces'] == 2
    call('document.save', {'path': str(out / 'nose-weight40-exterior.alloy')})
    for view in ('top', 'front', 'isometric', 'rearIsometric'):
        call('view.orientation', {'name': view}); call('view.fit')
        stage('Bounded tip weight40 exterior admission: ' + view, 'nose-exterior-' + view + '.png')
    return {'quality': quality, 'exterior': outer, 'source': str(source.resolve()),
        'scope': 'Exterior admission only; visually compare permitted public5:20/5:30 before full downstream candidate'}



def nose_weight_full(call, stage, source, admission_directory, out):
    """Propagate the admitted sole source tangent edit through unchanged full history."""
    quality = json.loads((admission_directory / 'nose-admission-quality.json').read_text())
    assert quality['passed'] and quality['sourceTangentWeightAfterMm'] == 40
    admitted = json.loads((admission_directory / 'nose-weight40-exterior.alloy').read_text())
    native = json.loads(source.resolve().read_text())
    assert len(native['features']) == 12
    assert json.loads((admission_directory / 'source-exterior-prefix.alloy').read_text())['features'] == native['features'][:5]
    call('document.open', {'path': str(source.resolve()), 'discard': True})
    before = call('document.inspect')
    lower, = [f for f in before['authored']['features'] if f['name'] == 'Lower Mirror pair']
    pair, = lower['parameters']['mirrorGroups'][0]['entityPairs']
    original, = [e for e in lower['parameters']['entities'] if e['id'] == pair['sourceEntityId']]
    tip = original['pointIds'][-1]
    assert original['tangentHandles'][tip] == {'duMm': 0, 'dvMm': -200}
    call('sketch.edit', {'sketchId': lower['id']})
    call('sketch.splineTangent', {'entityId': original['id'], 'pointId': tip, 'duMm': 0, 'dvMm': -40})
    expected, = [f for f in admitted['features'] if f['id'] == lower['id']]
    assert call('app.state')['activeSession']['draftSketch']['parameters'] == expected['parameters']
    call('sketch.finish')
    changed = call('document.inspect')
    (out / 'full-candidate-state.json').write_text(json.dumps(changed, indent=2))
    assert changed['authored']['features'] == [expected if f['id'] == lower['id'] else f for f in before['authored']['features']]
    solid, = changed['evaluation']['bodies']
    oldsolid, = before['evaluation']['bodies']
    assert solid['valid'] and solid['closed'] and solid['topology']['solids'] == 1
    assert abs(solid['volumeMm3']-oldsolid['volumeMm3']) > .01
    result = qualify_committed_solid(call, stage, before, out, source_change=True)
    result.update(sourceTipWeightMm=40, exactUnchangedOtherFeatureObjects=True,
        admittedPrefix=str(admission_directory.resolve()), sourceChangedMeaningfully=True)
    (out / 'nose-full-result.json').write_text(json.dumps(result, indent=2))
    return result


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--app', required=True)
    p.add_argument('--output', required=True, type=Path)
    p.add_argument('--record-video', type=Path)
    p.add_argument('--checkpoint', choices=['exterior', 'panel', 'trimmed-panel', 'bridge', 'sheets', 'trials', 'five-tools', 'solid'], default='trimmed-panel')
    p.add_argument('--through-trials', action='store_true')
    p.add_argument('--complete', action='store_true')
    p.add_argument('--reference-directory', type=Path)
    p.add_argument('--source', '--resume-source', dest='source', type=Path, help='Resume a reviewed bridge for sheets/trials/closure; not a from-blank recording')
    p.add_argument('--connected-trim-trial', action='store_true', help='Candidate09: one connected fixed three-face tool; separate-tool public parity unknown')
    p.add_argument('--trial-commit-undo', action='store_true', help='After exact Cancel, commit the ancestry-identified trial once and undo exactly')
    p.add_argument('--connected-fill-preview', action='store_true', help='Resume undone connected checkpoint; add bottom to same sketch/Extrude; inventory four-tool Boundary Fill and cancel')
    p.add_argument('--connected-solid-trial', action='store_true', help='Resume accepted four-tool checkpoint and commit one selected cell; exact history/reopen and checked exports')
    p.add_argument('--nose-weight-admission', action='store_true', help='Candidate12: only lower-source transverse tip tangent200→40; mirrored-curve and exterior admission')
    p.add_argument('--nose-weight-full', action='store_true', help='Propagate admitted source tip weight40 through candidate11; qualify exact edit/history/reopen and solid exports')
    p.add_argument('--nose-admission-directory', type=Path, help='Successful actual-app weight40 admission evidence directory')
    p.add_argument('--accepted-result', type=Path, help='Accepted candidate nose-full-result.json; full creation must match volume, area and envelope before export')
    p.add_argument('--connected-from-blank', action='store_true', help='Full accepted weight40 recipe: from blank through reversible connected Trim, same Extrude bottom, four-tool Fill, checked export and source edit')
    a = p.parse_args(); out = a.output.resolve(); out.mkdir(parents=True, exist_ok=True)
    if a.connected_from_blank and (not a.complete or a.source or not a.reference_directory or not a.accepted_result):
        p.error('--connected-from-blank requires --complete --reference-directory --accepted-result and no --source')
    if a.complete: a.checkpoint = 'solid'
    elif a.through_trials: a.checkpoint = 'trials'
    if a.connected_solid_trial:
        if a.connected_fill_preview or a.complete or a.through_trials or a.trial_commit_undo:
            p.error('--connected-solid-trial is a separate bounded commit trial')
        a.checkpoint = 'connected-solid'
        a.connected_trim_trial = True
    if a.connected_fill_preview:
        if a.complete or a.through_trials or a.trial_commit_undo:
            p.error('--connected-fill-preview is a separate bounded precommit inventory')
        a.checkpoint = 'fill-preview'
        a.connected_trim_trial = True
    advanced = a.checkpoint in ('sheets', 'trials', 'five-tools', 'solid', 'fill-preview', 'connected-solid')
    if a.nose_weight_full and (not a.source or not a.nose_admission_directory or a.nose_weight_admission):
        p.error('--nose-weight-full requires --source and --nose-admission-directory, separate from admission')
    if a.nose_weight_admission and not a.source:
        p.error('--nose-weight-admission requires accepted candidate11 --source')
    if a.source and not advanced and not a.nose_weight_admission and not a.nose_weight_full:
        p.error('--source resumes a reviewed bridge only for sheets/trials/closure')
    if a.connected_trim_trial and (not a.source or a.checkpoint not in ('sheets', 'trials', 'fill-preview', 'connected-solid')):
        p.error('--connected-trim-trial requires candidate07/08 --source and --checkpoint sheets or trials')
    if a.trial_commit_undo and not a.connected_trim_trial:
        p.error('--trial-commit-undo requires --connected-trim-trial')
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
                recording = False
                video.update(raw('recording.stop'))
        cleanup.callback(stop)
        raw('window.resize', {'width': 1500, 'height': 950})
        raw('document.new', {'discard': True})
        raw('ui.designTab', {'tab': 'surface'})
        if a.record_video:
            movie = a.record_video.resolve()
            assert not movie.is_relative_to(Path(__file__).resolve().parents[1])
            raw('recording.start', {'path': str(movie), 'fps': 5}); recording = True
        def call(command, parameters=None):
            started = time.time()
            operation = {'command': command, 'parameters': parameters or {}, 'startedUnix': started, 'status': 'running'}
            (out / 'current-operation.json').write_text(json.dumps(operation, indent=2))
            result = raw(command, parameters)
            operation.update(status='completed', elapsedSeconds=round(time.time()-started, 3))
            (out / 'current-operation.json').write_text(json.dumps(operation, indent=2))
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
            if a.nose_weight_full:
                result = nose_weight_full(call, stage, a.source, a.nose_admission_directory, out)
                (out / 'result.json').write_text(json.dumps({'passed': True, 'result': result, 'stages': stages}, indent=2))
                return
            if a.nose_weight_admission:
                result = nose_weight_admission(call, stage, a.source, out)
                (out / 'result.json').write_text(json.dumps({'passed': True, 'result': result, 'stages': stages}, indent=2))
                return
            references = setup_canvases(call, stage, a.reference_directory) if a.reference_directory and not a.source else []
            result = resume_surfaces(call, a.source, a.connected_trim_trial) if a.source else create_surfaces(call, stage, 'bridge' if advanced else a.checkpoint, out, tip_weight=40 if a.connected_from_blank else 200)
            result['referenceCanvases'] = references
            result['checkpoint'] = a.checkpoint
            result['adaptations'] = [
                'Source starts with prepared sketches; dimensions and fit points are independently authored.',
                'Central ellipse is authored in a separate sketch before opening Trim; the panel keeps coincident endpoint constraints.',
                'Exterior rail contact uses matching authored coordinates, not retained Project references.',
                'Public photos are perspective references at display scale, not calibrated metric drawings.',
                'Trim targets and removed rear regions are declared fixture choices; full source target inventory is unknown.']
            if a.connected_from_blank:
                result['adaptations'].append('Accepted source lower tip tangent weight40; upper/connector/lower are one connected sheet, replacing public separate tools; four Boundary Fill tools rather than five. Source exact dimensions and native gesture parity remain unknown.')
            result['gpu'] = call('app.state')['viewport']['gpu']
            if a.connected_solid_trial:
                state = call('document.inspect')
                assert len(state['authored']['features']) == 11 and len(state['evaluation']['bodies']) == 4
                sketch, extrude = state['authored']['features'][-2:]
                assert sketch['name'] == 'Back and notch sheets' and extrude['type'] == 'surfaceExtrude'
                assert [len(p['entityIds']) for p in extrude['parameters']['profiles']] == [3, 1]
                assert extrude['parameters']['sideOneMm'] == extrude['parameters']['sideTwoMm'] == 70
                sheets = {'sketchId': sketch['id'], 'extrudeId': extrude['id']}
                result['closurePreview'] = connected_fill_preview(call, stage, result, sheets, out, commit_solid=True)
            elif advanced:
                sheets = connected_trial_sheets(call, stage, result, out) if a.connected_trim_trial else create_trial_sheets(call, stage, result, out, connected=a.connected_from_blank)
                result['sheetReachEachSideMm'] = sheets['reachEachSideMm']
                if a.checkpoint not in ('sheets', 'fill-preview'):
                    result['trials'] = connected_trim_trial(call, stage, result, sheets, out, a.trial_commit_undo or a.connected_from_blank) if (a.connected_trim_trial or a.connected_from_blank) else trim_trials(call, stage, result, sheets, out)
                if a.checkpoint in ('five-tools', 'solid', 'fill-preview'):
                    add_bottom_sheet(call, stage, sheets, out, connected=a.connected_fill_preview or a.connected_from_blank)
                if a.connected_fill_preview:
                    result['closurePreview'] = connected_fill_preview(call, stage, result, sheets, out)
                if a.checkpoint == 'solid':
                    result['closure'] = fill_and_export(call, stage, out, connected=a.connected_from_blank, accepted_solid=json.loads(a.accepted_result.read_text())['solid'] if a.accepted_result else None)
            model = out / ('iron-finished.alloy' if a.checkpoint == 'solid' else 'iron-' + a.checkpoint + '.alloy' if advanced else 'iron-exterior.alloy' if a.checkpoint == 'exterior' else 'iron-prepared-panel.alloy' if a.checkpoint == 'panel' else 'iron-trimmed-panel.alloy' if a.checkpoint == 'trimmed-panel' else 'iron-surfaces.alloy')
            call('document.save', {'path': str(model)})
            stop()
            if a.checkpoint == 'solid':
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
                'result': result, 'video': video, 'scope': ('Corrected source trial/closure candidate; see explicit checkpoint, resume provenance and artifacts. API evidence, not native input or independent tutorial acceptance.' if advanced else 'Corrected exterior only; panel/grip and closure pending.' if a.checkpoint == 'exterior' else 'Prepared broad panel and high tilted ellipse; no Trim or bridge yet.' if a.checkpoint == 'panel' else 'Corrected exterior, large retained bilateral panel Trim and preauthored tilted ellipse; bridge not evaluated in this checkpoint.' if a.checkpoint == 'trimmed-panel' else 'Corrected outer and inner surfaces only; closing sheet/trial recipe and final closure unqualified.')}, indent=2))
        except Exception as error:
            failure = {'passed': False, 'checkpoint': a.checkpoint, 'error': str(error)}
            transport_failed = any(word in str(error).lower() for word in ('timed out', 'timeout', 'connection closed'))
            if transport_failed:
                # Do not queue another long request behind a blocked GUI thread.
                # Existing native checkpoints, transcript and in-flight record
                # preserve the exact last completed state; the harness closes it.
                recording = False
                failure['inspectionError'] = 'Skipped further API inspection after transport failure'
            else:
                try:
                    state = raw('app.state')
                    (out / 'failure-state.json').write_text(json.dumps(state, indent=2))
                    (out / 'last-committed-authored.alloy').write_text(json.dumps(state['document']['authored'], indent=2))
                    raw('capture.screenshot', {'path': str(out / 'failure.png')})
                except Exception as inspection_error:
                    failure['inspectionError'] = str(inspection_error)
            (out / 'result.json').write_text(json.dumps(failure, indent=2))
            raise
        finally:
            (out / 'transcript.json').write_text(json.dumps(ledger, indent=2))


if __name__ == '__main__':
    main()
