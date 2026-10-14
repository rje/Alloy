#!/usr/bin/env python3
"""From-blank adapted enclosure through ports, long slot and rounded card opening.

The local STEP is an independently authored nominal input. The XY axis choice and
placement are disclosed engineering adaptations; this runner does not certify fit
of the historical tutorial geometry against the independently authored fixture.
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


    # The connector-side cut uses a retained zero-offset construction plane,
    # then true imported face boundary projection and three signed Offset groups.
    current_case = next(b for b in inspect()['evaluation']['bodies']
                        if b['bodyId'] == case_body_id)
    def planar_face(body, axis, coordinate, direction):
        found = [f for f in body['supportFaces']
                 if abs(f['frame']['origin'][axis] - coordinate) < 1e-5
                 and f['frame']['normal'][axis] * direction > .99]
        assert len(found) == 1, [(f['role'], f['frame']) for f in found]
        return found[0]
    port_outer = planar_face(current_case, 1, 46.5, +1)
    port_inner = next(f for f in current_case['supportFaces']
                      if abs(f['frame']['origin'][1] - 43) < 1e-5
                      and abs(f['frame']['normal'][1]) > .99)
    stage('Offset Plane 0 from the three-port outer Case wall')
    port_plane = call('construction.plane', {'source': {'support': ref(port_outer)},
        'offset': 0, 'units': 'mm', 'name': 'Three-port plane'})['planeId']
    connector_names = ('SIDE-ETH-001', 'SIDE-USB-A-001', 'SIDE-USB-B-001')
    connectors = [next(b for b in inspect()['evaluation']['bodies'] if b['name'] == name)
                  for name in connector_names]
    connector_faces = [planar_face(b, 1, 45.5, +1) for b in connectors]
    stage('Sketch on plane; Project three imported connector faces')
    call('sketch.begin', {'planeId': port_plane,
                          'name': 'Three linked port clearances'})
    call('analysis.visibility', {'analysisId': top_analysis['id'], 'visible': False})
    width_normal_y = width_analysis['resolvedPlane']['normal'][1]
    assert abs(width_normal_y) > .99
    call('inspect.section', {'analysisId': width_analysis['id'],
                             'offsetMm': 32 / width_normal_y})
    call('analysis.visibility', {'analysisId': width_analysis['id'], 'visible': True})
    width_at_ports = next(a for a in call('analysis.inspect')['analyses']
                          if a['id'] == width_analysis['id'])
    close(width_at_ports['resolvedPlane']['origin'][1], 32)
    stage('Edit Width section to adapted Y = 32 mm while port sketch is active',
          'ports-width-section.png')
    call('sketch.projectBegin')
    for face in connector_faces:
        assert call('sketch.projectFace', ref(face))['added'] == 4
    call('sketch.projectCommit')
    port_sketch_id = call('app.state')['activeSession']['draftSketch']['id']
    projected = call('app.state')['activeSession']['draftSketch']['parameters']['projections']
    assert len(projected) == 12
    stage('Twelve locked purple source edges on retained plane', 'ports-projected.png')
    offset_groups = []
    for body in connectors:
        source_ids = [row['id'] for row in projected
                      if row['source']['bodyId'] == body['bodyId']]
        assert len(source_ids) == 4
        call('sketch.tool', {'tool': 'offset'})
        preview = call('sketch.offsetPreview', {'sourceEntityIds': source_ids,
            'distance': 1, 'units': 'mm'})
        assert preview['pendingClosedRectOffset']['valid']
        accepted = call('sketch.offsetAccept')
        offset_groups.append(accepted['draftSketch']['parameters']['closedRectOffsets'][-1])
    port_draft = call('app.state')['activeSession']['draftSketch']
    assert len(offset_groups) == 3 and len({g['id'] for g in offset_groups}) == 3
    assert all(not p['profile'] for p in port_draft['parameters']['projections'])
    stage('Three linked outward 1 mm rectangle offsets', 'ports-offset-one.png')
    call('sketch.finish')
    call('analysis.visibility', {'analysisId': width_analysis['id'], 'visible': False})
    call('analysis.visibility', {'analysisId': top_analysis['id'], 'visible': True})
    call('view.orientation', {'name': 'top'})
    stage('Switch to Top before connector Cut To Object')
    port_sketch = next(s for s in inspect()['evaluation']['sketches']
                       if s['sketchId'] == port_sketch_id)
    port_profiles = [p['profileId'] for p in port_sketch['profiles']]
    assert len(port_profiles) == 4
    close(sum(p['areaMm2'] for p in port_sketch['profiles']), 973.5)
    before_port_cut = next(b for b in inspect()['evaluation']['bodies']
                           if b['bodyId'] == case_body_id)['volumeMm3']
    stage('Cut all four union cells To Object, inner port wall; Case target only')
    call('solid.extrude', {'sketchId': port_sketch_id,
        'profileIds': port_profiles, 'extent': 'toObject',
        'endSupport': ref(port_inner), 'operation': 'cut',
        'targetBodyIds': [case_body_id]})
    current_case = next(b for b in inspect()['evaluation']['bodies']
                        if b['bodyId'] == case_body_id)
    assert current_case['valid'] and current_case['volumeMm3'] < before_port_cut
    call('view.home');call('view.orientation', {'name': 'back'});call('view.zoom', {'factor': 1.45})
    stage('Actual connected connector clearance opening', 'three-port-cut.png')

    # Local slot coordinates are derived from the selected current face frame,
    # not copied from one orientation of its sketch axes.
    def local_point(frame, world):
        delta = [world[i] - frame['origin'][i] for i in range(3)]
        return (sum(delta[i] * frame['uAxis'][i] for i in range(3)),
                sum(delta[i] * frame['vAxis'][i] for i in range(3)))
    slot_outer = planar_face(current_case, 0, 32, +1)
    slot_inner = next(f for f in current_case['supportFaces']
                      if abs(f['frame']['origin'][0] - 28.5) < 1e-5
                      and abs(f['frame']['normal'][0]) > .99)
    stage('Overall Slot on long Case wall: adapted endpoints, width 6.5 mm')
    call('sketch.begin', {'support': ref(slot_outer), 'name': 'Long connector slot'})
    slot_first = local_point(slot_outer['frame'], (32, -39, 10.6))
    slot_last = local_point(slot_outer['frame'], (32, 10.25, 10.6))
    call('sketch.overallSlot', {'firstX': slot_first[0], 'firstY': slot_first[1],
        'lastX': slot_last[0], 'lastY': slot_last[1], 'width': 6.5, 'units': 'mm'})
    slot_sketch_id = call('app.state')['activeSession']['draftSketch']['id']
    stage('Retained 6.5 mm Overall Slot', 'long-slot-sketch.png')
    call('sketch.finish')
    slot_sketch = next(s for s in inspect()['evaluation']['sketches']
                       if s['sketchId'] == slot_sketch_id)
    assert len(slot_sketch['profiles']) == 1
    slot_volume_before = next(b for b in inspect()['evaluation']['bodies']
                              if b['bodyId'] == case_body_id)['volumeMm3']
    stage('Cut Overall Slot To Object, inner long wall; Case target only')
    call('solid.extrude', {'sketchId': slot_sketch_id,
        'profileId': slot_sketch['profiles'][0]['profileId'],
        'extent': 'toObject', 'endSupport': ref(slot_inner),
        'operation': 'cut', 'targetBodyIds': [case_body_id]})
    current_case = next(b for b in inspect()['evaluation']['bodies']
                        if b['bodyId'] == case_body_id)
    assert current_case['valid'] and current_case['volumeMm3'] < slot_volume_before
    call('view.home');call('view.orientation', {'name': 'right'});call('view.zoom', {'factor': 1.45})
    stage('Actual long-side slot opening', 'long-slot-cut.png')

    card_outer = planar_face(current_case, 1, -46.5, -1)
    card_inner = next(f for f in current_case['supportFaces']
                      if abs(f['frame']['origin'][1] + 43) < 1e-5
                      and abs(f['frame']['normal'][1]) > .99)
    stage('Center Rectangle on opposite short wall, 16 × 12 mm')
    call('sketch.begin', {'support': ref(card_outer), 'name': 'Card opening'})
    card_origin = local_point(card_outer['frame'], (0, -46.5, 0))
    for actual, expected in zip(frame_point(card_outer['frame'], *card_origin),
                                (0, -46.5, 0)):
        close(actual, expected)
    call('sketch.centerRectangle', {'centerX': card_origin[0],
        'centerY': card_origin[1], 'width': 16, 'height': 12,
        'units': 'mm'})
    card_sketch_id = call('app.state')['activeSession']['draftSketch']['id']
    stage('World-axis-centered 16 × 12 card rectangle', 'card-rectangle-sketch.png')
    call('sketch.finish')
    card_sketch = next(s for s in inspect()['evaluation']['sketches']
                       if s['sketchId'] == card_sketch_id)
    assert len(card_sketch['profiles']) == 1
    card_volume_before = next(b for b in inspect()['evaluation']['bodies']
                              if b['bodyId'] == case_body_id)['volumeMm3']
    stage('Cut card rectangle To Object, opposite inner wall')
    call('solid.extrude', {'sketchId': card_sketch_id,
        'profileId': card_sketch['profiles'][0]['profileId'],
        'extent': 'toObject', 'endSupport': ref(card_inner),
        'operation': 'cut', 'targetBodyIds': [case_body_id]})
    current_case = next(b for b in inspect()['evaluation']['bodies']
                        if b['bodyId'] == case_body_id)
    assert current_case['valid'] and current_case['volumeMm3'] < card_volume_before
    call('view.home');call('view.orientation', {'name': 'front'});call('view.zoom', {'factor': 1.5})
    stage('Actual opposite card wall opening before R2 lip', 'card-cut-pre-fillet.png')
    card_lip = [e for e in current_case['solidEdges']
                if e['available'] and e['provenance'].get('role') == 'cutRim'
                and e['provenance'].get('sourceSketchId') == card_sketch_id
                and e['provenance'].get('targetFaceId') == card_outer['faceId']]
    assert len(card_lip) == len({e['provenance']['sourceEntityId'] for e in card_lip}) == 3
    for edge in card_lip:
        for endpoint in (edge['provenance']['startMm'], edge['provenance']['endMm']):
            close(endpoint[1], -46.5)
            assert -8.00001 <= endpoint[0] <= 8.00001
            assert 2.49999 <= endpoint[2] <= 6.00001
    stage('Fillet the three exact current card U-lip edges R2')
    call('solid.fillet', {'targetBodyId': case_body_id,
        'edgeIds': [e['edgeId'] for e in card_lip], 'radiusMm': 2,
        'tangentChain': True})
    case_final = next(b for b in inspect()['evaluation']['bodies']
                      if b['bodyId'] == case_body_id)
    assert case_final['valid'] and case_final['closed']
    assert case_final['volumeMm3'] < current_case['volumeMm3']
    call('component.opacitySet', {'definitionId': case_owner, 'opacity': 1})
    call('feature.visibility', {'featureId': port_plane, 'visible': False})
    for analysis in call('analysis.inspect')['analyses']:
        call('analysis.visibility', {'analysisId': analysis['id'], 'visible': False})
    stage('Rounded card lip R2 on the centered short wall', 'card-r2-front.png')
    call('view.home');call('view.orientation', {'name': 'back'});call('view.zoom', {'factor': 1.45})
    stage('Connected connector clearance opening in the finished Case', 'final-three-port-closeup.png')
    call('view.home');call('view.orientation', {'name': 'right'});call('view.zoom', {'factor': 1.45})
    stage('Long-wall slot in the finished Case', 'final-long-slot-closeup.png')
    restored = inspect()

    stage('Save and reopen authored ports, slot, card R2 and link history')
    design_path = out / 'enclosure-port-card.alloy'
    call('document.save', {'path': str(design_path)})
    call('document.new', {'discard': True})
    call('document.open', {'path': str(design_path), 'discard': True})
    reopened = inspect()
    assert reopened['authored'] == restored['authored']
    assert reopened['bodies'] == 13
    assert [a['name'] for a in call('analysis.inspect')['analyses']] == ['Length', 'Width', 'Top']
    assert call('reference.inspect')['references'][0]['sourceStatus'] == 'current'
    call('component.activate', {'definitionId': case_owner})
    call('view.home');call('view.orientation', {'name': 'front'});call('view.zoom', {'factor': 1.4})
    stage('Reopened editable Case and linked board', 'reopened.png')

    stage('Export only the Case component in millimeters')
    stl_path = out / 'case-port-card-mm.stl'
    stl_result = call('document.exportStl', {'path': str(stl_path),
        'occurrenceId': case['occurrenceId'], 'units': 'mm', 'format': 'binary',
        'surfaceDeviationMm': .1, 'normalDeviationDeg': 10})
    mesh = inspect_stl(stl_path, True, allow_cavities=True)
    assert len(mesh['components']) == 1
    assert mesh['components'][0]['signedVolume'] > 0
    close(mesh['signedVolume'], case_final['volumeMm3'], .01 * case_final['volumeMm3'])
    for actual, expected in zip(mesh['bounds'][0], [-32, -46.5, 0]):
        close(actual, expected, .02)
    for actual, expected in zip(mesh['bounds'][1], [32, 46.5, 30]):
        close(actual, expected, .02)
    step_path = out / 'case-port-card-mm.step'
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
    (out / 'enclosure-port-card-receipt.json').write_text(json.dumps({
        'fixtureSha256': fixture_hash, 'stages': stages, 'referenceFeatureId': reference_id,
        'boardOccurrenceId': board_occurrence, 'caseComponent': case,
        'caseBodyId': case_body_id, 'projectedCircles': peg_sketch['projectedCircles'],
        'pegProfileIds': peg_profiles, 'portOffsetGroupIds': [g['id'] for g in offset_groups],
        'portProfileIds': port_profiles, 'slotSketchId': slot_sketch_id,
        'cardSketchId': card_sketch_id, 'cardLipEdgeIds': [e['edgeId'] for e in card_lip],
        'analyses': call('analysis.inspect')['analyses'],
        'stl': stl_result, 'stlRead': mesh, 'step': step_result,
        'stepRead': step_read, 'video': video_result, 'ledger': ledger}, indent=2) + '\n')
print('PASS: from blank through linked ports, long slot and rounded card R2')
