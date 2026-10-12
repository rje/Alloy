#!/usr/bin/env python3
"""One-candidate illustrated bracket API chain; physical tutorial input is separate evidence."""
import argparse
import hashlib
import json
import math
from pathlib import Path
from app_harness import application


def near(actual, expected):
    assert math.isclose(actual, expected, rel_tol=1e-8, abs_tol=1e-4), (actual, expected)


def intersection(a, b, c, d):
    u, v = (b[0]-a[0], b[1]-a[1]), (d[0]-c[0], d[1]-c[1])
    det = u[0]*v[1]-u[1]*v[0]
    assert abs(det) > 1e-8
    t = ((c[0]-a[0])*v[1]-(c[1]-a[1])*v[0])/det
    return a[0]+t*u[0], a[1]+t*u[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--fixture', default=str(Path(__file__).parent / 'workflows/fixtures/tutorial-bracket.json'))
    args = parser.parse_args()
    fixture_path = Path(args.fixture)
    f = json.loads(fixture_path.read_text())
    assert f['case'] == 'tutorial-bracket'
    output = Path(args.output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    stages, captures, transcript = [], [], []
    receipt = {'result': 'failed', 'covers': [], 'fixtureSha256': hashlib.sha256(fixture_path.read_bytes()).hexdigest(),
               'sourceReferenceIds': f['sourceReferenceIds'], 'stages': stages, 'captures': captures,
               'limits': ['API commands share application state and pointer placement; no physical keys or mouse input are claimed.',
                          'Near-elbow seed and terminal circular-center snap interpretation are explicit source-informed inference.',
                          'Fillet removes the 500 mm dimension; final longitudinal size is evaluated, not assumed uniquely specified.',
                          'Captures require separate visual review; source New Design/tab interaction is outside this API case.']}
    try:
        with application(args.app, output) as (call, transcript):
            call('window.resize', {'width': 1600, 'height': 900})
            receipt['initialState'] = call('app.state')
            assert receipt['initialState']['document']['features'] == 0
            session = call('sketch.begin', {'plane': 'XY', 'name': 'Illustrated bracket outline', 'units': 'mm'})

            def preview():
                return call('command.preview', {'session': session['id']})

            def draft():
                return preview()['authoredDraft']

            def points():
                return {p['id']: (p['uMm'], p['vMm']) for p in draft()['parameters']['points']}

            def project(u, v):
                p = call('view.projectSketchPoint', {'sketchId': draft()['id'], 'uMm': u, 'vMm': v})
                return {'x': p['x'], 'y': p['y']}

            def frame_view():
                call('view.zoom', {'factor': 0.1})
                origin = project(0, 0)
                viewport = call('app.state')['viewport']
                call('view.pan', {'x': 100-origin['x'], 'y': viewport['height']-100-origin['y']})

            def checkpoint(step, **details):
                captures.append(call('capture.screenshot', {'path': str(output / (step + '.png'))}))
                state = call('app.state')
                stages.append({'step': step, 'status': 'matched_api', 'activeSession': state['activeSession'],
                               'document': state['document'], 'gpu': state['viewport']['gpu'], **details})

            def apply_pairs(kind, targets):
                # Activating a constraint applies an existing complete selection.
                # Start empty so only the following intended pairs are authored.
                call('sketch.selectionSet', {'entityIds': [], 'pointIds': []})
                call('sketch.constraintTool', {'type': kind})
                for a, b in targets:
                    call('sketch.selectionSet', {'entityIds': [a]})
                    result = call('sketch.selectionSet', {'entityIds': [a, b]})
                    assert result['constraintApplied'] and result['constraintType'] == kind
                call('sketch.tool', {'tool': 'select'})

            frame_view()
            call('sketch.tool', {'tool': 'line'})
            for uv in f['seedVerticesMm']:
                call('sketch.placePoint', project(*uv))
            call('sketch.tool', {'tool': 'select'})
            edges = draft()['parameters']['entities']
            assert len(edges) == 6
            ids = [e['id'] for e in edges]
            assert draft()['parameters']['originPointId']
            checkpoint('a1-01-outline')
            apply_pairs('perpendicular', [(ids[a], ids[b]) for a, b in f['perpendicularPairs']])
            checkpoint('a1-02-perpendicular')
            call('sketch.constraintTool', {'type': 'horizontalVertical'})
            for index in f['horizontalTargets']:
                assert call('sketch.selectionSet', {'entityIds': [ids[index]]})['constraintApplied']
            call('sketch.tool', {'tool': 'select'})
            checkpoint('a1-03-horizontal')
            apply_pairs('equal', [(ids[a], ids[b]) for a, b in f['equalPairs']])
            checkpoint('a1-04-equal')

            def line(index):
                p = points()
                e = next(e for e in draft()['parameters']['entities'] if e['id'] == ids[index])
                return p[e['startPointId']], p[e['endPointId']]

            def dimension(targets, placement, value, kind):
                call('sketch.tool', {'tool': 'dimension'})
                call('sketch.selectionSet', {'entityIds': targets})
                pending = call('sketch.pointerMove', placement)
                assert pending['kind'] == kind, pending
                call('sketch.dimensionPlace', placement)
                call('sketch.dimensionAccept', {'valueDeg' if kind == 'angle' else 'value': value})
                call('sketch.tool', {'tool': 'select'})
                authored = draft()['parameters']['dimensions'][-1]
                assert authored['kind'] == kind
                return authored['id']

            a, b = line(0)
            lower = project((a[0]+b[0])/2, (a[1]+b[1])/2)
            lower['y'] += 35
            lower_id = dimension([ids[0]], lower, f['lowerLengthMm'], 'horizontal')
            joint = line(0)[1]
            dimension([ids[0], ids[1]], project(joint[0]+150, joint[1]+50), f['angleDeg'], 'angle')
            a, b = line(2)
            ap, bp = project(*a), project(*b)
            dx, dy = bp['x']-ap['x'], bp['y']-ap['y']
            length = math.hypot(dx, dy)
            placement = {'x': (ap['x']+bp['x'])/2-dy*35/length, 'y': (ap['y']+bp['y'])/2+dx*35/length}
            dimension([ids[2]], placement, f['capLengthMm'], 'linear')
            assert preview()['sketchGeometry']['solver']['dof'] == 0
            for index in (0, 1):
                a, b = line(index)
                near(math.dist(a, b), f['lowerLengthMm'])
            checkpoint('a1-05-dimensions-fully-constrained')

            # Activity 2 continues the same sketch: terminal construction and circles.
            call('sketch.selectionSet', {'entityIds': [ids[2], ids[5]]})
            call('sketch.construction', {'construction': True})
            end_circles = []
            for end, tangent in f['endTangentPairs']:
                a, b = line(end)
                center = ((a[0]+b[0])/2, (a[1]+b[1])/2)
                a, b = line(tangent)
                dx, dy = b[0]-a[0], b[1]-a[1]
                t = ((center[0]-a[0])*dx+(center[1]-a[1])*dy)/(dx*dx+dy*dy)
                contact = (a[0]+t*dx, a[1]+t*dy)
                call('sketch.tool', {'tool': 'circle'})
                before = draft()
                snap = call('sketch.pointerMove', project(*center))['snap']
                assert snap['target'] == {'kind': 'midpoint', 'entityId': ids[end]}, snap
                call('sketch.placePoint', project(*center))
                snap = call('sketch.pointerMove', project(*contact))['snap']
                assert snap['target'] == {'kind': 'tangent', 'entityId': ids[tangent]}, snap
                assert draft() == before
                call('sketch.placePoint', project(*contact))
                circle = draft()['parameters']['entities'][-1]
                near(circle['radiusMm'], f['capLengthMm']/2)
                end_circles.append(circle['id'])
            call('sketch.tool', {'tool': 'select'})
            checkpoint('a2-01-construction-and-end-circles')
            call('sketch.tool', {'tool': 'fillet'})
            for first, second in f['filletPairs']:
                call('sketch.placePoint', project(*line(first)[1]))
            call('sketch.filletAccept', {'value': f['radiusMm'], 'units': 'mm'})
            call('sketch.tool', {'tool': 'select'})
            rounded = draft()
            assert lower_id not in {d['id'] for d in rounded['parameters']['dimensions']}
            assert sum(e['type'] == 'arc' for e in rounded['parameters']['entities']) == 2
            regions = preview()['sketchGeometry']['profiles']
            assert len(regions) == 3 and all(r['valid'] for r in regions)
            # Independent area from current analytic straight-line intersections.
            # Equal opposite 45-degree fillets add/remove equal area, hence cancel.
            polygon = [line(0)[0], intersection(*line(0), *line(1)), line(1)[1],
                       line(3)[0], intersection(*line(3), *line(4)), line(4)[1]]
            polygon_area = abs(sum(a[0]*b[1]-b[0]*a[1] for a, b in zip(polygon, polygon[1:]+polygon[:1])))/2
            stock_area = polygon_area + math.pi*(f['capLengthMm']/2)**2
            near(sum(r['areaMm2'] for r in regions), stock_area)
            checkpoint('a2-02-fillets-three-regions', independentAreaMm2=stock_area)
            source_id = rounded['id']
            profile_ids = sorted(r['profileId'] for r in regions)
            call('sketch.finish')
            call('solid.pressPullDialog', {'sketchId': source_id})
            for index, profile_id in enumerate(profile_ids):
                call('selection.profileSet', {'sketchId': source_id, 'profileId': profile_id, 'additive': index > 0})
            extrusion_session = call('app.state')['activeSession']['id']
            call('command.update', {'session': extrusion_session, 'inputs': {'distance': f['extrusionMm'], 'units': 'mm'}})
            call('command.preview', {'session': extrusion_session})
            call('command.commit', {'session': extrusion_session})
            stock = call('document.inspect')
            body = stock['evaluation']['bodies'][0]
            assert stock['bodies'] == 1 and body['valid'] and body['closed']
            near(body['volumeMm3'], stock_area*f['extrusionMm'])
            checkpoint('a2-03-three-region-extrusion')

            cap = body['supportFaces'][0]
            support = {key: cap[key] for key in ('producerFeatureId', 'bodyId', 'faceId')}
            call('sketch.create', {'name': 'Illustrated bracket guides and holes'})
            call('selection.faceSet', {'support': support})
            session = call('sketch.begin', {'support': support, 'name': 'Illustrated bracket guides and holes', 'units': 'mm'})
            active = call('app.state')['activeSession']
            context = active['projectionContext']
            centers = [next(p for p in context['points'] if p['source']['sourceEntityId'] == cid) for cid in end_circles]
            centers.sort(key=lambda p: p['uMm'])
            references = []
            for source_id in ids[:2]:
                edge = next(e for e in cap['edges'] if e['sourceEntityId'] == source_id)
                references.append(next(r for r in context['lines'] if r['source']['edgeId'] == edge['edgeId']))
            p, q = centers
            ref0, ref1 = references
            # Only seed a near-elbow location; two retained Parallel relations
            # determine it from the anchored centers, not frozen typed coordinates.
            bend = intersection((p['uMm'], p['vMm']),
                                (p['uMm']+ref0['u2Mm']-ref0['u1Mm'], p['vMm']+ref0['v2Mm']-ref0['v1Mm']),
                                (q['uMm'], q['vMm']),
                                (q['uMm']+ref1['u2Mm']-ref1['u1Mm'], q['vMm']+ref1['v2Mm']-ref1['v1Mm']))
            frame_view()
            call('sketch.tool', {'tool': 'line'})
            for uv, expected in [((p['uMm'], p['vMm']), p['id']), ((bend[0]+7, bend[1]+8), None),
                                 ((q['uMm'], q['vMm']), q['id'])]:
                snap = call('sketch.pointerMove', project(*uv))['snap']
                if expected:
                    assert snap['target'] == {'kind': 'point', 'pointId': expected}, snap
                call('sketch.placePoint', project(*uv))
            call('sketch.tool', {'tool': 'select'})
            guides = draft()['parameters']['entities']
            assert len(guides) == 2 and all(e['type'] == 'line' for e in guides)
            guide_ids = [e['id'] for e in guides]
            call('sketch.selectionSet', {'entityIds': guide_ids})
            call('sketch.construction', {'construction': True})
            apply_pairs('parallel', [(guide_ids[i], references[i]['id']) for i in range(2)])
            guides_draft = draft()
            assert len(guides_draft['parameters']['pointReferences']) == 2
            assert preview()['sketchGeometry']['solver']['dof'] == 0
            checkpoint('a2-04-anchored-construction-guides')

            pp = points()
            start = pp[guides[0]['startPointId']]
            joint = pp[guides[0]['endPointId']]
            end = pp[guides[1]['endPointId']]
            centers = [(start, {'kind': 'point', 'pointId': guides[0]['startPointId']}),
                       (((start[0]+joint[0])/2, (start[1]+joint[1])/2), {'kind': 'midpoint', 'entityId': guide_ids[0]}),
                       (joint, None),
                       (((joint[0]+end[0])/2, (joint[1]+end[1])/2), {'kind': 'midpoint', 'entityId': guide_ids[1]}),
                       (end, {'kind': 'point', 'pointId': guides[1]['endPointId']})]
            circles = []
            for index, (center, expected) in enumerate(centers):
                call('sketch.tool', {'tool': 'circle'})
                before = draft()
                history = call('app.state')['activeSession']['draftHistory']
                snap = call('sketch.pointerMove', project(*center))['snap']
                if expected:
                    assert snap['target'] == expected, snap
                else:
                    assert snap['target']['kind'] == 'point'
                    assert snap['target']['pointId'] in (guides[0]['endPointId'], guides[1]['startPointId']), snap
                assert draft() == before and call('app.state')['activeSession']['draftHistory'] == history
                call('sketch.placePoint', project(*center))
                if index == 0:
                    call('sketch.circleDiameterPreview', {'text': str(f['holeDiameterMm'])})
                    assert draft() == before and call('app.state')['activeSession']['draftHistory'] == history
                    call('sketch.circleDiameterAccept')
                else:
                    call('sketch.placePoint', project(center[0]+f['holeDiameterMm']*(0.32+0.025*index), center[1]))
                circle = draft()['parameters']['entities'][-1]
                assert circle['type'] == 'circle'
                circles.append(circle['id'])
            call('sketch.tool', {'tool': 'select'})
            apply_pairs('equal', list(zip(circles, circles[1:])))
            holes = draft()
            parameters = holes['parameters']
            diameters = [d for d in parameters['dimensions'] if d['kind'] == 'diameter' and d['driving']]
            assert len(diameters) == 1
            near(diameters[0]['valueMm'], f['holeDiameterMm'])
            assert [(c['entityIds']) for c in parameters['constraints'] if c['type'] == 'equal'] == [list(pair) for pair in zip(circles, circles[1:])]
            assert sum(c['type'] == 'midpoint' for c in parameters['constraints']) == 2
            for e in parameters['entities']:
                if e['type'] == 'circle':
                    near(e['radiusMm'], f['holeDiameterMm']/2)
            assert preview()['sketchGeometry']['solver']['dof'] == 0
            hole_profiles = sorted(r['profileId'] for r in preview()['sketchGeometry']['profiles'])
            assert len(hole_profiles) == 5
            checkpoint('a3-01-five-equal-circles')
            call('sketch.finish')
            # API profile collection mirrors the five native region choices;
            # the parent retains independent physical selection/arrow evidence.
            call('solid.pressPullDialog', {'sketchId': holes['id']})
            for index, profile_id in enumerate(hole_profiles):
                call('selection.profileSet', {'sketchId': holes['id'], 'profileId': profile_id, 'additive': index > 0})
            cut_session = call('app.state')['activeSession']['id']
            call('command.update', {'session': cut_session, 'inputs': {'distance': f['cutDistanceMm'], 'units': 'mm',
                                                                     'operation': 'cut', 'targetBodyIds': [body['bodyId']]}})
            call('command.preview', {'session': cut_session})
            checkpoint('a3-02-five-profile-cut-preview')
            gpu = call('app.state')['viewport']['gpu']
            assert gpu['cutPreviewToolCount'] == 5 and gpu['previewReplacedBodyId'] == body['bodyId']
            assert gpu['extrusionCue']['operation'] == 'cut'
            near(gpu['extrusionCue']['distanceMm'], f['cutDistanceMm'])
            before_cut = call('document.inspect')['authored']
            call('command.commit', {'session': cut_session})
            finished = call('document.inspect')
            assert finished['features'] == 4 and finished['bodies'] == 1
            solid = finished['evaluation']['bodies'][0]
            assert solid['valid'] and solid['closed'] and solid['topology']['solids'] == 1
            assert solid['bodyId'] == body['bodyId'] and solid['featureId'] == body['featureId']
            assert solid['sourceProfileIds'] == hole_profiles or sorted(solid['sourceProfileIds']) == hole_profiles
            removed = 5*math.pi*(f['holeDiameterMm']/2)**2*f['extrusionMm']
            near(solid['removedVolumeMm3'], removed)
            near(solid['volumeMm3'], stock_area*f['extrusionMm']-removed)
            cut = finished['authored']['features'][-1]
            assert cut['parameters']['operation'] == 'cut' and 'bodyId' not in cut
            checkpoint('a3-03-finished-five-holes', independentRemovedVolumeMm3=removed,
                       independentVolumeMm3=stock_area*f['extrusionMm']-removed)
            # Editing this pre-Cut sketch must display its real history-prefix
            # stock, without mutating the retained final Cut or body visibility.
            call('sketch.edit', {'sketchId': holes['id']})
            checkpoint('historical-edit-stock')
            edit_state = call('app.state')
            edit_gpu = edit_state['viewport']['gpu']
            assert edit_gpu['displayedBodyIds'] == [body['bodyId']], edit_gpu
            assert edit_gpu['bodyStageKey'].startswith('sketch-prefix:')
            assert not edit_gpu['editDisplay']['hiddenBodyIds']
            assert edit_state['document']['authored'] == finished['authored']
            call('sketch.cancel')
            captures.append(call('capture.screenshot', {'path': str(output / 'historical-edit-cancel-restored.png')}))
            restored_gpu = call('app.state')['viewport']['gpu']
            assert restored_gpu['displayedBodyIds'] == [body['bodyId']]
            assert not restored_gpu['bodyStageKey']
            assert call('document.inspect')['authored'] == finished['authored']
            call('history.undo')
            assert call('document.inspect')['authored'] == before_cut
            call('history.redo')
            assert call('document.inspect')['authored'] == finished['authored']
            call('document.save', {'path': str(output / 'illustrated-bracket.alloy')})
            saved = call('document.inspect')
            call('document.new', {'discard': False})
            call('document.open', {'path': str(output / 'illustrated-bracket.alloy')})
            reopened = call('document.inspect')
            assert reopened['authored'] == saved['authored']
            near(reopened['evaluation']['bodies'][0]['volumeMm3'], solid['volumeMm3'])
            checkpoint('storage-undo-redo-reopen')
            receipt['result'] = 'passed'
            receipt['finalState'] = call('app.state')
            receipt['verifiedCapabilities'] = ['tutorial.illustrated_bracket.api_chain']
            receipt['evidenceDimensions'] = {
                'api': {'status': 'passed', 'reason': 'One real application completed the shared-command tutorial chain.'},
                'geometry': {'status': 'passed', 'reason': 'Independent analytic area/volume checks and retained constraints/solid summaries passed.'},
                'save_reopen': {'status': 'passed', 'reason': 'Exact authored undo/redo and reopened save data matched.'},
                'visual': {'status': 'captured', 'reason': 'Source-step captures require independent visual compatibility review.'},
                'physical_input': {'status': 'not_run', 'reason': 'API pointer commands do not establish physical keyboard/mouse or menu input.'}}

    finally:
        receipt['transcript'] = transcript
        (output / 'tutorial-bracket-api.json').write_text(json.dumps(receipt, indent=2)+'\n')
    print(json.dumps({'result': receipt['result'], 'receipt': str(output / 'tutorial-bracket-api.json')}))


if __name__ == '__main__':
    main()
