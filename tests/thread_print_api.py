#!/usr/bin/env python3
"""Real-app retained external thread, source edit and independent printable STL gate."""
import argparse
import json
from pathlib import Path

from app_harness import application
from stl_export_api import inspect_stl


def feature(state, kind):
    return next(row for row in state['authored']['features'] if row['type'] == kind)


def body(state, body_id):
    return next(row for row in state['evaluation']['bodies'] if row['bodyId'] == body_id)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    out = Path(args.output).resolve()
    out.mkdir(parents=True, exist_ok=True)
    with application(args.app, out, command_timeout=120) as (call, transcript):
        discovered = call('commands.list')
        assert any(row['id'] == 'solid.thread' for row in discovered['commands'])
        call('solid.cylinder', {'radius': 4, 'height': 14, 'units': 'mm', 'name': 'M8 boss'})
        initial = call('document.inspect')
        source = feature(initial, 'cylinder')
        stock = initial['evaluation']['bodies'][0]
        support = next(row for row in stock['supportFaces'] if row['role'] == 'cylinder.lateral')
        face = {key: support[key] for key in ('producerFeatureId', 'bodyId', 'faceId')}
        assert not support['planar']
        assert body(initial, stock['bodyId'])['boundsMm']['max'][2] == 14

        call('solid.thread', {'support': face, 'nominalDiameterMm': 8, 'pitchMm': 1.25,
            'fullLength': False, 'startOffsetMm': 2, 'lengthMm': 10, 'direction': 'right'})
        threaded = call('document.inspect')
        thread = feature(threaded, 'thread')
        assert thread['parameters']['support'] == face
        assert thread['parameters']['lengthMm'] == 10
        assert len(threaded['authored']['features']) == len(initial['authored']['features']) + 1
        assert body(threaded, stock['bodyId'])['volumeMm3'] < stock['volumeMm3']

        # An impossible interval refuses atomically, leaving authored and undo state intact.
        call('solid.thread', {'featureId': thread['id'], 'lengthMm': 100}, error='evaluation_failed')
        refused = call('document.inspect')
        assert refused['authored'] == threaded['authored'] and refused['undo'] == threaded['undo']
        call('solid.thread', {'featureId': thread['id'], 'lengthMm': 8})
        shorter = call('document.inspect')
        assert feature(shorter, 'thread')['parameters']['lengthMm'] == 8
        call('history.undo')
        assert call('document.inspect')['authored'] == threaded['authored']
        call('history.redo')
        assert call('document.inspect')['authored'] == shorter['authored']

        call('feature.edit', {'featureId': source['id'], 'parameters': {'heightMm': 16}})
        taller = call('document.inspect')
        assert abs(body(taller, stock['bodyId'])['boundsMm']['max'][2] - 16) < 0.01, body(taller, stock['bodyId'])['boundsMm']
        assert feature(taller, 'thread')['parameters']['support'] == face
        design = out/'threaded-boss.alloy'
        call('document.save', {'path': str(design)})
        stl = out/'threaded-boss.stl'
        call('document.exportStl', {'path': str(stl), 'bodyId': stock['bodyId'],
            'units': 'mm', 'format': 'binary', 'surfaceDeviationMm': 0.05, 'normalDeviationDeg': 8})
        mesh = inspect_stl(stl, True)
        assert len(mesh['components']) == 1 and mesh['triangles'] > 1000
        assert all(abs(observed - expected) < 0.03 for observed, expected in
                   zip(mesh['bounds'][0], [-4, -4, 0]))
        assert all(abs(observed - expected) < 0.03 for observed, expected in
                   zip(mesh['bounds'][1], [4, 4, 16]))
        call('capture.screenshot', {'path': str(out/'threaded-boss.png')})
        call('document.open', {'path': str(design)})
        reopened = call('document.inspect')
        assert reopened['authored'] == taller['authored']
        assert body(reopened, stock['bodyId'])['boundsMm'] == body(taller, stock['bodyId'])['boundsMm']
        (out/'receipt.json').write_text(json.dumps({'passed': True, 'mesh': mesh,
            'threadFeatureId': thread['id'], 'support': face, 'transcript': transcript}, indent=2)+'\n')
    print('PASS retained external thread, atomic refusal, edit, history, reopen, printable STL')


if __name__ == '__main__':
    main()
