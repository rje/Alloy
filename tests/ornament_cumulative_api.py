#!/usr/bin/env python3
"""Draft cumulative ornament API gate; run only after dependencies and GUI lease."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import traceback
import subprocess
import time
from contextlib import ExitStack
from app_harness import application
from ornament_source_api import create_seed
from stl_export_api import inspect_stl


def run(call, out, report, stage, stop_recording):
    def inspect(): return call('document.inspect')
    def capture(name): call('capture.screenshot', {'path': str(out/(name+'.png'))})
    def body(state):
        rows = state['evaluation']['bodies']
        assert len(rows) == 1, rows
        row = rows[0]
        assert row['valid'] and row['closed'] and row['volumeMm3'] > 0
        assert row['topology']['solids'] == 1
        return row
    def occurrences(state, count):
        rows = state['evaluation']['bodies']
        assert len(rows) == count
        rows = sorted(rows, key=lambda b: b['circularPattern']['index'])
        for i, row in enumerate(rows):
            assert row['valid'] and row['closed'] and row['topology']['solids'] == 1
            meta = row['circularPattern']
            assert meta['index'] == i and meta['quantity'] == count
            assert meta['axis'] == {'kind': 'originAxis', 'axis': 'Z'} and meta['extent'] == 'full'
            assert math.isclose(meta['angleDeg'], 360*i/count, abs_tol=1e-8)
            assert math.isclose(row['volumeMm3'], rows[0]['volumeMm3'], rel_tol=1e-8)
        assert rows[0]['bodyId'] == seed['bodyId']
        return rows

    report['stage'] = 'source-prefix'
    seed = create_seed(call, out, stage)
    report['seed'] = seed
    report['stage'] = 'circular-pattern'
    stage('Circular Pattern: Bodies, Origin Z, Full, total quantity 8 including seed')
    call('solid.circularPattern', {'sourceBodyId': seed['bodyId'],
         'axis': {'kind': 'originAxis', 'axis': 'Z'}, 'extent': 'full', 'quantity': 8})
    eight = inspect(); rows = occurrences(eight, 8)
    pattern = next(f for f in eight['authored']['features'] if f['type'] == 'circularPattern')
    ids = [b['bodyId'] for b in rows]
    report['patternBodyIds'] = ids
    call('view.home'); capture('eight-retained-bodies')
    pattern_path = out/'ornament-pattern-eight.alloy'
    call('document.save', {'path': str(pattern_path)})

    report['stage'] = 'eight-participant-hub-join'
    stage('Create bottom hub: diameter 10 mm, Extrude 5 mm downward and Join all eight ribs')
    call('sketch.begin', {'plane': 'XY', 'name': 'Bottom hub diameter 10'})
    call('sketch.addCircle', {'centerX': 0, 'centerY': 0, 'diameter': 10, 'units': 'mm'})
    hub_id = call('app.state')['activeSession']['draftSketch']['id']
    call('sketch.finish')
    hub_profile = inspect()['evaluation']['sketches'][-1]['profiles'][0]['profileId']
    call('solid.extrude', {'sketchId': hub_id, 'profileId': hub_profile, 'distance': -5,
         'units': 'mm', 'operation': 'join', 'targetBodyIds': ids})
    joined = inspect(); joined_body = body(joined)
    assert joined_body['bodyId'] == ids[0]
    join_feature = joined['authored']['features'][-1]
    ledger = join_feature['bodyResults']
    # Join walks authored targetBodyIds in supplied order; it sorts profile IDs only.
    assert join_feature['parameters']['targetBodyIds'] == ids
    assert len(ledger) == 1 and ledger[0]['participantBodyIds'] == ids
    report['hubJoin'] = joined_body
    call('view.home'); capture('eight-ribs-hub-join')

    report['stage'] = 'transverse-hole'
    stage('Create hanging hole: diameter 4 mm, Cut through the bottom hub')
    plane_id = call('construction.plane', {'source': {'plane': 'XZ'}, 'offset': 10,
                    'units': 'mm', 'name': 'Bore start y -10'})['planeId']
    # Creation returns planeId; evaluated construction plane records expose featureId.
    plane = next(p for p in inspect()['evaluation']['constructionPlanes'] if p['featureId'] == plane_id)
    frame = plane['frame']
    for key, expected in (('origin', [0, -10, 0]), ('normal', [0, -1, 0]),
                          ('uAxis', [1, 0, 0]), ('vAxis', [0, 0, 1])):
        assert len(frame[key]) == 3
        assert all(math.isclose(a, b, abs_tol=1e-8) for a, b in zip(frame[key], expected)), frame
    # sketch.begin returns the active session directly, including resolvedFrame.
    session = call('sketch.begin', {'planeId': plane_id, 'name': 'Transverse diameter 4 bore'})
    assert session['resolvedFrame'] == frame
    call('sketch.addCircle', {'centerX': 0, 'centerY': -2.5, 'diameter': 4, 'units': 'mm'})
    hole_id = call('app.state')['activeSession']['draftSketch']['id']
    call('sketch.finish')
    hole_profile = inspect()['evaluation']['sketches'][-1]['profiles'][0]['profileId']
    # Verified normal -Y and origin y=-10: negative20 reaches y=+10.
    distance = -20
    assert math.isclose(frame['origin'][1]+frame['normal'][1]*distance, 10, abs_tol=1e-8)
    call('solid.extrude', {'sketchId': hole_id, 'profileId': hole_profile, 'distance': distance,
         'units': 'mm', 'operation': 'cut', 'targetBodyIds': [joined_body['bodyId']]})
    original = inspect(); original_body = body(original)
    assert original_body['bodyId'] == joined_body['bodyId']
    assert original_body['volumeMm3'] < joined_body['volumeMm3']-1
    report['holeFixture'] = {'planeId': plane_id, 'frame': frame, 'centerUvMm': [0, -2.5],
                             'diameterMm': 4, 'distanceMm': distance,
                             'policy': 'Disclosed Alloy transverse coordinates; source exact hole position unknown.'}
    call('view.home'); capture('original-bore-result')
    stage('Save the editable ornament')
    original_path = out/'ornament-original.alloy'
    call('document.save', {'path': str(original_path)})
    report['original'] = original
    stop_recording()
    report['stage'] = 'original-strict-mm-stl'
    original_mesh = out/'ornament-original-mm.stl'
    report['originalExport'] = call('document.exportStl', {'path': str(original_mesh), 'bodyId': original_body['bodyId'],
        'units': 'mm', 'format': 'binary', 'surfaceDeviationMm': .05, 'normalDeviationDeg': 8})
    report['originalMesh'] = inspect_stl(original_mesh, True)
    assert len(report['originalMesh']['components']) == 1
    assert report['originalMesh']['triangles'] == report['originalExport']['triangles']
    assert report['originalExport']['bytes'] == original_mesh.stat().st_size
    assert report['originalExport']['bodyIds'] == [original_body['bodyId']]
    assert report['originalExport']['authoredChanged'] is False
    assert math.isclose(report['originalMesh']['signedVolume'], original_body['volumeMm3'], rel_tol=.01)
    assert inspect()['authored'] == original['authored']

    report['stage'] = 'pre-join-count-history-validation'
    call('document.open', {'path': str(pattern_path), 'discard': True})
    assert inspect()['authored'] == eight['authored']
    call('solid.circularPattern', {'featureId': pattern['id'], 'sourceBodyId': seed['bodyId'],
         'axis': {'kind': 'originAxis', 'axis': 'Z'}, 'extent': 'full', 'quantity': 10})
    ten = inspect()
    assert [b['bodyId'] for b in occurrences(ten, 10)][:8] == ids
    call('history.undo'); assert inspect()['authored'] == eight['authored']
    call('history.redo'); assert inspect()['authored'] == ten['authored']
    call('history.undo'); assert inspect()['authored'] == eight['authored']

    report['patternCountHistory'] = {'quantitySequence': [8, 10, 8, 10, 8],
        'checkedOnPreJoinCheckpointAfterCreationMovie': True}
    call('document.open', {'path': str(original_path), 'discard': True})
    restored_original = inspect()
    assert restored_original['authored'] == original['authored']
    assert body(restored_original)['bodyId'] == original_body['bodyId']
    assert math.isclose(body(restored_original)['volumeMm3'], original_body['volumeMm3'], rel_tol=1e-9)

    report['stage'] = 'upstream-offset-edit'
    call('sketch.edit', {'sketchId': seed['stripSketchId']})
    call('sketch.splineOffsetUpdate', {'offsetEntityId': seed['offsetEntityId'], 'distance': -4, 'units': 'mm'})
    call('sketch.finish')
    edited = inspect(); edited_body = body(edited)
    assert edited_body['bodyId'] == original_body['bodyId']
    assert not math.isclose(edited_body['volumeMm3'], original_body['volumeMm3'], abs_tol=1e-4)
    assert next(f for f in edited['authored']['features'] if f['id'] == pattern['id']) == pattern
    rebuilt_join = next(f for f in edited['authored']['features'] if f['id'] == join_feature['id'])
    assert rebuilt_join['parameters']['targetBodyIds'] == ids and rebuilt_join['bodyResults'] == ledger
    call('history.undo'); assert inspect()['authored'] == original['authored']
    call('history.redo'); assert inspect()['authored'] == edited['authored']
    edited_path = out/'ornament-offset-4.alloy'
    call('document.save', {'path': str(edited_path)})
    call('document.open', {'path': str(original_path), 'discard': True})
    reopened_original = inspect()
    assert reopened_original['authored'] == original['authored']
    reopened_original_body = body(reopened_original)
    assert reopened_original_body['bodyId'] == original_body['bodyId']
    assert math.isclose(reopened_original_body['volumeMm3'], original_body['volumeMm3'], rel_tol=1e-9)
    call('document.open', {'path': str(edited_path), 'discard': True})
    reopened = inspect(); assert reopened['authored'] == edited['authored']
    assert body(reopened)['bodyId'] == edited_body['bodyId']
    assert math.isclose(body(reopened)['volumeMm3'], edited_body['volumeMm3'], rel_tol=1e-9)
    report['edited'] = reopened
    call('view.home'); capture('edited-offset-4-result')

    report['stage'] = 'strict-mm-stl'
    mesh_path = out/'ornament-offset-4-mm.stl'
    report['export'] = call('document.exportStl', {'path': str(mesh_path), 'bodyId': edited_body['bodyId'],
        'units': 'mm', 'format': 'binary', 'surfaceDeviationMm': .05, 'normalDeviationDeg': 8})
    report['mesh'] = inspect_stl(mesh_path, True)
    assert len(report['mesh']['components']) == 1
    assert report['mesh']['triangles'] == report['export']['triangles']
    assert report['export']['bytes'] == mesh_path.stat().st_size
    assert report['export']['bodyIds'] == [edited_body['bodyId']]
    assert report['export']['authoredChanged'] is False
    assert math.isclose(report['mesh']['signedVolume'], edited_body['volumeMm3'], rel_tol=.01)
    assert inspect()['authored'] == edited['authored']
    report['gpu'] = call('app.state')['viewport']['gpu']
    assert report['gpu']['hardwareAccelerated']
    report['stage'] = 'saved-model-geometry-verifier'
    verifier = subprocess.run([report['geometryVerifier'], str(original_path), str(edited_path)],
        capture_output=True, text=True, timeout=report['geometryVerifierTimeoutSeconds'])
    (out/'geometry-verifier.log').write_text(verifier.stdout+verifier.stderr)
    assert verifier.returncode == 0, verifier.stdout+verifier.stderr
    report['geometry'] = json.loads(verifier.stdout)
    assert report['geometry']['status'] == 'geometry-pass'
    report['stage'] = 'complete'
    report['status'] = 'cumulative-api-pass'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--record-video', type=Path)
    parser.add_argument('--geometry-verifier', required=True, help='Built ornament_geometry_verify executable.')
    parser.add_argument('--geometry-verifier-timeout', type=float, default=600)
    parser.add_argument('--command-timeout', type=float, default=30,
                        help='Local client seconds per command; use e.g.120 for known slow Join.')
    args = parser.parse_args()
    assert math.isfinite(args.geometry_verifier_timeout) and args.geometry_verifier_timeout > 0
    assert math.isfinite(args.command_timeout) and 0 < args.command_timeout <= 2147483
    out = Path(args.output).resolve(); out.mkdir(parents=True, exist_ok=True)
    movie = args.record_video.resolve() if args.record_video else None
    if movie:
        assert not movie.is_relative_to(Path(__file__).resolve().parents[1])
        movie.parent.mkdir(parents=True, exist_ok=True)
    transcript = []
    recording = False
    report = {'status': 'incomplete', 'fullTutorialAccepted': False, 'nativeInputParityClaimed': False,
              'movieRecorded': False, 'commandTimeoutSeconds': args.command_timeout,
              'geometryVerifier': str(Path(args.geometry_verifier).resolve()),
              'geometryVerifierSha256': hashlib.sha256(Path(args.geometry_verifier).read_bytes()).hexdigest(),
              'geometryVerifierTimeoutSeconds': args.geometry_verifier_timeout,
              'appSha256': hashlib.sha256(Path(args.app).read_bytes()).hexdigest()}
    try:
        with application(args.app, out, command_timeout=args.command_timeout) as (raw_call, transcript), ExitStack() as cleanup:
            def stop_recording():
                nonlocal recording
                if recording:
                    recording = False
                    report['recording'] = raw_call('recording.stop')
                    report['movieRecorded'] = True
            cleanup.callback(stop_recording)
            raw_call('window.resize', {'width': 1400, 'height': 1000})
            assert not raw_call('document.inspect')['authored']['features']
            if movie:
                raw_call('recording.start', {'path': str(movie), 'fps': 10})
                recording = True
                time.sleep(.5)
            def stage(caption):
                report.setdefault('creationStages', []).append(caption)
                if recording:
                    raw_call('recording.step', {'text': 'API | '+caption})
                    time.sleep(.45)
            def call(command, parameters=None):
                report['pendingCommand'] = {'command': command, 'args': parameters or {}}
                result = raw_call(command, parameters)
                report.pop('pendingCommand', None)
                if recording and command.startswith(('sketch.', 'solid.', 'selection.', 'command.', 'view.', 'analysis.')):
                    time.sleep(.2)
                return result
            run(call, out, report, stage, stop_recording)
    except BaseException as error:
        report['status'] = 'failed'
        report['failure'] = {'type': type(error).__name__, 'message': str(error), 'traceback': traceback.format_exc()}
        raise
    finally:
        (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
        (out/'transcript.json').write_text(json.dumps(transcript, indent=2)+'\n')


if __name__ == '__main__':
    main()
