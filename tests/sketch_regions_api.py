#!/usr/bin/env python3
"""Actual rounded bracket planar regions; union extrusion is a separate checkpoint."""
import argparse
import hashlib
import json
import math
from pathlib import Path
from app_harness import application


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--fixture', default=str(Path(__file__).parent / 'workflows/fixtures/sketch-regions.json'))
    args = parser.parse_args()
    fixture = json.loads(Path(args.fixture).read_text())
    output = Path(args.output).resolve()
    captures = []
    capabilities = ['sketch.retained_planar_regions']
    multiprofile = fixture.get('multiprofileExtrusion')
    face_support = fixture.get('faceSupport')
    with application(args.app, output) as (call, transcript):
        initial = call('app.state')
        session = call('sketch.begin', {'plane': 'XY', 'name': 'Three retained bracket regions'})

        def preview():
            return call('command.preview', {'session': session['id']})

        def draft():
            return preview()['authoredDraft']

        def project(u, v):
            point = call('view.projectSketchPoint', {'plane': 'XY', 'uMm': u, 'vMm': v})
            return {'x': point['x'], 'y': point['y']}

        # Explicit setup is distinct from the pointer-inference assertions below.
        vertices = fixture['seedVerticesMm']
        for (x1, y1), (x2, y2) in zip(vertices, vertices[1:]):
            call('sketch.line', {'x1': x1, 'y1': y1, 'x2': x2, 'y2': y2, 'units': 'mm'})
        edges = draft()['parameters']['entities']
        ids = [e['id'] for e in edges]
        call('sketch.constraint', {'type': 'fixed', 'pointIds': [edges[0]['startPointId']]})
        for a, b in fixture['perpendicularPairs']:
            call('sketch.constraint', {'type': 'perpendicular', 'entityIds': [ids[a], ids[b]]})
        for index in fixture['horizontalTargets']:
            call('sketch.constraint', {'type': 'horizontal', 'entityIds': [ids[index]]})
        for a, b in fixture['equalPairs']:
            call('sketch.constraint', {'type': 'equal', 'entityIds': [ids[a], ids[b]]})
        call('sketch.dimension', {'kind': 'horizontal', 'pointIds': [edges[0]['startPointId'], edges[0]['endPointId']], 'value': 500, 'units': 'mm'})
        call('sketch.dimension', {'kind': 'angle', 'entityIds': [ids[0], ids[1]], 'raySigns': [1, 1], 'valueDeg': 45})
        call('sketch.dimension', {'kind': 'linear', 'entityId': ids[2], 'value': 100, 'units': 'mm'})
        cap_dimension = draft()['parameters']['dimensions'][-1]['id']
        call('sketch.construction', {'entityIds': [ids[2], ids[5]], 'construction': True})
        call('view.zoom', {'factor': 0.1})
        origin = project(0, 0)
        viewport = call('app.state')['viewport']
        call('view.pan', {'x': 100 - origin['x'], 'y': viewport['height'] - 100 - origin['y']})

        def points():
            return {p['id']: (p['uMm'], p['vMm']) for p in draft()['parameters']['points']}

        def line(index):
            p = points()
            return p[edges[index]['startPointId']], p[edges[index]['endPointId']]

        circles = []
        for end_index, tangent_index in fixture['endTangentPairs']:
            a, b = line(end_index)
            center = ((a[0]+b[0])/2, (a[1]+b[1])/2)
            a, b = line(tangent_index)
            dx, dy = b[0]-a[0], b[1]-a[1]
            t = ((center[0]-a[0])*dx+(center[1]-a[1])*dy)/(dx*dx+dy*dy)
            foot = (a[0]+t*dx, a[1]+t*dy)
            call('sketch.tool', {'tool': 'circle'})
            before = draft()
            history = call('app.state')['activeSession']['draftHistory']
            snap = call('sketch.pointerMove', project(*center))['snap']
            assert snap['status'] == 'acquired' and snap['target'] == {'kind': 'midpoint', 'entityId': ids[end_index]}
            call('sketch.placePoint', project(*center))
            tangent = call('sketch.pointerMove', project(*foot))['snap']
            assert tangent['status'] == 'acquired' and tangent['target'] == {'kind': 'tangent', 'entityId': ids[tangent_index]}, tangent
            assert draft() == before and call('app.state')['activeSession']['draftHistory'] == history
            captures.append(call('capture.screenshot', {'path': str(output / ('tangent-cue-' + str(end_index) + '.png'))}))
            call('sketch.placePoint', project(*foot))
            circle = draft()['parameters']['entities'][-1]
            assert circle['type'] == 'circle' and math.isclose(circle['radiusMm'], 50, abs_tol=1e-6)
            relations = draft()['parameters']['constraints']
            midpoint = next(c for c in relations if c['type'] == 'midpoint' and c['pointIds'] == [circle['centerPointId']])
            tangent = next(c for c in relations if c['type'] == 'tangent' and c['entityIds'][0] == circle['id'])
            assert midpoint['entityIds'] == [ids[end_index]]
            assert tangent['entityIds'] == [circle['id'], ids[tangent_index]] and tangent['side'] in (-1, 1)
            circles.append((circle, midpoint, tangent, end_index))
        call('sketch.tool', {'tool': 'select'})
        before = draft()
        p = points()
        polygon_area = abs(sum(p[e['startPointId']][0] * p[e['endPointId']][1] - p[e['startPointId']][1] * p[e['endPointId']][0] for e in edges)) / 2
        call('sketch.tool', {'tool': 'fillet'})
        for first, second in fixture['filletPairs']:
            shared = set((edges[first]['startPointId'], edges[first]['endPointId'])) & set((edges[second]['startPointId'], edges[second]['endPointId']))
            assert len(shared) == 1
            call('sketch.placePoint', project(*p[shared.pop()]))
        call('sketch.filletAccept', {'value': fixture['radiusMm'], 'units': 'mm'})
        call('sketch.tool', {'tool': 'select'})
        accepted = draft()

        def regions():
            profiles = preview()['sketchGeometry']['profiles']
            assert len(profiles) == 3, preview()['sketchGeometry'].get('profileDiagnostics')
            assert all(r['valid'] and r['areaMm2'] > 0 and r['sourceEntityIds'] for r in profiles)
            assert all(ids[2] not in r['sourceEntityIds'] and ids[5] not in r['sourceEntityIds'] for r in profiles)
            return profiles

        initial_regions = regions()
        stable_ids = {r['profileId'] for r in initial_regions}
        assert len(stable_ids) == 3
        assert sum(math.isclose(r['areaMm2'], 2500 * math.pi, abs_tol=1e-5) for r in initial_regions) == 2
        # Equal convex/concave fillet segment-area changes cancel for this outline.
        assert math.isclose(sum(r['areaMm2'] for r in initial_regions), polygon_area + 2500 * math.pi, abs_tol=1e-4)
        radius = next(d for d in accepted['parameters']['dimensions'] if d['kind'] == 'radius')
        call('sketch.dimensionUpdate', {'dimensionId': radius['id'], 'value': fixture['editedRadiusMm'], 'units': 'mm'})
        assert {r['profileId'] for r in regions()} == stable_ids
        call('sketch.dimensionUpdate', {'dimensionId': cap_dimension, 'value': fixture['editedCapLengthMm'], 'units': 'mm'})
        final_regions = regions()
        assert {r['profileId'] for r in final_regions} == stable_ids
        assert sum(math.isclose(r['areaMm2'], 3600 * math.pi, abs_tol=1e-5) for r in final_regions) == 2
        edited = draft()
        call('history.undo')
        assert {r['profileId'] for r in regions()} == stable_ids
        call('history.redo')
        assert draft() == edited and {r['profileId'] for r in regions()} == stable_ids
        captures.append(call('capture.screenshot', {'path': str(output / 'three-draft-regions.png')}))
        call('sketch.finish')
        committed = call('document.inspect')
        summaries = committed['evaluation']['sketches'][0]['profiles']
        assert {r['profileId'] for r in summaries} == stable_ids
        params = edited['parameters']
        point_map = {p['id']: (p['uMm'], p['vMm']) for p in params['points']}
        # Pick inside both end disks and halfway along the lower straight region.
        locations = [point_map[e['centerPointId']] for e in params['entities'] if e['type'] == 'circle']
        bottom = params['entities'][0]
        a, b = point_map[bottom['startPointId']], point_map[bottom['endPointId']]
        locations.append(((a[0] + b[0]) / 2, fixture['editedCapLengthMm'] / 2))
        picked_ids = set()
        for index, location in enumerate(locations):
            picked = call('selection.pick', project(*location))
            assert picked['kind'] == 'profile' and picked['sketchId'] == edited['id'], picked
            picked_ids.add(picked['profileId'])
            captures.append(call('capture.screenshot', {'path': str(output / ('selected-region-' + str(index) + '.png'))}))
        assert picked_ids == stable_ids, (picked_ids, stable_ids)
        path = output / 'three-regions.alloy'
        call('document.save', {'path': str(path)})
        saved = call('document.inspect')['authored']
        call('document.new', {'discard': False})
        call('document.open', {'path': str(path)})
        reopened = call('document.inspect')
        if reopened['authored'] != saved:
            (output / 'region-reopen-authored-mismatch.json').write_text(json.dumps({'saved': saved, 'reopened': reopened['authored']}, indent=2) + '\n')
        assert reopened['authored'] == saved, 'Authored reopen mismatch: see region-reopen-authored-mismatch.json'
        restored = reopened['evaluation']['sketches'][0]['profiles']
        assert {r['profileId'] for r in restored} == stable_ids
        assert math.isclose(sum(r['areaMm2'] for r in restored), sum(r['areaMm2'] for r in final_regions), abs_tol=1e-5)
        if multiprofile:
            source = call('document.inspect')
            depth = multiprofile['distanceMm']
            call('solid.extrude', {'sketchId': edited['id'], 'profileIds': sorted(stable_ids), 'distance': depth, 'units': 'mm'})
            extruded = call('document.inspect')
            assert extruded['bodies'] == 1
            body_feature = next(f for f in extruded['authored']['features'] if f['type'] == 'extrude')
            retained_identity = (body_feature['id'], body_feature['bodyId'])
            assert body_feature['parameters']['profileIds'] == sorted(stable_ids)

            def inspect_solid(document):
                body = document['evaluation']['bodies'][0]
                profiles = document['evaluation']['sketches'][0]['profiles']
                area = sum(r['areaMm2'] for r in profiles)
                assert {r['profileId'] for r in profiles} == stable_ids
                assert body['valid'] and body['closed'] and body['topology']['solids'] == 1
                assert set(body['sourceProfileIds']) == stable_ids
                assert math.isclose(body['sourceAreaMm2'], area, rel_tol=1e-9, abs_tol=1e-5)
                assert math.isclose(body['volumeMm3'], area * depth, rel_tol=1e-9, abs_tol=1e-5)
                feature = next(f for f in document['authored']['features'] if f['type'] == 'extrude')
                assert (feature['id'], feature['bodyId']) == retained_identity
                return body['volumeMm3']

            old_volume = inspect_solid(extruded)
            assert extruded['undo']['count'] == source['undo']['count'] + 1
            captures.append(call('capture.screenshot', {'path': str(output / 'three-region-union-extrusion.png')}))
            call('sketch.edit', {'sketchId': edited['id']})
            call('sketch.dimensionUpdate', {'dimensionId': cap_dimension, 'value': multiprofile['upstreamCapLengthMm'], 'units': 'mm'})
            call('sketch.finish')
            changed = call('document.inspect')
            assert not math.isclose(inspect_solid(changed), old_volume, rel_tol=1e-6)
            call('history.undo')
            assert call('document.inspect')['authored'] == extruded['authored']
            assert math.isclose(inspect_solid(call('document.inspect')), old_volume, rel_tol=1e-9)
            call('history.redo')
            assert call('document.inspect')['authored'] == changed['authored']
            inspect_solid(call('document.inspect'))
            captures.append(call('capture.screenshot', {'path': str(output / 'union-upstream-edit.png')}))
            path = output / 'three-region-extrusion.alloy'
            call('document.save', {'path': str(path)})
            saved_solid = call('document.inspect')['authored']
            call('document.new', {'discard': False})
            call('document.open', {'path': str(path)})
            reopened_solid = call('document.inspect')
            assert reopened_solid['authored'] == saved_solid
            inspect_solid(reopened_solid)
            capabilities.append('solid.multi_profile_union')
        if face_support:
            assert multiprofile, 'Face checkpoint requires the rounded bracket solid setup'
            before_face = call('document.inspect')
            cap = before_face['evaluation']['bodies'][0]['supportFaces'][0]
            assert cap['role'] == 'extrude.endCap' and cap['planar']
            support = {key: cap[key] for key in ('producerFeatureId', 'bodyId', 'faceId')}
            assert cap['frame']['origin'] == [0, 0, 50]
            # A stale semantic reference must not silently select another planar face.
            stale = dict(support, faceId='00000000-0000-4000-8000-000000000001')
            call('selection.faceSet', {'support': stale}, error='support_unavailable')
            assert call('document.inspect')['authored'] == before_face['authored']
            selected_cap = call('selection.faceSet', {'support': support})
            assert selected_cap['kind'] == 'face' and selected_cap['support'] == support
            face_session = call('sketch.create', {'name': 'Supported local rectangle'})
            assert face_session['resolvedFrame'] == cap['frame']
            x1, y1, x2, y2 = face_support['rectangleMm']
            call('sketch.rectangle', {'x1': x1, 'y1': y1, 'x2': x2, 'y2': y2, 'units': 'mm'})
            face_preview = call('command.preview', {'session': face_session['id']})
            local = face_preview['authoredDraft']
            assert local['parameters']['plane'] == 'face'
            assert local['parameters']['support'] == support and 'frame' not in local['parameters']
            call('view.projectSketchPoint', {'sketchId': local['id'], 'uMm': x1, 'vMm': y1})
            captures.append(call('capture.screenshot', {'path': str(output / 'cap-local-rectangle-preview.png')}))
            call('sketch.finish')
            attached = call('document.inspect')

            def inspect_attached(document, z):
                authored = next(f for f in document['authored']['features'] if f['id'] == local['id'])
                assert authored == local, 'Support reevaluation changed local authored coordinates or IDs'
                evaluated = next(f for f in document['evaluation']['sketches'] if f['sketchId'] == local['id'])
                assert evaluated['frame']['origin'] == [0, 0, z]
                assert evaluated['frame']['uAxis'] == cap['frame']['uAxis']
                assert evaluated['frame']['vAxis'] == cap['frame']['vAxis']
                coordinates = {p['id']: (p['uMm'], p['vMm']) for p in local['parameters']['points']}
                for point in evaluated['worldPoints']:
                    u, v = coordinates[point['id']]
                    assert all(math.isclose(a, b, abs_tol=1e-7) for a, b in zip(point['positionMm'], (u, v, z)))
                assert len(evaluated['profiles']) == 1
                assert math.isclose(evaluated['profiles'][0]['areaMm2'], (x2-x1)*(y2-y1), abs_tol=1e-7)
                current_cap = document['evaluation']['bodies'][0]['supportFaces'][0]
                assert {key: current_cap[key] for key in support} == support
                return evaluated['profiles'][0]['profileId']

            face_profile = inspect_attached(attached, 50)
            depth = face_support['editedDepthMm']
            call('solid.extrude', {'featureId': retained_identity[0], 'sketchId': edited['id'],
                'profileIds': sorted(stable_ids), 'distance': depth, 'units': 'mm'})
            transported = call('document.inspect')
            assert inspect_attached(transported, depth) == face_profile
            inspect_solid(transported)
            call('view.projectSketchPoint', {'sketchId': local['id'], 'uMm': x2, 'vMm': y2})
            captures.append(call('capture.screenshot', {'path': str(output / 'cap-depth-transport.png')}))
            call('history.undo')
            undone = call('document.inspect')
            assert undone['authored'] == attached['authored']
            inspect_attached(undone, 50)
            call('history.redo')
            assert call('document.inspect')['authored'] == transported['authored']
            path = output / 'supported-bracket.alloy'
            call('document.save', {'path': str(path)})
            saved_face = call('document.inspect')['authored']
            call('document.new', {'discard': False})
            call('document.open', {'path': str(path)})
            reopened_face = call('document.inspect')
            assert reopened_face['authored'] == saved_face
            assert inspect_attached(reopened_face, depth) == face_profile
            inspect_solid(reopened_face)
            capabilities.append('sketch.semantic_cap_support')
        final = call('app.state')
    result = {
        'result': 'passed', 'testSha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'fixtureSha256': hashlib.sha256(Path(args.fixture).read_bytes()).hexdigest(),
        'verifiedCapabilities': capabilities, 'covers': [],
        'initialState': initial, 'finalState': final, 'transcript': transcript, 'captures': captures,
        'evidenceDimensions': {'api': {'status': 'passed'}, 'geometry': {'status': 'passed'},
            'save_reopen': {'status': 'passed'}, 'visual': {'status': 'captured'},
            'physical_input': {'status': 'not_run', 'reason': 'Shared pointer API dispatch is separate from native mouse evidence.'}},
        'limits': ['Positive extrusion end-cap support only; arbitrary planar/curved or modified faces remain unsupported.'] if face_support else ['Bounded authored-contact regions and connected same-sketch union only.'] if multiprofile else ['Bounded authored-contact regions; multi-profile union extrusion remains separate.']}
    (output / ('sketch-face-support-api.json' if face_support else 'sketch-multiprofile-api.json' if multiprofile else 'sketch-regions-api.json')).write_text(json.dumps(result, indent=2) + '\n')
    print('PASS: three selectable rounded-bracket regions, retained edit identities and storage')


if __name__ == '__main__':
    main()
