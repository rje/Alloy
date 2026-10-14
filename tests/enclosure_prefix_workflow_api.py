#!/usr/bin/env python3
"""From-blank adapted enclosure prefix through four linked mounting pegs.

The local STEP is an independently authored nominal input. The XY axis choice and
placement are disclosed engineering adaptations; this runner does not certify fit
of connector envelopes against the still-unopened case walls.
"""
import argparse
from contextlib import ExitStack
import hashlib
import json
import math
import shutil
import subprocess
import time
import uuid
from pathlib import Path

from app_harness import application
from stl_export_api import inspect_stl


parser = argparse.ArgumentParser()
parser.add_argument('--app', required=True)
parser.add_argument('--fixture', required=True)
parser.add_argument('--output', required=True)
parser.add_argument('--step-tool', required=True)
parser.add_argument('--record-video', type=Path)
args = parser.parse_args()
out = Path(args.output).resolve()
out.mkdir(parents=True, exist_ok=True)
source = out / 'adapted-board-reference.step'
shutil.copyfile(args.fixture, source)
fixture_hash = hashlib.sha256(source.read_bytes()).hexdigest()
assert fixture_hash == 'f695639a970642c3154f10758acccdafc9d71a8efdace512b1ce057e5805fe27'


def ref(face):
    return {key: face[key] for key in ('producerFeatureId', 'bodyId', 'faceId')}


def close(actual, expected, tol=1e-5):
    assert math.isclose(actual, expected, abs_tol=tol), (actual, expected)


def bounds(body, expected_min, expected_max, tol=1e-4):
    for key, expected in (('min', expected_min), ('max', expected_max)):
        for actual, target in zip(body['boundsMm'][key], expected):
            close(actual, target, tol)


def group(edges, radius):
    return {'id': str(uuid.uuid4()), 'edgeIds': edges,
            'radiusMm': radius, 'tangentChain': True}


def frame_point(frame, u, v):
    return tuple(frame['origin'][i] + u * frame['uAxis'][i]
                 + v * frame['vAxis'][i] for i in range(3))


with application(args.app, out, command_timeout=120) as (raw, ledger), ExitStack() as cleanup:
    recording = False
    video_result = {}
    stages = []

    def stop_recording():
        if recording:
            video_result.update(raw('recording.stop'))

    cleanup.callback(stop_recording)
    raw('window.resize', {'width': 1400, 'height': 1000})
    if args.record_video:
        movie = args.record_video.resolve()
        assert not movie.is_relative_to(Path(__file__).resolve().parents[1])
        movie.parent.mkdir(parents=True, exist_ok=True)
        raw('recording.start', {'path': str(movie), 'fps': 5})
        recording = True

    def call(command, inputs=None, **kwargs):
        result = raw(command, inputs, **kwargs)
        if recording and command.startswith(('sketch.', 'solid.', 'component.',
                'reference.', 'inspect.', 'analysis.', 'document.', 'history.', 'view.')):
            time.sleep(.28)
        return result

    def stage(caption, capture=None):
        stages.append(caption)
        if recording:
            raw('recording.step', {'text': 'API | ' + caption})
            time.sleep(.9)
        if capture:
            call('capture.screenshot', {'path': str(out / capture)})

    def inspect():
        return call('document.inspect')

    assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
    assert inspect()['bodies'] == 0
    stage('Insert the independently authored board assembly')
    imported = call('reference.insert', {'path': str(source), 'name': 'Board Reference'})
    reference_id = imported['featureId']
    board_occurrence = imported['outerOccurrenceId']
    assert len(imported['assembly']['occurrences']) > 12
    original = inspect()
    assert original['bodies'] == 12

    # Keep the insertion rotation and later placed/captured translation as
    # separate authored history, rather than pre-transforming the STEP input.
    stage('Rotate linked board insertion 90 degrees about Z; Capture Position')
    rotation = {'axis': 'Z', 'angleDeg': 90, 'translationMm': [0, 0, 0]}
    call('reference.move', {'featureId': reference_id, 'placement': rotation})
    assert call('app.state')['pendingReferencePosition']['featureId'] == reference_id
    call('reference.capture')
    rotated = inspect()
    assert rotated['bodies'] == 12
    stage('Create sibling Case and activate its authoring context')
    case = call('component.create', {'name': 'Case', 'activate': True})
    case_owner = case['definitionId']
    stage('Place board +28, −42.5, +6 mm; Capture Position')
    placement = {'axis': 'Z', 'angleDeg': 90,
                 'translationMm': [28, -42.5, 6]}
    call('reference.move', {'featureId': reference_id, 'placement': placement})
    call('reference.capture')
    placed = inspect()
    pcb = next(b for b in placed['evaluation']['bodies'] if b['name'] == 'PCB-001')
    bounds(pcb, [-28, -42.5, 6], [28, 42.5, 7.6])
    assert pcb['outerOccurrenceId'] == board_occurrence
    holes = [e for e in pcb['solidEdges']
             if e['provenance'].get('sourceEdgeIndex') in (12, 13, 14, 15)]
    assert len(holes) == len({e['edgeId'] for e in holes}) == 4
    stage('Centered 64 × 93 rectangle on XY')
    call('sketch.begin', {'plane': 'XY', 'name': 'Centered case stock'})
    call('sketch.centerRectangle', {'centerX': 0, 'centerY': 0,
        'width': 64, 'height': 93, 'units': 'mm',
        'centerSnapTarget': {'kind': 'origin'}})
    stock_sketch = call('app.state')['activeSession']['draftSketch']['id']
    call('sketch.finish')
    profile = next(s for s in inspect()['evaluation']['sketches']
                   if s['sketchId'] == stock_sketch)['profiles'][0]['profileId']
    stage('Extrude centered stock 30 mm')
    call('solid.extrude', {'sketchId': stock_sketch, 'profileId': profile,
                          'distance': 30, 'units': 'mm'})
    stock = next(b for b in inspect()['evaluation']['bodies'] if b['ownerId'] == case_owner)
    case_body_id = stock['bodyId']
    bounds(stock, [-32, -46.5, 0], [32, 46.5, 30])
    call('component.opacitySet', {'definitionId': case_owner, 'opacity': .6})
    vertical = [e['edgeId'] for e in stock['solidEdges']
                if e['provenance']['role'] == 'sideCorner']
    rims = [e['edgeId'] for e in stock['solidEdges']
            if e['provenance']['role'] in ('baseRim', 'endRim')]
    assert len(vertical) == 4 and len(rims) == 8
    stage('One Fillet: four vertical edges R5; eight top and bottom rims R2.5')
    call('solid.fillet', {'targetBodyId': case_body_id,
        'groups': [group(vertical, 5), group(rims, 2.5)]})
    rounded = next(b for b in inspect()['evaluation']['bodies'] if b['bodyId'] == case_body_id)
    assert rounded['valid'] and len(rounded['radiusGroups']) == 2
    stage('Closed Shell 3.5 mm; retain six exact inner planar supports')
    call('solid.shell', {'targetBodyId': case_body_id, 'thicknessMm': 3.5})
    case_shell = next(b for b in inspect()['evaluation']['bodies'] if b['bodyId'] == case_body_id)
    assert case_shell['valid'] and case_shell['closed'] and case_shell['topology']['shells'] == 2
    inner = [f for f in case_shell['supportFaces'] if f['role'] == 'shell-inner']
    assert len(inner) == 6
    floor_face = next(f for f in inner if f['frame']['normal'][2] > .99)
    close(floor_face['frame']['origin'][2], 3.5)
    cavity_volume = (86 * 57 - (4 - math.pi) * 1.5**2) * 23
    close(case_shell['volumeMm3'], rounded['volumeMm3'] - cavity_volume, 1e-4)
    call('view.home')
    stage('Case shell and captured board placement', 'shell-and-board.png')

    # Analyses are retained visual state. Each previous section is hidden so
    # only the one being demonstrated clips the next capture.
    before_sections = inspect()
    stage('Length section: YZ at X = 0')
    call('inspect.section', {'plane': 'YZ', 'name': 'Length'})
    length_analysis = call('analysis.inspect')['analyses'][-1]
    assert length_analysis['resolvedPlane']['origin'] == [0, 0, 0]
    stage('Length section', 'length-section.png')
    call('analysis.visibility', {'analysisId': length_analysis['id'], 'visible': False})
    stage('Width section: XZ at Y = 0')
    call('inspect.section', {'plane': 'XZ', 'name': 'Width'})
    width_analysis = call('analysis.inspect')['analyses'][-1]
    assert width_analysis['resolvedPlane']['origin'] == [0, 0, 0]
    stage('Width section', 'width-section.png')
    call('analysis.visibility', {'analysisId': width_analysis['id'], 'visible': False})
    outer_top = next(f for f in case_shell['supportFaces'] if f['role'] == 'extrude.endCap')
    stage('Top section: outer top support, 5.5 mm inward')
    call('inspect.section', {'plane': 'face', 'support': ref(outer_top),
                             'offsetMm': -5.5, 'name': 'Top'})
    top_analysis = call('analysis.inspect')['analyses'][-1]
    assert top_analysis['resolvedPlane']['origin'] == [0, 0, 24.5]
    call('inspect.section', {'analysisId': top_analysis['id'], 'flip': True})
    top_analysis = call('analysis.inspect')['analyses'][-1]
    assert top_analysis['resolvedPlane']['normal'] == [0, 0, -1]
    stage('Top section retained and flipped', 'top-section.png')
    after_sections = inspect()
    assert after_sections['authored']['features'] == before_sections['authored']['features']
    close(next(b for b in after_sections['evaluation']['bodies']
               if b['bodyId'] == case_body_id)['volumeMm3'], case_shell['volumeMm3'])
    close(pcb['boundsMm']['min'][2] - floor_face['frame']['origin'][2], 2.5)
    for axis, extent in ((0, 28.5), (1, 43)):
        close(extent - pcb['boundsMm']['max'][axis], .5)
        close(pcb['boundsMm']['min'][axis] + extent, .5)
    for x in (pcb['boundsMm']['min'][0], pcb['boundsMm']['max'][0]):
        for y in (pcb['boundsMm']['min'][1], pcb['boundsMm']['max'][1]):
            dx = max(0, abs(x) - 27)
            dy = max(0, abs(y) - 41.5)
            assert math.hypot(dx, dy) < 1.5  # even PCB bounding corners clear the inner wall
    for name in ('PCB-001', 'GPIO-001', 'CORE-001', 'RIBBON-001'):
        body = next(b for b in after_sections['evaluation']['bodies'] if b['name'] == name)
        assert body['boundsMm']['min'][2] >= 3.5
        assert body['boundsMm']['max'][2] <= 26.5
    card = next(b for b in after_sections['evaluation']['bodies'] if b['name'] == 'CARD-SLOT-001')
    close(card['boundsMm']['min'][2], 3.5)
    call('view.orientation', {'name': 'top'})
    call('view.home')
    stage('Board placed within the nominal case plan', 'placed-board.png')

    # The closed case remains uncut at the connector side. Test the PCB and
    # mounting interface, not a false whole-assembly no-intersection claim.
    stage('Sketch on retained inner floor; Project four actual PCB hole edges')
    call('sketch.begin', {'support': ref(floor_face), 'name': 'Four mounting pegs'})
    call('sketch.projectBegin')
    for edge in holes:
        call('sketch.projectSelect', {'bodyId': pcb['bodyId'], 'edgeId': edge['edgeId']})
    call('sketch.projectCommit')
    draft = call('app.state')['activeSession']['draftSketch']
    projected = draft['parameters']['circleProjections']
    assert len(projected) == 4
    assert all(c['source']['occurrenceId'] == pcb['occurrenceId'] for c in projected)
    for circle in projected:
        call('sketch.circleOffset', {'sourceEntityId': circle['id'],
                                     'distance': -.3, 'units': 'mm'})
    stage('Four linked circles Offset −0.3 mm', 'peg-sketch.png')
    call('sketch.finish')
    peg_sketch = next(s for s in inspect()['evaluation']['sketches']
                      if s['name'] == 'Four mounting pegs')
    assert len(peg_sketch['projectedCircles']) == 4
    for circle in peg_sketch['projectedCircles']:
        close(circle['radiusMm'], 1.35)
    expected_holes = {(24.5, -39), (24.5, 19), (-24.5, -39), (-24.5, 19)}
    actual_holes = {(round(frame_point(floor_face['frame'], c['uMm'], c['vMm'])[0], 5),
                     round(frame_point(floor_face['frame'], c['uMm'], c['vMm'])[1], 5))
                    for c in peg_sketch['projectedCircles']}
    assert actual_holes == expected_holes, actual_holes
    peg_profiles = [p['profileId'] for p in peg_sketch['profiles']
                    if math.isclose(p['areaMm2'], math.pi * 1.05**2, abs_tol=1e-5)]
    assert len(peg_profiles) == 4
    stage('Extrude four peg profiles 5 mm, Join to the Case')
    call('solid.extrude', {'sketchId': peg_sketch['sketchId'],
        'profileIds': peg_profiles, 'distance': 5, 'units': 'mm',
        'operation': 'join', 'targetBodyIds': [case_body_id]})
    pegged = inspect()
    case_final = next(b for b in pegged['evaluation']['bodies'] if b['bodyId'] == case_body_id)
    assert pegged['bodies'] == 13 and case_final['valid'] and case_final['closed']
    close(case_final['volumeMm3'] - case_shell['volumeMm3'],
          4 * math.pi * 1.05**2 * 5, 1e-4)
    close(floor_face['frame']['origin'][2] + 5, 8.5)
    call('view.home')
    stage('Retained four-peg enclosure with board in place', 'four-pegs-assembled.png')
    call('feature.visibility', {'featureId': reference_id, 'visible': False})
    stage('Case-only view: four joined mounting pegs', 'four-pegs-case-only.png')
    call('feature.visibility', {'featureId': reference_id, 'visible': True})
    pegged = inspect()

    # A captured source placement edit changes every linked circle and Join;
    # undo restores the exact authored state and original nominal alignment.
    stage('Edit board placement: linked mounting references follow')
    shifted = {'axis': 'Z', 'angleDeg': 90,
               'translationMm': [28.25, -42.5, 6]}
    call('reference.move', {'featureId': reference_id, 'placement': shifted})
    call('reference.capture')
    edited = inspect()
    moved_sketch = next(s for s in edited['evaluation']['sketches']
                        if s['sketchId'] == peg_sketch['sketchId'])
    assert len(moved_sketch['projectedCircles']) == 4
    for old, new in zip(peg_sketch['projectedCircles'], moved_sketch['projectedCircles']):
        assert old['id'] == new['id']
        close(new['radiusMm'], old['radiusMm'])
        close(abs(new['uMm'] - old['uMm']) + abs(new['vMm'] - old['vMm']), .25)
    assert next(b for b in edited['evaluation']['bodies']
                if b['bodyId'] == case_body_id)['valid']
    shifted_pcb = next(b for b in edited['evaluation']['bodies'] if b['bodyId'] == pcb['bodyId'])
    close(28.5 - shifted_pcb['boundsMm']['max'][0], .25)
    call('history.undo')
    restored = inspect()
    assert restored['authored'] == pegged['authored']
    assert next(b for b in restored['evaluation']['bodies']
                if b['bodyId'] == case_body_id)['valid']

    stage('Save and reopen authored feature, analysis and link history')
    design_path = out / 'enclosure-prefix.alloy'
    call('document.save', {'path': str(design_path)})
    call('document.new', {'discard': True})
    call('document.open', {'path': str(design_path), 'discard': True})
    reopened = inspect()
    assert reopened['authored'] == restored['authored']
    assert reopened['bodies'] == 13
    assert [a['name'] for a in call('analysis.inspect')['analyses']] == ['Length', 'Width', 'Top']
    assert call('reference.inspect')['references'][0]['sourceStatus'] == 'current'
    stage('Reopened editable Case and linked board', 'reopened.png')

    stage('Export only the Case component in millimeters')
    stl_path = out / 'case-prefix-mm.stl'
    stl_result = call('document.exportStl', {'path': str(stl_path),
        'occurrenceId': case['occurrenceId'], 'units': 'mm', 'format': 'binary',
        'surfaceDeviationMm': .1, 'normalDeviationDeg': 10})
    mesh = inspect_stl(stl_path, True, allow_cavities=True)
    assert len(mesh['components']) == 2
    assert sum(c['signedVolume'] > 0 for c in mesh['components']) == 1
    assert sum(c['signedVolume'] < 0 for c in mesh['components']) == 1
    close(mesh['signedVolume'], case_final['volumeMm3'], .01 * case_final['volumeMm3'])
    for actual, expected in zip(mesh['bounds'][0], [-32, -46.5, 0]):
        close(actual, expected, .02)
    for actual, expected in zip(mesh['bounds'][1], [32, 46.5, 30]):
        close(actual, expected, .02)
    step_path = out / 'case-prefix-mm.step'
    step_result = call('document.exportStep', {'path': str(step_path),
        'bodyId': case_body_id, 'units': 'mm'})
    step_read = json.loads(subprocess.run(
        [str(Path(args.step_tool).resolve()), 'read', str(step_path)],
        check=True, capture_output=True, text=True).stdout.splitlines()[-1])
    assert step_read['valid'] and step_read['solidCount'] == 1
    assert step_result['volumeMm3'] == case_final['volumeMm3']
    if recording:
        time.sleep(1.5)
        stop_recording()
        recording = False
    (out / 'enclosure-prefix-receipt.json').write_text(json.dumps({
        'fixtureSha256': fixture_hash, 'stages': stages, 'referenceFeatureId': reference_id,
        'boardOccurrenceId': board_occurrence, 'caseComponent': case,
        'caseBodyId': case_body_id, 'projectedCircles': peg_sketch['projectedCircles'],
        'pegProfileIds': peg_profiles, 'analyses': call('analysis.inspect')['analyses'],
        'stl': stl_result, 'stlRead': mesh, 'step': step_result,
        'stepRead': step_read, 'video': video_result, 'ledger': ledger}, indent=2) + '\n')
print('PASS: from blank linked board, centered closed case, three sections, four joined pegs, edit/reopen and Case-only export')
