#!/usr/bin/env python3
"""Two retained planar-inset owners on one split-cap sketch through the real app."""
import argparse, hashlib, json
from pathlib import Path
from app_harness import application

p = argparse.ArgumentParser()
p.add_argument('--app', required=True)
p.add_argument('--output', required=True)
p.add_argument('--source', required=True)
a = p.parse_args()
out = Path(a.output).resolve()
with application(a.app, out, command_timeout=180) as (call, ledger):
    inspect = lambda: call('document.inspect')
    session = lambda: call('app.state')['activeSession']
    draft = lambda: session()['draftSketch']
    def profiles(state, sketch):
        return next(s for s in state['evaluation']['sketches'] if s['sketchId'] == sketch)['profiles']
    source_path = Path(a.source).resolve()
    call('document.open', {'path': str(source_path)})
    call('window.resize', {'width': 1500, 'height': 950})
    initial = inspect()
    body = next(b for b in initial['evaluation']['bodies'] if any(f['role'] == 'split.negative.cap' for f in b['supportFaces']))
    cap = next(f for f in body['supportFaces'] if f['role'] == 'split.negative.cap')
    assert len(cap['edges']) == 18
    support = {k: cap[k] for k in ('producerFeatureId', 'bodyId', 'faceId')}
    owner = next(b['ownerId'] for b in call('component.inspect')['bodies'] if b['bodyId'] == body['bodyId'])
    call('component.activate', {'definitionId': owner})
    for feature in initial['authored']['features']:
        if feature['type'] in ('stepAssemblyImport', 'splitBody'):
            call('feature.visibility', {'featureId': feature['id'], 'visible': False})
    for analysis in initial['authored'].get('analyses', []):
        if analysis.get('visible'):
            call('analysis.visibility', {'analysisId': analysis['id'], 'visible': False})
    call('sketch.begin', {'support': support, 'name': 'Independent planar inset owners', 'units': 'mm'})
    # The larger interior goes first; the smaller later interior must remain selectable.
    for distance in (-0.75, -1.25):
        before = draft()
        call('sketch.tool', {'tool': 'offset'})
        call('sketch.loopOffsetPreview', {'kind': 'planarInsetV1', 'distance': distance, 'chain': True, 'units': 'mm'})
        assert draft() == before
        call('sketch.loopOffsetAccept')
    assert len(draft()['parameters']['loopOffsets']) == 2
    original = draft()
    call('sketch.tool', {'tool': 'offset'})
    call('sketch.loopOffsetPreview', {'kind': 'planarInsetV1', 'distance': -1, 'units': 'mm'})
    assert len(session()['pendingLoopOffset']['previewDraft']['parameters']['loopOffsets']) == 3
    call('sketch.tool', {'tool': 'select'})
    assert draft() == original
    # Exact update of the second owner must preserve the first authored row.
    rows = original['parameters']['loopOffsets']
    call('sketch.loopOffsetUpdate', {'offsetId': rows[1]['id'], 'distance': -1.1, 'units': 'mm'})
    assert draft()['parameters']['loopOffsets'][0] == rows[0]
    assert draft()['parameters']['loopOffsets'][1]['id'] == rows[1]['id']
    call('history.undo')
    assert draft() == original
    call('sketch.finish')
    base = inspect()
    target = base['authored']['features'][-1]
    old_profiles = profiles(base, target['id'])
    assert all(row['kind'] == 'planarInsetV1' for row in target['parameters']['loopOffsets'])
    # Both rows follow the same retained face edit without changing their identity.
    src = next(f for f in base['authored']['features'] if f['name'] == 'Centered case stock')
    dimension = next(d for d in src['parameters']['dimensions'] if d['kind'] == 'horizontal' and d['valueMm'] == 64)
    call('sketch.edit', {'sketchId': src['id']})
    call('sketch.dimensionUpdate', {'dimensionId': dimension['id'], 'value': 65, 'units': 'mm'})
    call('sketch.finish')
    base = inspect()
    both = profiles(base, target['id'])
    assert [r['profileId'] for r in both] == [r['profileId'] for r in old_profiles]
    assert all(x['areaMm2'] != y['areaMm2'] for x, y in zip(both, old_profiles))
    assert next(f for f in base['authored']['features'] if f['id'] == target['id']) == target
    baseline = out / 'multi-native-before.alloy'
    call('document.save', {'path': str(baseline)})
    for index in (0, 1):
        case = out / ('owner-' + str(index))
        case.mkdir(exist_ok=True)
        call('document.open', {'path': str(baseline), 'discard': True})
        base = inspect()
        call('sketch.edit', {'sketchId': target['id']})
        call('sketch.lookAt'); call('view.fit'); call('sketch.tool', {'tool': 'select'})
        original = draft()
        selected = both[index]['profileId']
        for args, code in [({'profileIds': [selected, selected]}, 'invalid_profile_target'),
                           ({'profileIds': [both[0]['profileId'], both[1]['profileId']]}, 'invalid_profile_target'),
                           ({'profileIds': ['00000000-0000-0000-0000-000000000001']}, 'stale_profile'),
                           ({'profileIds': [selected], 'entityIds': ['stale']}, 'ambiguous_delete_target')]:
            before = session(); call('sketch.deleteSelection', args, error=code)
            assert session() == before and inspect()['authored'] == base['authored'] and inspect()['evaluation'] == base['evaluation'], (code, session() == before, inspect()['authored'] == base['authored'], inspect()['evaluation'] == base['evaluation'])
        call('capture.screenshot', {'path': str(case / 'before.png')})
        call('sketch.selectionSet', {'profileIds': [selected]})
        call('capture.screenshot', {'path': str(case / 'selected.png')})
        before = session(); result = call('sketch.deleteSelection')
        changed = draft()
        expected = json.loads(json.dumps(original))
        expected['parameters']['loopOffsets'].pop(index)
        assert changed == expected
        assert session()['draftHistory']['index'] == before['draftHistory']['index'] + 1
        call('capture.screenshot', {'path': str(case / 'deleted.png')})
        call('history.undo'); assert draft() == original
        call('capture.screenshot', {'path': str(case / 'undone.png')})
        call('history.redo'); assert draft() == changed
        call('sketch.finish')
        final = inspect()
        assert final['authored']['features'][:-1] == base['authored']['features'][:-1]
        assert final['evaluation']['bodies'] == base['evaluation']['bodies']
        assert profiles(final, target['id']) == [both[1-index]]
        path = case / 'deleted.alloy'
        call('document.save', {'path': str(path)})
        call('document.open', {'path': str(path), 'discard': True})
        assert inspect()['authored'] == final['authored'] and profiles(inspect(), target['id']) == [both[1-index]]
        call('capture.screenshot', {'path': str(case / 'reopened.png')})
    # Consumed profile refuses; removing its sibling is allowed and retains that solid.
    call('document.open', {'path': str(baseline), 'discard': True})
    call('component.activate', {'definitionId': owner})
    call('solid.extrude', {'sketchId': target['id'], 'profileId': both[0]['profileId'], 'distance': 1, 'units': 'mm'})
    consumed = inspect()
    consumer = consumed['authored']['features'][-1]
    call('sketch.edit', {'sketchId': target['id']})
    call('sketch.tool', {'tool': 'select'})
    call('sketch.selectionSet', {'profileIds': [both[0]['profileId']]})
    before = session(); call('sketch.deleteSelection', error='downstream_profile_in_use')
    assert session() == before and inspect()['authored'] == consumed['authored'] and inspect()['evaluation'] == consumed['evaluation']
    refusal = next(x['response'] for x in reversed(ledger) if x['command'] == 'sketch.deleteSelection')
    assert refusal['error']['diagnostics']['featureId'] == consumer['id']
    assert refusal['error']['diagnostics']['profileId'] == both[0]['profileId']
    call('sketch.deleteSelection', {'profileIds': [both[1]['profileId']]})
    call('sketch.finish')
    assert inspect()['evaluation']['bodies'] == consumed['evaluation']['bodies']
    call('document.save', {'path': str(out / 'safe-sibling-deleted.alloy')})
    gpu = call('app.state')['viewport']['gpu']
    assert gpu['hardwareAccelerated']
    gpu = {k: gpu[k] for k in ('vendor', 'renderer', 'version', 'hardwareAccelerated', 'details') if k in gpu}
    (out / 'receipt.json').write_text(json.dumps({'status': 'pass', 'appSha256': hashlib.sha256(Path(a.app).read_bytes()).hexdigest(), 'variant': 'planarInsetV1', 'source': str(source_path), 'sourceSha256': hashlib.sha256(source_path.read_bytes()).hexdigest(), 'profilesBeforeSourceEdit': old_profiles, 'target': target['id'], 'profiles': both, 'support': support, 'gpu': gpu, 'downstreamRefusal': refusal}, indent=2) + '\n')
    (out / 'transcript.json').write_text(json.dumps(ledger, indent=2) + '\n')
print('PASS: two same-sketch owners, either Delete, exact sibling/source, edit, history/reopen and consumed refusal')
