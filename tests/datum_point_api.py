#!/usr/bin/env python3
"""GEO-027 independent exact-source datum Point real-application verification.

Requires the sole private application lease. Physical input and exact public
visual parity are outside this API fixture's evidence.
"""
import argparse
import json
import math
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl


def vector(actual, expected):
    assert math.dist(actual, expected) < 1e-7, (actual, expected)


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
            def feature(state, fid):
                return next(f for f in state['authored']['features'] if f['id'] == fid)
            def point(state, fid):
                return next(p for p in state['evaluation']['constructionPoints'] if p['featureId'] == fid)
            def unchanged(before):
                after = inspect()
                for key in ('authored', 'evaluation', 'undo', 'revision'):
                    assert after[key] == before[key], key
            captures = {}
            def capture(name):
                captures[name] = call('capture.screenshot', {'path': str(out/(name+'.png'))})
                assert captures[name]['viewportPixelWidth'] > 0
                assert captures[name]['viewportPixelHeight'] > 0
                assert captures[name]['source'] == 'QWidget chrome + OCCT OpenGL framebuffer'
            measurements = {}
            def check(label, state, fid, xyz):
                actual = point(state, fid)
                vector(actual['positionMm'], xyz)
                measurements[label] = {'actual': actual, 'expectedPositionMm': xyz}
            def export(name):
                path = out/(name+'.stl')
                call('document.exportStl', {'path': str(path), 'bodyId': body['bodyId'], 'units': 'mm', 'format': 'binary',
                    'surfaceDeviationMm': .05, 'normalDeviationDeg': 8})
                return inspect_stl(path, True)
            call('window.resize', {'width': 1280, 'height': 900})
            environment = call('app.state')
            assert environment['viewport']['gpu']['hardwareAccelerated']
            assert 'construction.point' in {c['id'] for c in call('commands.list')['commands']}
            call('document.parameterAdd', {'name': 'BoxWidth', 'value': 20, 'units': 'mm'})
            width_id = inspect()['userParameters'][0]['id']
            call('solid.box', {'widthParameter': 'BoxWidth', 'depth': 30, 'height': 15, 'units': 'mm'})
            box = inspect()['authored']['features'][-1]
            body = inspect()['evaluation']['bodies'][0]
            edge = next(e for e in body['solidEdges'] if e['available'] and
                e['provenance'].get('startMm', [None]*3)[0] == 20 and
                e['provenance'].get('endMm', [None]*3)[0] == 20 and
                e['provenance']['startMm'][2] == e['provenance']['endMm'][2] == 15 and
                {e['provenance']['startMm'][1], e['provenance']['endMm'][1]} == {0, 30})
            endpoint_y = edge['provenance']['startMm'][1]
            vertex_ref = {'bodyId': body['bodyId'], 'edgeId': edge['edgeId'], 'endpoint': 0}
            call('sketch.create', {'plane': 'XY', 'name': 'Exact path sources'})
            call('sketch.line', {'x1': 35, 'y1': 0, 'x2': 47, 'y2': 16, 'units': 'mm'})
            call('sketch.addCircle', {'centerX': 60, 'centerY': 8, 'diameter': 8, 'units': 'mm'})
            sketch = call('app.state')['activeSession']['draftSketch']
            line = next(e for e in sketch['parameters']['entities'] if e['type'] == 'line')
            circle = next(e for e in sketch['parameters']['entities'] if e['type'] == 'circle')
            sid = sketch['id']
            line_ref = {k: line[k] for k in ('startPointId', 'endPointId')}
            line_ref.update(sketchId=sid, entityId=line['id'])
            call('sketch.finish')
            baseline = inspect()
            base_mesh = export('before-points')
            inputs = [
                {'source': {'vertex': vertex_ref}, 'name': 'Box vertex'},
                {'source': {'line': line_ref}, 'location': {'mode': 'proportional', 'ratio': .25}, 'name': 'Quarter point'},
                {'source': {'line': line_ref}, 'location': {'mode': 'physical', 'distanceMm': 5}, 'name': 'Five mm point'}]
            expected = [[20, endpoint_y, 15], [38, 4, 0], [38, 4, 0]]
            ids = []
            for label, values, xyz in zip(('vertex', 'proportional', 'physical'), inputs, expected):
                before = inspect()
                session = call('command.begin', {'command': 'construction.point', 'inputs': {}})
                assert not session['previewed']
                assert call('app.state')['ui']['activeDialog']['visible']
                call('command.update', {'session': session['id'], 'inputs': values})
                preview = call('command.preview', {'session': session['id']})
                assert preview['previewed']
                vector(preview['previewPoint']['positionMm'], xyz)
                unchanged(before)
                call('view.orientation', {'name': 'isometric'}); call('view.fit')
                capture(label+'-preview')
                call('capture.dialog', {'path': str(out/(label+'-panel.png'))})
                call('command.cancel', {'session': session['id']})
                unchanged(before)
                session = call('command.begin', {'command': 'construction.point', 'inputs': values})
                call('command.preview', {'session': session['id']})
                call('command.commit', {'session': session['id']})
                state = inspect(); fid = state['authored']['features'][-1]['id']; ids.append(fid)
                assert feature(state, fid)['type'] == 'constructionPoint'
                assert feature(state, fid)['parameters']['source'] == values['source']
                if 'location' in values:
                    assert feature(state, fid)['parameters']['location'] == values['location']
                check(label+'Committed', state, fid, xyz)
                assert state['evaluation']['bodies'] == baseline['evaluation']['bodies']
                call('history.undo'); assert inspect()['authored'] == before['authored']
                call('history.redo'); assert inspect()['authored'] == state['authored']
            vertex_id, ratio_id, physical_id = ids
            call('construction.point', {'featureId': vertex_id,
                'source': {'vertex': {**vertex_ref, 'endpoint': 1}}})
            check('otherVertexEndpoint', inspect(), vertex_id, [20, 30-endpoint_y, 15])
            call('construction.point', {**inputs[0], 'featureId': vertex_id})
            # Endpoint incidence and same-feature edit retain the exact ordered refs.
            for mode, key, value, xyz in [('proportional','ratio',0,[35,0,0]),
                    ('proportional','ratio',1,[47,16,0]), ('physical','distanceMm',0,[35,0,0]),
                    ('physical','distanceMm',20,[47,16,0])]:
                call('construction.point', {'featureId': ratio_id, 'source': {'line': line_ref},
                    'location': {'mode': mode, key: value}})
                state = inspect(); check(mode+str(value), state, ratio_id, xyz)
                assert len(state['authored']['features']) == len(baseline['authored']['features'])+3
            call('construction.point', {**inputs[1], 'featureId': ratio_id})
            before_edit = inspect()
            call('feature.editDialog', {'featureId': physical_id})
            session = call('app.state')['activeSession']
            assert session['inputs']['source'] == inputs[2]['source']
            assert session['inputs']['location'] == inputs[2]['location']
            call('command.update', {'session': session['id'], 'inputs': {'location': {'mode': 'physical', 'distanceMm': 10}}})
            call('command.preview', {'session': session['id']})
            call('command.cancel', {'session': session['id']}); unchanged(before_edit)
            call('feature.editDialog', {'featureId': physical_id})
            session = call('app.state')['activeSession']
            call('command.update', {'session': session['id'], 'inputs': {'name': 'Edited five mm point'}})
            call('command.preview', {'session': session['id']})
            call('capture.dialog', {'path': str(out/'physical-edit-panel.png')})
            call('command.commit', {'session': session['id']})
            assert feature(inspect(), physical_id)['name'] == 'Edited five mm point'
            # Source edits produce distinct Proportional and Physical positions.
            call('document.parameterUpdate', {'parameterId': width_id, 'value': 40})
            call('sketch.edit', {'sketchId': sid})
            call('sketch.movePoint', {'pointId': line['endPointId'], 'x': 59, 'y': 32, 'units': 'mm'})
            call('sketch.finish')
            changed = inspect()
            check('vertexResized', changed, vertex_id, [40, endpoint_y, 15])
            check('ratioSourceEdit', changed, ratio_id, [41, 8, 0])
            check('physicalSourceEdit', changed, physical_id, [38, 4, 0])
            assert math.isclose(point(changed, ratio_id)['sourceLengthMm'], 40, abs_tol=1e-7)
            assert point(changed, ratio_id)['ratio'] == .25
            assert point(changed, physical_id)['distanceMm'] == 5
            assert feature(changed, vertex_id)['parameters']['source'] == inputs[0]['source']
            for fid in (ratio_id, physical_id):
                assert feature(changed, fid)['parameters']['source'] == {'line': line_ref}
            call('history.undo'); check('sourceUndo', inspect(), ratio_id, [38,4,0])
            call('history.redo'); assert inspect()['authored'] == changed['authored']
            changed = inspect()
            source_refusals = []
            for operation, values in [('sketch.deleteSelection', {'entityIds': [line['id']]}),
                    ('sketch.movePoint', {'pointId': line['endPointId'], 'x': 35.6, 'y': .8, 'units': 'mm'})]:
                call('sketch.edit', {'sketchId': sid}); call(operation, values)
                call('sketch.finish', error='evaluation_failed')
                source_refusals.append(transcript[-1]['response']['error'])
                unchanged(changed)
                session = call('app.state')['activeSession']
                call('command.cancel', {'session': session['id']}); unchanged(changed)
            call('feature.delete', {'featureId': box['id']}, error='evaluation_failed'); unchanged(changed)
            errors = []
            stale = '00000000-0000-0000-0000-000000000001'
            bad_inputs = [
                {'source': {'vertex': {**vertex_ref, 'edgeId': stale}}},
                {'source': {'vertex': {**vertex_ref, 'bodyId': stale}}},
                {'source': {'line': {**line_ref, 'entityId': circle['id']}}, 'location': inputs[1]['location']},
                {'source': {'line': {**line_ref, 'entityId': stale}}, 'location': inputs[1]['location']},
                {'source': {'line': {**line_ref, 'startPointId': line_ref['endPointId'], 'endPointId': line_ref['startPointId']}}, 'location': inputs[1]['location']},
            ]
            for mode, key, value in [('proportional','ratio',-.1),('proportional','ratio',1.1),
                    ('physical','distanceMm',-1),('physical','distanceMm',41)]:
                bad_inputs.append({'source': {'line': line_ref}, 'location': {'mode': mode, key: value}})
            for bad in bad_inputs:
                value = bad.get('location', {})
                invalid_value = ('ratio' in value and not 0 <= value['ratio'] <= 1) or value.get('distanceMm', 0) < 0
                call('construction.point', {**bad, 'featureId': ratio_id},
                    error='invalid_point_source' if invalid_value else 'evaluation_failed')
                errors.append(transcript[-1]['response']['error']); unchanged(changed)
            for source in ({'chain': [line_ref, line_ref]}, {'vertex': {**vertex_ref, 'endpoint': 2}}):
                call('construction.point', {'source': source}, error='invalid_point_source')
                errors.append(transcript[-1]['response']['error']); unchanged(changed)
            # IEEE nonfinite values cannot be represented in valid JSON. The
            # shared client rejects them before any application mutation.
            for value in (float('nan'), float('inf')):
                try:
                    call('construction.point', {'source': {'line': line_ref},
                        'location': {'mode': 'proportional', 'ratio': value}})
                    raise AssertionError('Nonfinite input unexpectedly accepted')
                except ValueError as error:
                    errors.append({'layer': 'shared client JSON serialization', 'message': str(error)})
                unchanged(changed)
            call('feature.visibility', {'featureId': vertex_id, 'visible': False})
            assert not point(inspect(), vertex_id)['visible']
            assert not feature(inspect(), vertex_id)['visible']
            call('feature.visibility', {'featureId': vertex_id, 'visible': True})
            visible = inspect()
            model = out/'retained-datum-points.alloy'
            call('document.save', {'path': str(model)})
            call('document.new', {'discard': True})
            call('document.open', {'path': str(model), 'discard': True})
            reopened = inspect(); assert reopened['authored'] == visible['authored']
            for fid, xyz in zip(ids, ([40,endpoint_y,15],[41,8,0],[38,4,0])):
                check('reopened'+fid, reopened, fid, xyz)
            assert len(reopened['evaluation']['bodies']) == 1
            final_mesh = export('after-points-resized')
            assert final_mesh['bounds'] == [[0,0,0],[40,30,15]], final_mesh
            assert math.isclose(final_mesh['signedVolume'], 18000, abs_tol=1e-6)
            assert base_mesh['triangles'] == final_mesh['triangles'] == 12
            call('view.orientation', {'name': 'isometric'}); call('view.fit'); capture('reopened-oblique')
            call('window.resize', {'width': 1400, 'height': 950}); capture('resized-oblique')
            final_environment = call('app.state')
            # A current supported solid with a real edge still must not masquerade
            # as the specifically admitted semantic Box endpoint source class.
            call('document.new', {'discard': True})
            call('solid.cylinder', {'radius': 3, 'height': 7, 'units': 'mm'})
            cylinder_state = inspect(); cylinder = cylinder_state['evaluation']['bodies'][0]
            rim = next(e for e in cylinder['solidEdges'] if e['available'])
            call('construction.point', {'source': {'vertex': {'bodyId': cylinder['bodyId'],
                'edgeId': rim['edgeId'], 'endpoint': 0}}}, error='evaluation_failed')
            errors.append(transcript[-1]['response']['error']); unchanged(cylinder_state)
            call('document.open', {'path': str(model), 'discard': True})
            assert inspect()['authored'] == reopened['authored']
            (out/'result.json').write_text(json.dumps({'passed': True, 'measurements': measurements,
                'pointIds': ids, 'vertexRef': vertex_ref, 'lineRef': line_ref,
                'sourceRefusals': source_refusals, 'errors': errors, 'environment': environment,
                'captureEnvironment': final_environment, 'captures': captures, 'baselineMesh': base_mesh, 'finalMesh': final_mesh,
                'final': reopened, 'scope': 'Independent real-app API, analytic positions, authored refs, history, visibility/reopen and GPU captures. Physical OS input and exact public visual parity remain unqualified.'}, indent=2)+'\n')
        finally:
            (out/'transcript.json').write_text(json.dumps(transcript, indent=2)+'\n')


if __name__ == '__main__':
    main()
