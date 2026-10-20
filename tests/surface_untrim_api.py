#!/usr/bin/env python3
"""SRF-032 independent crossing-sheet Trim → External Edges Untrim API fixture."""
import argparse
import copy
import json
import math
from pathlib import Path
import uuid
from app_harness import application


def near(actual, expected):
    assert math.isclose(actual, expected, rel_tol=1e-9, abs_tol=2e-5), (actual, expected)


def identity(body):
    return (body.get('bodyId', body['supportFaces'][0]['bodyId']), [(f['producerFeatureId'], f['faceId']) for f in body['supportFaces']],
            sorted(e['edgeId'] for e in body['solidEdges']))


def restored(body, source):
    assert body['valid'] and body['type'] == 'surface' and not body['closed'], body
    assert body['topology']['faces'] == 1 and body['topology']['edges'] == 4 and body['topology']['solids'] == 0
    near(body['areaMm2'], source['areaMm2'])
    assert identity(body) == identity(source), (identity(body), identity(source))
    for key in ('min', 'max'):
        for a, b in zip(body['boundsMm'][key], source['boundsMm'][key]):
            near(a, b)
    boundaries = lambda b: {e['edgeId']: e for e in b['solidEdges']}
    for key, edge in boundaries(body).items():
        prior = boundaries(source)[key]
        for endpoint in ('startMm', 'endMm'):
            assert math.dist(edge[endpoint], prior[endpoint]) < 2e-5, (edge, prior)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--source', required=True, help='Untrimmed 40 × 20 crossing-sheet source model')
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    out = Path(args.output).resolve()
    out.mkdir(parents=True, exist_ok=True)
    with application(args.app, out, command_timeout=90) as (call, transcript):
        try:
            inspect = lambda: call('document.inspect')
            def body(state, body_id):
                return next(b for b in state['evaluation']['bodies'] if b['bodyId'] == body_id)
            def unchanged(before):
                after = inspect()
                for key in ('authored', 'revision', 'undo', 'evaluation'):
                    assert after[key] == before[key], key
            call('window.resize', {'width': 1280, 'height': 800})
            environment = call('app.state')
            assert environment['viewport']['gpu']['hardwareAccelerated'], environment
            assert 'surface.untrim' in {c['id'] for c in call('commands.list')['commands']}
            call('document.open', {'path': str(Path(args.source).resolve()), 'discard': True})
            original = inspect()
            target = next(b for b in original['evaluation']['bodies'] if b['name'] == 'Target sheet')
            tool = next(b for b in original['evaluation']['bodies'] if b['name'] == 'Tool sheet')
            source = next(f for f in original['authored']['features'] if f['name'] == 'Target sheet source')
            tool_source = next(f for f in original['authored']['features'] if f['name'] == 'Tool sheet source')
            target_id, tool_id = target['bodyId'], tool['bodyId']
            near(target['areaMm2'], 800)
            call('document.save', {'path': str(out/'original-800.alloy')})
            trim = call('surface.trimDialog', {'targetBodyId': target_id, 'toolBodyId': tool_id})
            assert len(trim['regions']) == 2
            call('surface.trimRegion', {'regionId': trim['regions'][0]['regionId']})
            call('command.commit', {'session': trim['id']})
            trimmed = inspect()
            trim_feature = trimmed['authored']['features'][-1]
            near(body(trimmed, target_id)['areaMm2'], 400)
            call('document.save', {'path': str(out/'trimmed-400.alloy')})
            trimmed = inspect()
            face, = body(trimmed, target_id)['supportFaces']
            source_ref = {k: face[k] for k in ('bodyId', 'producerFeatureId', 'faceId')}
            assert source_ref['producerFeatureId'] == trim_feature['id']
            inputs = {'sourceFace': source_ref, 'trimFeatureId': trim_feature['id'], 'untrimType': 'externalEdges'}
            call('view.home')
            empty = call('surface.untrimDialog')
            call('command.preview', {'session': empty['id']}, error='selection_required')
            pixel = call('view.projectBody', {'bodyId': target_id})
            call('selection.facePick', {'x': pixel['x'], 'y': pixel['y']})
            selected = call('app.state')['activeSession']['inputs']
            assert selected['sourceFace'] == source_ref and selected['trimFeatureId'] == trim_feature['id'], selected
            preview = call('command.preview', {'session': empty['id']})
            restored(preview['geometry'], target)
            call('capture.dialog', {'path': str(out/'untrim-controls.png')})
            call('capture.screenshot', {'path': str(out/'untrim-preview.png')})
            unchanged(trimmed)
            call('command.cancel', {'session': empty['id']})
            unchanged(trimmed)
            call('surface.untrim', inputs)
            committed = inspect()
            feature = committed['authored']['features'][-1]
            feature_id = feature['id']
            assert feature['type'] == 'surfaceUntrim'
            assert committed['authored']['features'][:-1] == trimmed['authored']['features']
            assert all(feature['parameters'][k] == v for k, v in inputs.items())
            restored(body(committed, target_id), target)
            assert body(committed, tool_id) == tool
            call('document.save', {'path': str(out/'restored-800.alloy')})
            call('capture.screenshot', {'path': str(out/'untrim-committed.png')})
            call('history.undo')
            assert inspect()['authored'] == trimmed['authored']
            near(body(inspect(), target_id)['areaMm2'], 400)
            call('history.redo')
            assert inspect()['authored'] == committed['authored']
            edit = call('feature.editDialog', {'featureId': feature_id})
            restored(call('command.preview', {'session': edit['id']})['geometry'], target)
            edit_baseline = inspect()
            stale_ref = {**source_ref, 'faceId': str(uuid.uuid4())}
            call('command.update', {'session': edit['id'], 'inputs': {'sourceFace': stale_ref}}, error='surface_source_unavailable')
            assert not call('app.state')['activeSession']['previewed']
            call('command.commit', {'session': edit['id']}, error='surface_source_unavailable')
            unchanged(edit_baseline)
            call('command.update', {'session': edit['id'], 'inputs': {'sourceFace': source_ref}})
            restored(call('command.preview', {'session': edit['id']})['geometry'], target)
            call('command.cancel', {'session': edit['id']})
            assert inspect()['authored'] == committed['authored']
            edit = call('feature.editDialog', {'featureId': feature_id})
            call('command.update', {'session': edit['id'], 'inputs': {'name': 'Restored finite sheet'}})
            call('command.commit', {'session': edit['id']})
            baseline = inspect()
            assert baseline['authored']['features'][-1]['id'] == feature_id
            assert baseline['authored']['features'][-1]['name'] == 'Restored finite sheet'
            restored(body(baseline, target_id), target)
            failures = []
            for changed, code in (({'untrimType': 'internalEdges'}, 'invalid_arguments'),
                                  ({'trimFeatureId': str(uuid.uuid4())}, 'evaluation_failed'),
                                  ({'sourceFace': {**source_ref, 'faceId': str(uuid.uuid4())}}, 'evaluation_failed')):
                call('surface.untrim', {'featureId': feature_id, **changed}, error=code)
                failure = transcript[-1]['response']['error']
                if code == 'evaluation_failed':
                    expected = 'untrim_history_unavailable' if 'trimFeatureId' in changed else 'surface_source_unavailable'
                    assert failure['diagnostics']['code'] == expected, failure
                failures.append(failure)
                unchanged(baseline)
            # A valid upstream target edit must recompute the retained original, not a frozen snapshot.
            parameters = copy.deepcopy(source['parameters'])
            for point in parameters['points']:
                point['uMm'] *= 1.25
            call('feature.edit', {'featureId': source['id'], 'parameters': parameters})
            changed = inspect()
            near(body(changed, target_id)['areaMm2'], 1000)
            assert identity(body(changed, target_id)) == identity(target)
            call('timeline.rollbackBefore', {'featureId': trim_feature['id']})
            live_source = body(inspect(), target_id)
            near(live_source['areaMm2'], 1000)
            call('timeline.returnToLatest')
            restored(body(inspect(), target_id), live_source)
            call('history.undo')
            assert inspect()['authored'] == baseline['authored']
            call('history.redo')
            assert inspect()['authored'] == changed['authored']
            # Moving the tool clear of the target invalidates the retained Trim and refuses atomically.
            failed = copy.deepcopy(tool_source['parameters'])
            for point in failed['points']:
                point['vMm'] = 30
            before_failure = inspect()
            call('feature.edit', {'featureId': tool_source['id'], 'parameters': failed}, error='evaluation_failed')
            unchanged(before_failure)
            model = out/'restored-source-edit-1000.alloy'
            call('document.save', {'path': str(model)})
            final = inspect()
            call('document.new', {'discard': True})
            call('document.open', {'path': str(model), 'discard': True})
            reopened = inspect()
            assert reopened['authored'] == final['authored']
            restored(body(reopened, target_id), live_source)
            call('view.home')
            call('capture.screenshot', {'path': str(out/'untrim-source-edit-reopened.png')})
            report = {'passed': True, 'featureId': feature_id, 'trimFeatureId': trim_feature['id'],
                      'sourceFace': source_ref, 'environment': environment, 'captureEnvironment': call('app.state'),
                      'original': original, 'trimmed': trimmed, 'committed': committed, 'reopened': reopened,
                      'failures': failures, 'scope': 'Shared API, programmatic face hit, finite retained original boundary, exact IDs, source recompute, history/reopen and GPU capture. Independent saved BRep probe separate; physical native input and matched public pixels unverified.'}
            (out/'result.json').write_text(json.dumps(report, indent=2)+'\n')
        finally:
            (out/'transcript.json').write_text(json.dumps(transcript, indent=2)+'\n')


if __name__ == '__main__':
    main()
