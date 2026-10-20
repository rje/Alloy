#!/usr/bin/env python3
"""SOL-089: directed chamfer geometry/history through the actual application.

The angle/setback convention is Alloy policy, not a reference-runtime claim.
Native pointer input and GPU visual acceptance require separate evidence.
"""
import argparse
from contextlib import contextmanager
import json
import math
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl


def near(actual, expected, tolerance=1e-5):
    assert abs(actual - expected) < tolerance, (actual, expected)


def ref(face):
    return {key: face[key] for key in ('bodyId', 'producerFeatureId', 'faceId')}


def geometry(body, length=30, distance=2, angle=30, front=False, cut=False):
    other = distance / math.tan(math.radians(angle))
    near(body['volumeMm3'], length * (200 - distance * other / 2) - (math.pi if cut else 0))
    assert body['valid'] and body['closed'] and body['topology']['solids'] == 1
    for key, expected in [('min', [0, 0, 0]), ('max', [length, 20, 10])]:
        for actual, wanted in zip(body['boundsMm'][key], expected):
            near(actual, wanted)
    # Surviving exact source edges report their current kernel endpoints.
    points = [edge[key] for edge in body['solidEdges'] if edge['available']
              for key in ('startMm', 'endMm')]
    top_setback, front_setback = (other, distance) if front else (distance, other)
    for x in (0, length):
        measured = []
        for wanted in ([x, top_setback, 10], [x, 0, 10-front_setback]):
            matches = [point for point in points if all(abs(a-b) < 1e-5 for a, b in zip(point, wanted))]
            assert matches, (wanted, points)
            measured.append(matches[0])
        measured_top, measured_front = measured[0][1], 10-measured[1][2]
        ratio = measured_front / measured_top if front else measured_top / measured_front
        near(math.degrees(math.atan(ratio)), angle)


@contextmanager
def recorded_application(app, out):
    with application(app, out) as (call, transcript):
        try:
            yield call, transcript
        finally:
            (out / 'transcript.json').write_text(json.dumps(transcript, indent=2)+'\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    out = Path(args.output).resolve()
    with recorded_application(args.app, out) as (call, transcript):
        inspect = lambda: call('document.inspect')
        draft = lambda: call('app.state')['activeSession']['draftSketch']
        call('sketch.begin', {'plane': 'XY'})
        call('sketch.rectangle', {'x1': 0, 'y1': 0, 'x2': 30, 'y2': 20, 'units': 'mm'})
        sketch = draft(); edges = sketch['parameters']['entities']
        call('sketch.constraint', {'type': 'fixed', 'pointIds': [edges[0]['startPointId']]})
        call('sketch.dimension', {'kind': 'horizontal', 'entityId': edges[0]['id'], 'value': 30, 'units': 'mm'})
        width = draft()['parameters']['dimensions'][-1]['id']
        call('sketch.dimension', {'kind': 'vertical', 'entityId': edges[1]['id'], 'value': 20, 'units': 'mm'})
        call('sketch.finish')
        solved = inspect()['evaluation']['sketches'][0]
        call('solid.extrude', {'sketchId': solved['featureId'], 'profileId': solved['profiles'][0]['profileId'], 'distance': 10, 'units': 'mm'})
        source = inspect(); stock = source['evaluation']['bodies'][0]; body_id = stock['bodyId']
        edge = next(edge for edge in stock['solidEdges'] if all(abs(edge[key][1]) < 1e-7 and abs(edge[key][2]-10) < 1e-7 for key in ('startMm', 'endMm')))
        top = next(face for face in stock['supportFaces'] if face['frame']['normal'][2] > .99)
        front = next(face for face in stock['supportFaces'] if face['frame']['normal'][1] < -.99)
        back = next(face for face in stock['supportFaces'] if face['frame']['normal'][1] > .99)
        inputs = {'targetBodyId': body_id, 'edgeIds': [edge['edgeId']], 'mode': 'distanceAngle', 'distanceMm': 2, 'angleDeg': 30, 'referenceFace': ref(top)}
        session = call('solid.chamferDialog', inputs); sid = session['id']
        preview = call('command.preview', {'session': sid}); geometry(preview['geometry'])
        call('command.update', {'session': sid, 'inputs': {'referenceFace': ref(front)}})
        reversed_preview = call('command.preview', {'session': sid}); geometry(reversed_preview['geometry'], front=True)
        assert reversed_preview['inputs']['distanceMm'] == 2 and reversed_preview['inputs']['angleDeg'] == 30
        call('command.update', {'session': sid, 'inputs': {'angleDeg': 45}})
        datum = call('command.preview', {'session': sid}); geometry(datum['geometry'], angle=45, front=True)
        call('command.cancel', {'session': sid})
        cancelled = inspect(); assert cancelled['authored'] == source['authored'] and cancelled['undo'] == source['undo']
        session = call('solid.chamferDialog', inputs)
        call('command.commit', {'session': session['id']})
        created = inspect(); feature_id = created['authored']['features'][-1]['id']; geometry(created['evaluation']['bodies'][0])
        call('history.undo'); assert inspect()['authored'] == source['authored']
        call('history.redo'); assert inspect()['authored'] == created['authored']
        call('solid.chamfer', {**inputs, 'featureId': feature_id, 'referenceFace': ref(front)})
        reversed_commit = inspect(); geometry(reversed_commit['evaluation']['bodies'][0], front=True)
        assert reversed_commit['authored']['features'][-1]['parameters']['referenceFace'] == ref(front)
        reverse_edit = call('feature.editDialog', {'featureId': feature_id})
        assert reverse_edit['inputs']['referenceFace'] == ref(front)
        call('command.cancel', {'session': reverse_edit['id']})
        call('history.undo'); assert inspect()['authored'] == created['authored']
        # A real circular blind Cut on the unchanged back plane.
        supported = call('sketch.begin', {'support': ref(back)})
        frame = supported['resolvedFrame']; delta = [p-o for p, o in zip([15, 20, 5], frame['origin'])]
        u = sum(d*a for d, a in zip(delta, frame['uAxis'])); v = sum(d*a for d, a in zip(delta, frame['vAxis']))
        call('sketch.addCircle', {'centerX': u, 'centerY': v, 'diameter': 2, 'units': 'mm'})
        call('sketch.finish'); cut_sketch = inspect()['evaluation']['sketches'][-1]
        call('solid.extrude', {'sketchId': cut_sketch['featureId'], 'profileId': cut_sketch['profiles'][0]['profileId'], 'distance': -1, 'units': 'mm', 'operation': 'cut', 'targetBodyIds': [body_id]})
        downstream = inspect(); geometry(downstream['evaluation']['bodies'][0], cut=True)
        edit_receipts = []
        for change, distance, angle in [({'distanceMm': 3}, 3, 30), ({'angleDeg': 40}, 3, 40)]:
            session = call('feature.editDialog', {'featureId': feature_id})
            assert session['inputs']['referenceFace'] == ref(top) and session['inputs']['edgeIds'] == [edge['edgeId']]
            call('command.update', {'session': session['id'], 'inputs': change})
            current_preview = call('command.preview', {'session': session['id']}); geometry(current_preview['geometry'], distance=distance, angle=angle)
            call('command.commit', {'session': session['id']})
            edited = inspect(); geometry(edited['evaluation']['bodies'][0], distance=distance, angle=angle, cut=True)
            assert edited['authored']['features'][3:] == downstream['authored']['features'][3:]
            edit_receipts.append(edited)
        call('history.undo'); assert inspect()['authored'] == edit_receipts[0]['authored']
        call('history.redo'); assert inspect()['authored'] == edited['authored']
        # Each invalid draft refuses preview and commit, preserves authored/history,
        # and can be corrected within the same session before cancellation.
        for change, code in [({'distanceMm': 0}, 'invalid_arguments'), ({'angleDeg': 0}, 'invalid_arguments'), ({'angleDeg': -30}, 'invalid_arguments'), ({'angleDeg': 90}, 'invalid_arguments'), ({'angleDeg': 120}, 'invalid_arguments'), ({'referenceFace': ref(back)}, 'evaluation_failed'), ({'distanceMm': 100}, 'evaluation_failed')]:
            session = call('feature.editDialog', {'featureId': feature_id}); sid = session['id']
            call('command.update', {'session': sid, 'inputs': change})
            call('command.preview', {'session': sid}, error=code)
            call('command.commit', {'session': sid}, error=code)
            stable = inspect(); assert stable['authored'] == edited['authored'] and stable['undo'] == edited['undo'] and stable['evaluation']['bodies'] == edited['evaluation']['bodies']
            call('command.update', {'session': sid, 'inputs': {'distanceMm': 3, 'angleDeg': 40, 'referenceFace': ref(top)}})
            call('command.preview', {'session': sid}); call('command.cancel', {'session': sid})
        # Negative distance is rejected by discovery argument validation itself.
        session = call('feature.editDialog', {'featureId': feature_id}); sid = session['id']
        call('command.update', {'session': sid, 'inputs': {'distanceMm': -1}}, error='invalid_arguments')
        call('command.preview', {'session': sid}); call('command.cancel', {'session': sid})
        stable = inspect(); assert stable['authored'] == edited['authored'] and stable['undo'] == edited['undo'] and stable['evaluation']['bodies'] == edited['evaluation']['bodies']
        # Restore the principal datum before the specified source length edit.
        call('solid.chamfer', {**inputs, 'featureId': feature_id})
        restored = inspect(); geometry(restored['evaluation']['bodies'][0], cut=True)
        call('sketch.edit', {'sketchId': solved['featureId']})
        call('sketch.dimensionUpdate', {'dimensionId': width, 'value': 36})
        call('sketch.finish'); upstream = inspect(); geometry(upstream['evaluation']['bodies'][0], length=36, cut=True)
        assert upstream['authored']['features'][2:] == restored['authored']['features'][2:]
        call('history.undo'); assert inspect()['authored'] == restored['authored']
        call('history.redo'); assert inspect()['authored'] == upstream['authored']
        model = out / 'distance-angle-chamfer.alloy'
        call('document.save', {'path': str(model)})
        call('document.new', {'discard': True}); call('document.open', {'path': str(model)})
        reopened = inspect(); assert reopened['authored'] == upstream['authored'] and reopened['evaluation']['bodies'] == upstream['evaluation']['bodies']
        stl = out / 'distance-angle-chamfer.stl'
        call('document.exportStl', {'path': str(stl), 'bodyId': body_id, 'units': 'mm', 'format': 'binary', 'surfaceDeviationMm': .02, 'normalDeviationDeg': 5})
        mesh = inspect_stl(stl, True); assert len(mesh['components']) == 1
        near(mesh['signedVolume'], 36*(200-2*math.sqrt(3))-math.pi, .03)
        for actual, expected in zip(mesh['bounds'], [[0, 0, 0], [36, 20, 10]]):
            for value, wanted in zip(actual, expected): near(value, wanted, .02)
        (out / 'receipt.json').write_text(json.dumps({'passed': True, 'scope': 'API/geometry/history/reopen/STL; native input and visual acceptance separate', 'source': source, 'preview': preview, 'reversedPreview': reversed_preview, 'reversedCommit': reversed_commit, 'datum45': datum, 'created': created, 'downstream': downstream, 'edits': edit_receipts, 'upstream': upstream, 'reopened': reopened, 'mesh': mesh, 'calls': len(transcript)}, indent=2)+'\n')
        (out / 'transcript.json').write_text(json.dumps(transcript, indent=2)+'\n')
    print('PASS Distance And Angle Chamfer API geometry/face assignment/Cancel/edit/downstream/refusal/history/reopen/STL')


if __name__ == '__main__':
    main()
