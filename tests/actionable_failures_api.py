#!/usr/bin/env python3
"""Bounded cumulative REL-015 real-app failure/recovery fixtures.

Run against a staged package. API, saved intent, geometry and GPU evidence are
reported independently; this runner does not qualify physical pointer/key input.
"""
import argparse
import hashlib
import json
from pathlib import Path
from app_harness import application

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--app', required=True)
p.add_argument('--output', required=True, type=Path)
a = p.parse_args()
out = a.output.resolve()
out.mkdir(parents=True, exist_ok=True)
ref = lambda f: {k: f[k] for k in ('producerFeatureId', 'bodyId', 'faceId')}
sha = lambda path: hashlib.sha256(Path(path).read_bytes()).hexdigest()
report = {'scope': 'Bounded API/package/GPU qualification; physical input separate',
          'binarySha256': sha(a.app), 'gates': {}}

with application(a.app, out, command_timeout=120) as (call, transcript):
    inspect = lambda: call('document.inspect')
    def stable(state):
        return {k: state[k] for k in ('authored', 'evaluation', 'revision', 'undo')}
    def checkpoint(name):
        path = out / (name + '.alloy')
        call('document.save', {'path': str(path)})
        return path, sha(path), stable(inspect())
    def refused(name, command, args, code, checkpoint):
        path, digest, before = checkpoint
        call(command, args, error=code)
        error = transcript[-1]['response']['error']
        expected = {
            'invalid-downstream-dimension': 'reduce the thickness',
            'missing-face-support': 'support producer does not exist',
            'missing-projected-support': 'Projected source edge',
            'stale-command-context': 'cancel and restart',
        }[name]
        assert expected in error.get('message', ''), error
        assert stable(inspect()) == before, name
        assert sha(path) == digest, 'Original file changed on failure'
        report['gates'][name] = {'error': error, 'fixture': str(path),
            'fixtureSha256': digest, 'atomicAuthoredGeometryRevisionUndo': True,
            'originalFilePreserved': True}
    def reopen(checkpoint):
        path, digest, before = checkpoint
        call('document.open', {'path': str(path), 'discard': True})
        assert inspect()['authored'] == before['authored']
        after = inspect()['evaluation']
        (out / (path.stem + '-reopen-comparison.json')).write_text(json.dumps({'before': before['evaluation'], 'after': after}, indent=2))
        assert {k:v for k,v in after.items() if k not in ('documentRevision', 'recompute')} == {
            k:v for k,v in before['evaluation'].items() if k not in ('documentRevision', 'recompute')}
        assert sha(path) == digest
    def history_cycle(before, changed):
        call('history.undo'); assert inspect()['authored'] == before
        call('history.redo'); assert inspect()['authored'] == changed

    # Existing Box/Shell direct named-length fixture, with impossible downstream Shell.
    call('document.parameterAdd', {'name': 'Wall', 'value': 4, 'units': 'mm'})
    wall = inspect()['userParameters'][0]['id']
    call('solid.box', {'width': 40, 'depth': 30, 'height': 20, 'units': 'mm'})
    box = inspect()['authored']['features'][-1]
    body = inspect()['evaluation']['bodies'][0]
    top = next(f for f in body['supportFaces'] if f['role'] == 'box.z1')
    call('solid.shell', {'targetBodyId': body['bodyId'], 'removedFace': ref(top),
                         'thicknessParameter': 'Wall'})
    shell = inspect()['authored']['features'][-1]
    seed = checkpoint('named-shell')
    refused('invalid-downstream-dimension', 'document.parameterUpdate',
            {'parameterId': wall, 'value': 30}, 'parameter_evaluation_failed', seed)
    call('document.parameterUpdate', {'parameterId': wall, 'value': 3})
    repaired = inspect()['authored']
    history_cycle(seed[2]['authored'], repaired)
    report['gates']['invalid-downstream-dimension']['repair'] = 'Update Wall to 3 mm; exact Undo/Redo'
    reopen(seed)

    # Existing linked Project fixture: a front-supported sketch plus a Shell-inner
    # normal edge projected as a point. Removing Shell loses that exact source.
    body = inspect()['evaluation']['bodies'][0]
    front = next(f for f in body['supportFaces'] if f['role'] == 'box.y0')
    def along_y(edge):
        q = edge.get('provenance', edge)
        x, y, z = q['startMm']; xx, yy, zz = q['endMm']
        return abs(x-xx)<1e-6 and abs(z-zz)<1e-6 and abs(y-yy)>1
    inner = next(e for f in body['supportFaces'] if f['role']=='shell-inner'
                 for e in f['edges'] if along_y(e))
    call('sketch.begin', {'support': ref(front), 'name': 'Retained front and inner edge'})
    call('sketch.projectBegin')
    call('sketch.projectFace', ref(front))
    call('sketch.projectSelect', {'bodyId': body['bodyId'], 'edgeId': inner['edgeId']})
    call('sketch.projectCommit'); call('sketch.finish')
    linked = checkpoint('linked-front')
    refused('missing-projected-support', 'feature.delete', {'featureId': shell['id']},
            'evaluation_failed', linked)
    call('document.parameterUpdate', {'parameterId': wall, 'value': 3})
    changed = inspect()['authored']
    history_cycle(linked[2]['authored'], changed)
    for name in ('missing-projected-support',):
        report['gates'][name]['repair'] = 'Preserve exact source; edit valid Shell thickness to 3 mm; Undo/Redo and reopen'
    reopen(linked)

    # A real feature dialog must refuse its prepared draft after the document changes.
    edit = call('feature.editDialog', {'featureId': box['id']})
    call('command.update', {'session': edit['id'], 'inputs': {'width': 45}})
    call('document.units', {'units': 'in'})
    context = (linked[0], linked[1], stable(inspect()))
    refused('stale-command-context', 'command.commit', {'session': edit['id']},
            'stale_session', context)
    call('capture.screenshot', {'path': str(out/'stale-command.png')})
    call('command.cancel', {'session': edit['id']})
    fresh = call('feature.editDialog', {'featureId': box['id']})
    call('command.cancel', {'session': fresh['id']})
    call('history.undo'); assert inspect()['authored'] == linked[2]['authored']
    report['gates']['stale-command-context']['repair'] = 'Cancel and reopen feature edit; Undo restores units'

    # The exact inner floor disappears if the upstream Shell is removed.
    reopen(seed)
    body = inspect()['evaluation']['bodies'][0]
    floor = next(f for f in body['supportFaces'] if f['role'] == 'shell-inner'
                 and f['frame']['normal'][2] > .99)
    call('sketch.begin', {'support': ref(floor), 'name': 'Exact inner floor'})
    call('sketch.rectangle', {'x1': 8, 'y1': 8, 'x2': 12, 'y2': 12, 'units': 'mm'})
    call('sketch.finish')
    supported = checkpoint('inner-floor-supported')
    refused('missing-face-support', 'feature.delete', {'featureId': shell['id']},
            'evaluation_failed', supported)
    call('document.parameterUpdate', {'parameterId': wall, 'value': 3})
    history_cycle(supported[2]['authored'], inspect()['authored'])
    reopen(supported)
    report['gates']['missing-face-support']['repair'] = 'Preserve Shell producer; valid thickness edit, Undo/Redo and reopen'

    # Existing Split ambiguity fixture: two exact descendants share the retained
    # side-face identity. Mark the analysis unavailable instead of choosing a half.
    call('document.new', {'discard': True})
    call('solid.box', {'width': 40, 'depth': 30, 'height': 20, 'units': 'mm'})
    body = inspect()['evaluation']['bodies'][0]
    face = lambda role: ref(next(f for f in body['supportFaces'] if f['role']==role))
    call('inspect.section', {'plane': 'face', 'support': face('box.x1'), 'name': 'Side'})
    plane = call('construction.midplane', {'faces': [face('box.z0'), face('box.z1')]})['planeId']
    unique = checkpoint('unique-side-section')
    call('solid.splitBody', {'targetBodyId': body['bodyId'], 'planeId': plane})
    split = checkpoint('ambiguous-side-section')
    row = call('analysis.inspect')['analyses'][0]
    assert row['availability'] == 'unavailable', row
    assert row['support'] == unique[2]['authored']['analyses'][0]['support']
    assert call('app.state')['viewport']['gpu']['sectionClippedActors'] == 0
    call('view.home'); call('capture.screenshot', {'path': str(out/'ambiguous-section.png')})
    call('history.undo')
    assert inspect()['authored'] == unique[2]['authored']
    assert call('analysis.inspect')['analyses'][0]['availability'] == 'available'
    call('history.redo')
    assert inspect()['authored'] == split[2]['authored']
    reopen(split)
    assert call('analysis.inspect')['analyses'][0]['availability'] == 'unavailable'
    report['gates']['ambiguous-topology'] = {'analysis': row,
        'fixture': str(split[0]), 'fixtureSha256': split[1],
        'authoredSupportPreserved': True, 'unavailableRatherThanGuessed': True,
        'repair': 'Undo Split restores available exact face; Redo/reopen retain unavailable analysis'}
    report['gpu'] = call('app.state')['viewport']['gpu']
    assert report['gpu']['hardwareAccelerated']
    report['calls'] = len(transcript)
    report['passed'] = True
    (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    (out/'transcript.json').write_text(json.dumps(transcript, indent=2)+'\n')
print('PASS: downstream invalid dimension, missing face/Project, stale context, ambiguous Split; atomic intent/file/history and repair')
