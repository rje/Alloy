#!/usr/bin/env python3
"""Fix/UnFix an authored arc in a closed rounded profile and downstream solid."""
import argparse
import json
import math
from pathlib import Path

from app_harness import application


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    output = Path(args.output).resolve()
    with application(args.app, output) as (call, transcript):
        session = call('sketch.begin', {'plane': 'XY', 'name': 'Rounded plate'})

        def draft():
            return call('app.state')['activeSession']['draftSketch']

        def line(x1, y1, x2, y2):
            call('sketch.line', {'x1': x1, 'y1': y1, 'x2': x2, 'y2': y2, 'units': 'mm'})
            return draft()['parameters']['entities'][-1]

        bottom = line(0, 0, 60, 0)
        right = line(60, 0, 60, 34.9)
        call('sketch.addArc', {'mode': 'center', 'centerX': 55, 'centerY': 35,
                              'radius': 5, 'startAngleDeg': 0, 'sweepAngleDeg': 90, 'units': 'mm'})
        arc = draft()['parameters']['entities'][-1]
        top = line(55, 40, 0, 40)
        left = line(0, 40, 0, 0)
        assert right['endPointId'] != arc['startPointId']
        call('sketch.movePoint', {'pointId': right['endPointId'], 'x': 60, 'y': 35, 'units': 'mm'})
        assert arc['endPointId'] == top['startPointId']
        assert bottom['endPointId'] == right['startPointId']
        assert top['endPointId'] == left['startPointId']
        assert left['endPointId'] == bottom['startPointId']
        call('sketch.constraint', {'type': 'fixed', 'pointIds': [bottom['startPointId']]})
        for relation_type, entity in [('horizontal', bottom), ('vertical', right),
                                      ('horizontal', top), ('vertical', left)]:
            call('sketch.constraint', {'type': relation_type, 'entityIds': [entity['id']]})
        call('sketch.constraint', {'type': 'tangent', 'endpoint': 'end:start', 'entityIds': [right['id'], arc['id']]})
        second = call('sketch.constraint', {'type': 'tangent', 'entityIds': [arc['id'], top['id']]})
        relations = [c for c in second['draftSketch']['parameters']['constraints'] if c['type'] == 'arcTangent']
        assert len(relations) == 2
        distinct = next(c for c in relations if 'endpoints' in c)
        assert distinct['endpoints'][0]['pointId'] == arc['startPointId']
        assert distinct['endpoints'][1]['pointId'] == right['endPointId']
        assert distinct['endpoints'][0]['endpoint'] == 'start' and distinct['endpoints'][1]['endpoint'] == 'end'
        before_bad = draft()
        call('sketch.constraint', {'type': 'tangent', 'entityIds': [bottom['id'], arc['id']]}, 'sketch_solve_failed')
        assert draft() == before_bad
        radius_result = call('sketch.dimension', {'kind': 'radius', 'entityId': arc['id'], 'value': 5, 'units': 'mm'})
        radius_id = radius_result['draftSketch']['parameters']['dimensions'][-1]['id']
        width_result = call('sketch.dimension', {'kind': 'horizontal', 'entityId': bottom['id'], 'value': 60, 'units': 'mm'})
        width_id = width_result['draftSketch']['parameters']['dimensions'][-1]['id']
        call('sketch.dimension', {'kind': 'vertical', 'entityId': left['id'], 'value': 40, 'units': 'mm'})
        call('sketch.addPoint',{'x':55,'y':35,'units':'mm'})
        extra_point=draft()['parameters']['entities'][-1]['pointId']
        call('sketch.constraint',{'type':'coincident','pointIds':[extra_point,arc['centerPointId']]})
        coincidence=draft()['parameters']['constraints'][-1];assert coincidence['type']=='coincident'
        preview = call('command.preview', {'session': session['id']})
        profiles = preview['sketchGeometry']['profiles']
        assert len(profiles) == 1, profiles
        # One retained fixedArc locks center/radius/start/sweep while preserving
        # the existing endpoint tangency/coincidence authority and stable IDs.
        call('sketch.tool', {'tool':'select'});call('sketch.selectionSet')
        call('sketch.fixToggle', {'entityIds':[arc['id']]})
        fixed=draft();fixed_relation=fixed['parameters']['constraints'][-1]
        assert coincidence in fixed['parameters']['constraints']
        assert fixed_relation['type']=='fixedArc'
        assert fixed_relation['pointIds']==[arc['centerPointId'],arc['startPointId'],arc['endPointId']]
        def arc_in(d): return next(e for e in d['parameters']['entities'] if e['id']==arc['id'])
        assert all(c in fixed['parameters']['constraints'] for c in before_bad['parameters']['constraints'])
        assert fixed_relation['radiusMm']==arc_in(fixed)['radiusMm']
        assert fixed_relation['startAngleDeg']==arc_in(fixed)['startAngleDeg']
        assert fixed_relation['sweepAngleDeg']==arc_in(fixed)['sweepAngleDeg']
        def session(): return call('app.state')['activeSession']
        before_history=session()['draftHistory']
        for bad in [{'entityIds':['00000000-0000-0000-0000-000000000000']}, {'entityIds':[arc['id']],'pointIds':[arc['startPointId']]}]:
            call('sketch.fixToggle',bad,'sketch_solve_failed');assert draft()==fixed and session()['draftHistory']==before_history
        call('sketch.dimensionUpdate',{'dimensionId':radius_id,'value':7,'units':'mm'},'sketch_solve_failed');assert draft()==fixed and session()['draftHistory']==before_history
        call('sketch.dimension',{'kind':'arcSweep','entityId':arc['id'],'valueDeg':75},'sketch_solve_failed');assert draft()==fixed
        call('sketch.constraint',{'type':'fixed','entityIds':[arc['id']]});unfixed=draft()
        assert fixed_relation not in unfixed['parameters']['constraints'] and coincidence in unfixed['parameters']['constraints']
        call('history.undo');assert draft()==fixed
        call('history.redo');assert draft()==unfixed
        call('history.undo');assert draft()==fixed
        call('sketch.deleteSelection',{'constraintIds':[fixed_relation['id']]});assert draft()==unfixed
        call('history.undo');assert draft()==fixed
        state=call('app.state');assert next(e['role'] for e in state['activeSession']['solverDiagnostics']['entityRoles'] if e['entityId']==arc['id'])=='fixed'
        assert not {'fixed','fixedLine','fixedCircle','fixedArc'} & {e['kind'] for e in state['viewport']['gpu']['sketchConstraintGlyphs']}
        call('sketch.lookAt')
        call('capture.screenshot',{'path':str(output/'fixed-arc.png')})
        sketch_id = draft()['id']
        call('capture.screenshot', {'path': str(output / 'rounded-plate-sketch.png')})
        call('sketch.finish')
        extrude = call('command.begin', {'command': 'solid.extrude', 'inputs':
            {'sketchId': sketch_id, 'profileId': profiles[0]['profileId'], 'distance': 5, 'units': 'mm'}})
        call('command.preview', {'session': extrude['id']})
        call('command.commit', {'session': extrude['id']})
        initial = call('document.inspect')['evaluation']['bodies'][0]['volumeMm3']
        assert math.isclose(initial, 5 * (2400 - 25 + math.pi * 25 / 4), abs_tol=1e-3), initial
        call('sketch.edit', {'sketchId': sketch_id})
        # Fixed geometry remains authoritative in a retained source sketch with
        # a downstream solid, then native-equivalent UnFix permits the source edit.
        historical=draft();assert fixed_relation in historical['parameters']['constraints']
        call('sketch.dimensionUpdate', {'dimensionId':radius_id,'value':7,'units':'mm'},'sketch_solve_failed');assert draft()==historical
        call('sketch.fixToggle',{'entityIds':[arc['id']]})
        call('sketch.dimensionUpdate', {'dimensionId': radius_id, 'value': 7, 'units': 'mm'})
        call('sketch.dimensionUpdate', {'dimensionId': width_id, 'value': 70, 'units': 'mm'})
        call('sketch.fixToggle',{'entityIds':[arc['id']]})
        updated = draft();assert coincidence in updated['parameters']['constraints']
        assert {c['id'] for c in updated['parameters']['constraints'] if c['type'] == 'arcTangent'} == {c['id'] for c in relations}
        assert math.isclose(next(e['radiusMm'] for e in updated['parameters']['entities'] if e['id'] == arc['id']), 7, abs_tol=1e-5)
        assert next(c for c in updated['parameters']['constraints'] if c['id'] == distinct['id']) == distinct
        points = {p['id']: p for p in updated['parameters']['points']}
        a, b = [points[x] for x in [arc['startPointId'], right['endPointId']]]
        assert math.hypot(a['uMm']-b['uMm'], a['vMm']-b['vMm']) < 1e-6
        center = points[arc['centerPointId']]
        start = points[right['startPointId']]
        dx, dy = b['uMm']-start['uMm'], b['vMm']-start['vMm']
        rx, ry = a['uMm']-center['uMm'], a['vMm']-center['vMm']
        assert math.isclose(math.hypot(rx, ry), 7, abs_tol=1e-6)
        assert abs((dx*rx+dy*ry)/math.hypot(dx, dy)) < 1e-6
        before = call('app.state')
        call('sketch.dimensionUpdate', {'dimensionId': radius_id, 'value': 100, 'units': 'mm'}, 'sketch_solve_failed')
        after = call('app.state')
        assert after['activeSession']['selection'] == before['activeSession']['selection']
        assert after['document'] == before['document']
        assert draft() == updated
        call('capture.screenshot', {'path': str(output / 'rounded-plate-edited.png')})
        gpu = call('app.state')['viewport']['gpu']
        call('sketch.finish')
        final = call('document.inspect')['evaluation']['bodies'][0]['volumeMm3']
        assert math.isclose(final, 5 * (2800 - 49 + math.pi * 49 / 4), abs_tol=1e-3), final
        call('view.home')
        call('capture.screenshot', {'path': str(output / 'rounded-plate-solid.png')})
        call('history.undo')
        assert math.isclose(call('document.inspect')['evaluation']['bodies'][0]['volumeMm3'], initial, abs_tol=1e-3)
        call('history.redo')
        assert math.isclose(call('document.inspect')['evaluation']['bodies'][0]['volumeMm3'], final, abs_tol=1e-3)
        saved = output / 'fixed-arc-plate.alloy'
        call('document.save', {'path': str(saved)})
        authored = call('document.inspect')['authored']
        call('document.new', {'discard': False})
        call('document.open', {'path': str(saved)})
        assert call('document.inspect')['authored'] == authored
        (output / 'result.json').write_text(json.dumps({'status': 'passed', 'calls': len(transcript),
            'relationIds': [c['id'] for c in relations], 'radiusDimensionId': radius_id,
            'widthDimensionId': width_id, 'initialVolumeMm3': initial, 'finalVolumeMm3': final,
            'authored': authored, 'gpu': gpu, 'transcript': transcript}, indent=2) + '\n')
    print('PASS: Arc Fix/UnFix,  distinct endpoint Tangent, edited closed plate/solid, atomic refusal, history and exact reopen')


if __name__ == '__main__':
    main()
