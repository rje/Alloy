#!/usr/bin/env python3
"""GEO-026 independent retained Point/Line Axis real-entrypoint verification.

Run only with the private application lease. Numeric orientation and display
extension are Alloy policies, not a claim of public-reference parity.
"""
import argparse
import json
import math
from pathlib import Path
from app_harness import application


def vector(actual, expected, tolerance=1e-7):
    assert math.dist(actual, expected) < tolerance, (actual, expected)


def check_axis(axis, first, second):
    delta = [b-a for a, b in zip(first, second)]
    length = math.sqrt(sum(x*x for x in delta))
    expected = [x/length for x in delta]
    vector(axis['originMm'], first)
    vector(axis['direction'], expected)
    assert math.isclose(axis['sourceSpanMm'], length, abs_tol=1e-7), axis
    residuals = []
    for point in (first, second):
        displacement = [p-o for p, o in zip(point, axis['originMm'])]
        projection = sum(x*y for x, y in zip(displacement, axis['direction']))
        residuals.append(math.dist(displacement, [projection*x for x in axis['direction']]))
    assert max(residuals) < 1e-7, residuals
    return {'actual': axis, 'expectedOriginMm': first, 'expectedDirection': expected,
            'expectedSpanMm': length, 'maxSourceIncidenceResidualMm': max(residuals)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    out = Path(args.output).resolve()
    out.mkdir(parents=True, exist_ok=True)
    with application(args.app, out, command_timeout=90) as (call, transcript):
        try:
            inspect = lambda: call('document.inspect')
            draft = lambda: call('app.state')['activeSession']['draftSketch']
            def feature(state, fid):
                return next(f for f in state['authored']['features'] if f['id'] == fid)
            def axis(state, fid):
                return next(a for a in state['evaluation']['constructionAxes'] if a['featureId'] == fid)
            def unchanged(before):
                after = inspect()
                for key in ('authored', 'evaluation', 'undo', 'revision'):
                    assert after[key] == before[key], key
            def capture(name):
                call('capture.screenshot', {'path': str(out/(name+'.png'))})
            call('window.resize', {'width': 1280, 'height': 900})
            initial_environment = call('app.state')
            assert initial_environment['viewport']['gpu']['hardwareAccelerated']
            assert 'construction.axis' in {c['id'] for c in call('commands.list')['commands']}
            plane_id = call('construction.plane', {'source': {'plane': 'XY'}, 'offset': 7,
                                                  'units': 'mm', 'name': 'Source z7'})['planeId']
            call('sketch.create', {'planeId': plane_id, 'name': 'Authored sources'})
            for x, y in ((4, 3), (16, 12), (4, 3)):
                call('sketch.addPoint', {'x': x, 'y': y, 'units': 'mm'})
            call('sketch.line', {'x1': -10, 'y1': -10, 'x2': -10, 'y2': 10, 'units': 'mm'})
            call('sketch.addCircle', {'centerX': 25, 'centerY': -10, 'diameter': 8, 'units': 'mm'})
            source = draft()
            points = [e for e in source['parameters']['entities'] if e['type'] == 'point']
            line, = [e for e in source['parameters']['entities'] if e['type'] == 'line']
            circle, = [e for e in source['parameters']['entities'] if e['type'] == 'circle']
            sid = source['id']
            refs = [{'sketchId': sid, 'entityId': p['id'], 'pointId': p['pointId']} for p in points]
            line_ref = {k: line[k] for k in ('startPointId', 'endPointId')}
            line_ref.update(sketchId=sid, entityId=line['id'])
            call('sketch.finish')
            original = inspect()
            point_inputs = {'source': {'points': refs[:2]}, 'name': 'Point Axis'}
            line_inputs = {'source': {'line': line_ref}, 'name': 'Line Axis'}
            measurements = {}
            created_ids = []
            for label, inputs, first, second in (
                    ('points', point_inputs, [4, 3, 7], [16, 12, 7]),
                    ('line', line_inputs, [-10, -10, 7], [-10, 10, 7])):
                before = inspect()
                session = call('command.begin', {'command': 'construction.axis', 'inputs': {}})
                assert not session['previewed']
                assert call('app.state')['ui']['activeDialog']['visible']
                call('command.update', {'session': session['id'], 'inputs': inputs})
                preview = call('command.preview', {'session': session['id']})
                measurements[label+'Preview'] = check_axis(preview['previewAxis'], first, second)
                unchanged(before)
                call('view.orientation', {'name': 'isometric'})
                call('view.fit')
                capture(label+'-preview')
                call('capture.dialog', {'path': str(out/(label+'-panel.png'))})
                call('command.cancel', {'session': session['id']})
                unchanged(before)
                session = call('command.begin', {'command': 'construction.axis', 'inputs': inputs})
                call('command.preview', {'session': session['id']})
                call('command.commit', {'session': session['id']})
                created = inspect()
                fid = created['authored']['features'][-1]['id']
                created_ids.append(fid)
                assert feature(created, fid)['type'] == 'constructionAxis'
                assert feature(created, fid)['parameters']['source'] == inputs['source']
                measurements[label+'Committed'] = check_axis(axis(created, fid), first, second)
                assert created['evaluation']['bodies'] == original['evaluation']['bodies'] == []
                call('history.undo')
                assert inspect()['authored'] == before['authored']
                call('history.redo')
                assert inspect()['authored'] == created['authored']
            point_axis, line_axis = created_ids
            # Same-feature edit changes source ordering while preserving exact refs.
            edited_inputs = {**point_inputs, 'featureId': point_axis, 'source': {'points': list(reversed(refs[:2]))}}
            before_edit = inspect()
            call('construction.axis', edited_inputs)
            edited = inspect()
            assert len(edited['authored']['features']) == len(before_edit['authored']['features'])
            measurements['sameFeatureEdit'] = check_axis(axis(edited, point_axis), [16, 12, 7], [4, 3, 7])
            call('feature.editDialog', {'featureId': point_axis})
            session = call('app.state')['activeSession']
            assert session['inputs']['source'] == edited_inputs['source']
            call('command.update', {'session': session['id'], 'inputs': point_inputs})
            call('command.preview', {'session': session['id']})
            call('command.cancel', {'session': session['id']})
            unchanged(edited)
            call('feature.editDialog', {'featureId': line_axis})
            line_session = call('app.state')['activeSession']
            assert line_session['inputs']['source'] == line_inputs['source']
            call('command.update', {'session': line_session['id'], 'inputs': {'name': 'Edited Line Axis'}})
            call('command.preview', {'session': line_session['id']})
            call('capture.dialog', {'path': str(out/'line-edit-panel.png')})
            call('command.commit', {'session': line_session['id']})
            line_edited = inspect()
            assert feature(line_edited, line_axis)['name'] == 'Edited Line Axis'
            assert feature(line_edited, line_axis)['parameters']['source'] == line_inputs['source']
            assert len(line_edited['authored']['features']) == len(edited['authored']['features'])
            call('history.undo'); assert inspect()['authored'] == edited['authored']
            call('history.redo'); assert inspect()['authored'] == line_edited['authored']
            edited = line_edited
            # Exact source point and Line endpoint edits recompute both datums.
            call('sketch.edit', {'sketchId': sid})
            call('sketch.movePoint', {'pointId': points[1]['pointId'], 'x': 20, 'y': 15, 'units': 'mm'})
            call('sketch.movePoint', {'pointId': line['endPointId'], 'x': 0, 'y': 10, 'units': 'mm'})
            call('sketch.finish')
            changed = inspect()
            measurements['pointSourceEdit'] = check_axis(axis(changed, point_axis), [20, 15, 7], [4, 3, 7])
            measurements['lineSourceEdit'] = check_axis(axis(changed, line_axis), [-10, -10, 7], [0, 10, 7])
            assert feature(changed, point_axis)['parameters']['source'] == edited_inputs['source']
            assert feature(changed, line_axis)['parameters']['source'] == line_inputs['source']
            call('history.undo'); assert inspect()['authored'] == edited['authored']
            call('history.redo'); assert inspect()['authored'] == changed['authored']
            changed = inspect()
            # Source removal and source-driven coincidence must not replace the
            # committed history; the invalid draft remains cancellable.
            source_refusals = []
            for operation, args in (
                    ('sketch.deleteSelection', {'entityIds': [points[1]['id']]}),
                    ('sketch.movePoint', {'pointId': points[1]['pointId'], 'x': 4, 'y': 3, 'units': 'mm'})):
                call('sketch.edit', {'sketchId': sid})
                call(operation, args)
                call('sketch.finish', error='evaluation_failed')
                source_refusals.append(transcript[-1]['response']['error'])
                unchanged(changed)
                active = call('app.state')['activeSession']
                call('command.cancel', {'session': active['id']})
                unchanged(changed)
            errors = []
            bad_sources = [
                {'points': [refs[0], refs[2]]},  # distinct authored IDs, coincident geometry
                {'points': [refs[0], refs[0]]},
                {'points': [refs[0], {**refs[1], 'pointId': '00000000-0000-0000-0000-000000000001'}]},
                {'points': [refs[0], {**refs[1], 'entityId': line['id'], 'pointId': line['startPointId']}]},
                {'line': {**line_ref, 'entityId': circle['id']}},
                {'line': {**line_ref, 'startPointId': line_ref['endPointId'], 'endPointId': line_ref['startPointId']}},
                {'line': {**line_ref, 'entityId': '00000000-0000-0000-0000-000000000001'}},
            ]
            for bad in bad_sources:
                call('construction.axis', {'featureId': point_axis, 'source': bad}, error='evaluation_failed')
                errors.append(transcript[-1]['response']['error'])
                unchanged(changed)
            call('feature.visibility', {'featureId': point_axis, 'visible': False})
            hidden = inspect()
            assert not axis(hidden, point_axis)['visible'] and not feature(hidden, point_axis)['visible']
            capture('point-axis-hidden')
            call('feature.visibility', {'featureId': point_axis, 'visible': True})
            visible = inspect()
            assert axis(visible, point_axis)['visible'] and feature(visible, point_axis)['visible']
            assert visible['evaluation']['bodies'] == []
            model = out/'retained-datum-axes.alloy'
            call('document.save', {'path': str(model)})
            call('document.new', {'discard': True})
            call('document.open', {'path': str(model), 'discard': True})
            reopened = inspect()
            assert reopened['authored'] == visible['authored']
            measurements['reopenedPoints'] = check_axis(axis(reopened, point_axis), [20, 15, 7], [4, 3, 7])
            measurements['reopenedLine'] = check_axis(axis(reopened, line_axis), [-10, -10, 7], [0, 10, 7])
            call('view.orientation', {'name': 'isometric'}); call('view.fit')
            capture('edited-reopened-oblique')
            call('window.resize', {'width': 1400, 'height': 950})
            capture('resized-oblique')
            capture_environment = call('app.state')
            (out/'result.json').write_text(json.dumps({'passed': True, 'measurements': measurements,
                'axisIds': created_ids, 'sourceSketchId': sid, 'pointRefs': refs, 'lineRef': line_ref,
                'errors': errors, 'sourceRefusals': source_refusals, 'environment': initial_environment, 'captureEnvironment': capture_environment,
                'final': reopened, 'scope': 'Independent real-app API, analytic lines/incidence, exact authored refs, history, visibility/reopen and GPU captures. Physical OS input and exact public visual parity are unqualified.'}, indent=2)+'\n')
        finally:
            (out/'transcript.json').write_text(json.dumps(transcript, indent=2)+'\n')


if __name__ == '__main__':
    main()
