#!/usr/bin/env python3
"""Real-app valid-solid edge-deletion refusal after a stepped-face offset."""
import argparse
import hashlib
import json
import math
from pathlib import Path

from app_harness import application


def edge_with_ends(body, first, last):
    expected = sorted((first, last))
    matches = [edge for edge in body['solidEdges']
               if edge['available'] and sorted((edge['startMm'], edge['endMm'])) == expected]
    assert len(matches) == 1, matches
    return matches[0]


parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--app', required=True)
parser.add_argument('--fixture', required=True)
parser.add_argument('--output', required=True)
args = parser.parse_args()
out = Path(args.output).resolve()
out.mkdir(parents=True, exist_ok=True)
fixture = Path(args.fixture).resolve()

with application(args.app, out) as (call, transcript):
    def inspect():
        return call('document.inspect')

    call('document.open', {'path': str(fixture)})
    source = inspect()
    original = source['evaluation']['bodies'][0]
    assert original['valid'] and original['closed']
    assert math.isclose(original['volumeMm3'], 1500, abs_tol=1e-5)
    deleted = [edge_with_ends(original, [10, y, 5], [10, y, 10])
               for y in (0, -10)]
    stable = edge_with_ends(original, [0, 0, 0], [20, 0, 0])
    ledges = [face for face in original['supportFaces']
              if face['frame']['origin'] == [20, 0, 5]
              and face['frame']['normal'][2] > .99]
    assert len(ledges) == 1, ledges
    face = {key: ledges[0][key] for key in ('bodyId', 'producerFeatureId', 'faceId')}
    inputs = {'targetBodyId': original['bodyId'], 'face': face, 'distanceMm': 5}
    call('solid.faceOffset', inputs)
    offset = inspect()
    result = offset['evaluation']['bodies'][0]
    assert result['valid'] and result['closed'] and result['topology']['solids'] == 1
    assert math.isclose(result['volumeMm3'], 2000, abs_tol=1e-5)
    by_id = {edge['edgeId']: edge for edge in result['solidEdges']}
    for edge in deleted:
        row = by_id[edge['edgeId']]
        assert not row['available'] and row['provenance']['resolutionError'] == 'deleted', row
    assert by_id[stable['edgeId']]['available']
    feature = offset['authored']['features'][-1]['id']
    call('view.orientation', {'name': 'isometric'})
    call('view.fit')
    call('capture.screenshot', {'path': str(out / 'stepped-offset-valid-solid.png')})

    # A deleted exact edge cannot silently become a nearby edge. Refusal is
    # atomic, while an unrelated exact bottom edge is still inspectable.
    call('solid.fillet', {'targetBodyId': original['bodyId'],
                          'edgeIds': [deleted[0]['edgeId']], 'radiusMm': .5},
         error='evaluation_failed')
    refusal = transcript[-1]['response']['error']
    assert refusal['diagnostics']['code'] == 'fillet_edge_deleted', refusal
    assert inspect() == offset
    pixel = call('view.projectEdge', {'bodyId': original['bodyId'],
                                      'edgeId': stable['edgeId']})
    assert math.isfinite(pixel['x']) and math.isfinite(pixel['y'])

    # The disappearing edge is available again before the ledge closes, then
    # explicitly unavailable again after restoring the exact source edit.
    call('solid.faceOffset', {**inputs, 'featureId': feature, 'distanceMm': 4})
    before = inspect()
    shorter = next(edge for edge in before['evaluation']['bodies'][0]['solidEdges']
                   if edge['edgeId'] == deleted[0]['edgeId'])
    assert shorter['available'], shorter
    call('solid.faceOffset', {**inputs, 'featureId': feature})
    restored = inspect()
    assert not next(edge for edge in restored['evaluation']['bodies'][0]['solidEdges']
                    if edge['edgeId'] == deleted[0]['edgeId'])['available']
    call('history.undo')
    assert inspect()['authored'] == before['authored']
    call('history.redo')
    assert inspect()['authored'] == restored['authored']
    saved = out / 'stepped-offset-deletion.alloy'
    call('document.save', {'path': str(saved)})
    call('document.new', {'discard': False})
    call('document.open', {'path': str(saved)})
    reopened = inspect()
    assert reopened['authored'] == restored['authored']
    assert not next(edge for edge in reopened['evaluation']['bodies'][0]['solidEdges']
                    if edge['edgeId'] == deleted[0]['edgeId'])['available']
    gpu = call('app.state')['viewport']['gpu']
    assert gpu['hardwareAccelerated'], gpu
    (out / 'receipt.json').write_text(json.dumps({
        'status': 'passed', 'scope': 'valid-solid stepped ledge deletes exact short edge',
        'fixtureSha256': hashlib.sha256(fixture.read_bytes()).hexdigest(),
        'gpu': gpu, 'deletedEdgeIds': [edge['edgeId'] for edge in deleted],
        'unaffectedEdgeId': stable['edgeId'], 'refusal': refusal,
        'exactAuthoredUndoRedoReopen': True, 'calls': len(transcript)
    }, indent=2) + '\n')
    (out / 'transcript.json').write_text(json.dumps(transcript, indent=2) + '\n')

print('PASS: valid-solid offset edge deletion, atomic Fillet refusal and reopen')
