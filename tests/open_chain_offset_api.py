#!/usr/bin/env python3
"""Real-app linked five-edge Offset, separate closure, binding and history."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application


def close(a, b, tol=1e-5):
    return abs(a-b) < tol


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--app', required=True)
    parser.add_argument('--source', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    out = Path(args.output).resolve()
    out.mkdir(parents=True, exist_ok=True)
    report = {'scope': 'Actual cut-front projected five-edge open Offset and independent Line closure', 'status': 'incomplete'}
    transcript = []
    try:
        with application(args.app, out) as (call, transcript):
            inspect = lambda: call('document.inspect')
            state = lambda: call('app.state')['activeSession']
            draft = lambda: state()['draftSketch']
            call('document.open', {'path': str(Path(args.source).resolve()), 'discard': True})
            cut = inspect()
            body = cut['evaluation']['bodies'][0]
            front = next(f for f in body['supportFaces'] if f['role'] == 'box.y0')
            assert len(front['edges']) == 8
            support = {key: front[key] for key in ('producerFeatureId', 'bodyId', 'faceId')}
            lid = call('component.create', {'name': 'Lid'})['definitionId']
            call('component.activate', {'definitionId': lid})
            call('sketch.begin', {'support': support, 'name': 'Lid clearance'})
            call('sketch.lookAt')
            call('sketch.projectBegin')
            call('sketch.projectFace', support)
            call('sketch.projectCommit')
            original = draft()
            projected = state()['projectionContext']['lines']
            def chosen(predicate):
                matches = [line for line in projected if predicate(line)]
                assert len(matches) == 1, matches
                return matches[0]
            left_shoulder = chosen(lambda l: close(l['v1Mm'],40) and close(l['v2Mm'],40)
                                   and max(l['u1Mm'],l['u2Mm']) <= 4.001)
            left_slant = chosen(lambda l: close(max(l['v1Mm'],l['v2Mm']),40)
                                and 35 < min(l['v1Mm'],l['v2Mm']) < 40
                                and max(l['u1Mm'],l['u2Mm']) < 25)
            bottom = chosen(lambda l: close(l['v1Mm'],l['v2Mm'])
                            and 35 < l['v1Mm'] < 40 and abs(l['u2Mm']-l['u1Mm']) > 40)
            right_slant = chosen(lambda l: close(max(l['v1Mm'],l['v2Mm']),40)
                                 and 35 < min(l['v1Mm'],l['v2Mm']) < 40
                                 and min(l['u1Mm'],l['u2Mm']) > 25)
            right_shoulder = chosen(lambda l: close(l['v1Mm'],40) and close(l['v2Mm'],40)
                                    and min(l['u1Mm'],l['u2Mm']) >= 45.999)
            source_ids = [line['id'] for line in (left_shoulder,left_slant,bottom,right_slant,right_shoulder)]
            call('sketch.tool', {'tool': 'offset'})
            preview = call('sketch.offsetPreview', {'sourceEntityIds':source_ids,'distance':-0.25,'units':'mm'})
            assert preview['pendingOpenChainOffset']['valid'] and draft() == original
            call('view.home')
            call('capture.screenshot', {'path':str(out/'offset-preview.png')})
            call('sketch.offsetCancel')
            assert draft() == original
            call('sketch.tool', {'tool': 'offset'})
            call('sketch.offsetPreview', {'sourceEntityIds':source_ids,'distance':-0.25,'units':'mm'})
            call('sketch.offsetAccept')
            group = draft()['parameters']['openChainOffsets'][0]
            assert group['sourceEntityIds'] == source_ids and 'topEntityId' not in group
            assert len(group['generatedEntityIds']) == 5
            points = {p['id']:p for p in draft()['parameters']['points']}
            left, right = (points[i] for i in group['topIntersectionPointIds'])
            assert close(left['vMm'],40) and close(right['vMm'],40)
            assert left['uMm'] < right['uMm']
            call('capture.screenshot', {'path':str(out/'offset-accepted.png')})
            call('sketch.line', {'x1':left['uMm'],'y1':left['vMm'],
                                 'x2':right['uMm'],'y2':right['vMm'],'units':'mm',
                                 'startSnapTarget':{'kind':'point','pointId':group['topIntersectionPointIds'][0]},
                                 'endSnapTarget':{'kind':'point','pointId':group['topIntersectionPointIds'][1]}})
            closure = draft()['parameters']['entities'][-1]
            assert closure['type'] == 'line' and closure['id'] not in group['generatedEntityIds']
            sketch_id = draft()['id']
            call('sketch.finish')
            closed = inspect()
            profiles = closed['evaluation']['sketches'][-1]['profiles']
            lid_profiles = [p for p in profiles if closure['id'] in p['sourceEntityIds']
                            and bottom['id'] not in p['sourceEntityIds']]
            assert lid_profiles, profiles
            profile = lid_profiles[0]
            call('capture.screenshot', {'path':str(out/'closure-profile.png')})
            parameter = call('document.parameterAdd', {'name':'Clearance','value':0.25,'units':'mm'})['userParameters'][-1]
            call('sketch.edit', {'sketchId':sketch_id})
            call('sketch.openChainOffsetUpdate', {'offsetId':group['id'],'parameterName':'Clearance'})
            assert draft()['parameters']['openChainOffsets'][0]['distanceParameterId'] == parameter['id']
            call('sketch.finish')
            bound = inspect()
            call('document.parameterUpdate', {'parameterId':parameter['id'],'value':0.3})
            changed = inspect()
            changed_group = next(f for f in changed['authored']['features'] if f['id']==sketch_id)['parameters']['openChainOffsets'][0]
            assert changed_group['distanceParameterId'] == parameter['id']
            assert close(changed_group['signedDistanceMm'],-0.3)
            assert changed['evaluation']['sketches'][-1]['profiles'] != bound['evaluation']['sketches'][-1]['profiles']
            call('document.parameterUpdate', {'parameterId':parameter['id'],'name':'Gap'})
            renamed = inspect()
            assert next(f for f in renamed['authored']['features'] if f['id']==sketch_id)['parameters']['openChainOffsets'][0]['distanceParameterId'] == parameter['id']
            call('history.undo')
            assert inspect()['authored'] == changed['authored']
            call('history.redo')
            assert inspect()['authored'] == renamed['authored']
            saved = out/'linked-open-offset.alloy'
            call('document.save', {'path':str(saved)})
            call('document.open', {'path':str(saved), 'discard':True})
            assert inspect()['authored'] == renamed['authored']
            report.update(status='api-pass',sourceIds=source_ids,offsetId=group['id'],
                          closureId=closure['id'],profileId=profile['profileId'],
                          parameterId=parameter['id'],authored=renamed['authored'])
    finally:
        report['callCount'] = len(transcript)
        (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
        (out/'transcript.json').write_text(json.dumps(transcript,indent=2)+'\n')
    print('PASS: actual projected cut chain Offset, closure, Clearance binding and history')


if __name__ == '__main__':
    main()
