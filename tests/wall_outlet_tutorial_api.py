#!/usr/bin/env python3
"""Prepared source-order wall-outlet sketch gate; requires SKT-051/052/054.

No pass is implied until the real app run and independent review complete.
Movie is a shared-API creation record, separate from child native input evidence.
"""
import argparse
from contextlib import ExitStack
import json
from pathlib import Path
import time
from app_harness import application
from wall_outlet_oracle import verify


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    parser.add_argument('--record-video', type=Path)
    args = parser.parse_args()
    output = Path(args.output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    report = {'result': 'incomplete', 'sourceId': 'wall-outlet-sketch-20261013',
              'scope': 'Sketch only; source ends at Finish Sketch, no solid or STL.',
              'roughSeeds': {'outer': '66x108 centered at origin',
                             'upper': 'x -16..15, y 13..40, outward through x -25/24',
                             'circleDiameter': 10},
              'evidence': 'Shared API movie; child native receipts remain separate.', 'stages': []}
    transcript = []
    recording = False
    try:
        with application(args.app, output) as (raw, transcript), ExitStack() as cleanup:
            def stop():
                nonlocal recording
                if recording:
                    recording = False
                    report['recording'] = raw('recording.stop')
            cleanup.callback(stop)
            raw('window.resize', {'width': 1400, 'height': 1000})
            assert not raw('document.inspect')['authored']['features']
            if args.record_video:
                movie = args.record_video.resolve()
                assert not movie.is_relative_to(Path(__file__).resolve().parents[1])
                movie.parent.mkdir(parents=True, exist_ok=True)
                raw('recording.start', {'path': str(movie), 'fps': 10})
                recording = True
            def call(command, args=None):
                result = raw(command, args)
                if recording and command.startswith(('sketch.', 'view.')):
                    time.sleep(.25)
                return result
            def active(): return raw('app.state')['activeSession']
            def draft(): return active()['draftSketch']
            def params(): return draft()['parameters']
            def pixel(u, v):
                p = raw('view.projectSketchPoint', {'sketchId': draft()['id'], 'uMm': u, 'vMm': v})
                return {'x': p['x'], 'y': p['y']}
            def click(u, v): return call('sketch.placePoint', pixel(u, v))
            def stage(name):
                report['stages'].append(name)
                if recording:
                    raw('recording.step', {'text': 'API | ' + name})
                    time.sleep(.6)
            def capture(name):
                raw('capture.screenshot', {'path': str(output / (name + '.png'))})
                (output / (name + '.json')).write_text(json.dumps(raw('app.state'), indent=2)+'\n')
            def dimension(kind, entity_ids, value, label):
                before = {d['id'] for d in params()['dimensions']}
                targets = {'entityId': entity_ids[0]} if len(entity_ids) == 1 else {'entityIds': entity_ids}
                call('sketch.dimension', {'kind': kind, **targets,
                     'value': value, 'units': 'mm', 'driving': True,
                     'annotation': {'uMm': label[0], 'vMm': label[1]}})
                new = [d for d in params()['dimensions'] if d['id'] not in before]
                assert len(new) == 1
                return new[0]['id']

            stage('A1: Top-plane sketch; rough Center Rectangle')
            call('sketch.begin', {'plane': 'XY', 'name': 'Fully defined outlet sketch'})
            call('sketch.lookAt')
            call('view.zoom', {'factor': .55})
            call('sketch.tool', {'tool': 'centerRectangle'})
            click(0, 0); click(33, 54)
            assert active()['solverDiagnostics']['dof'] == 2
            assert not params()['dimensions']
            outer = params()['entities'][:4]
            capture('01-rough-rectangle')
            stage('A1: Midpoint construction line and origin circle')
            call('sketch.tool', {'tool': 'line'})
            click(-33, 0); click(33, 0)
            axis_id = params()['entities'][-1]['id']
            call('sketch.tool', {'tool': 'select'})
            call('sketch.construction', {'entityIds': [axis_id], 'construction': True})
            call('sketch.tool', {'tool': 'circle'})
            click(0, 0); click(5, 0)
            circle = next(e for e in params()['entities'] if e['type'] == 'circle')
            stage('A1: Rough upper rectangle; vertical sides Construction; outward arcs')
            call('sketch.tool', {'tool': 'rectangle'})
            click(-16, 13); click(15, 40)
            upper = params()['entities'][-4:]
            call('sketch.construction', {'entityIds': [upper[1]['id'], upper[3]['id']], 'construction': True})
            arcs = []
            for start, end, through in [((-16, 40), (-16, 13), (-25, 26.5)),
                                         ((15, 13), (15, 40), (24, 26.5))]:
                call('sketch.tool', {'tool': 'threePointArc'})
                click(*start); click(*end); click(*through)
                arcs.append(params()['entities'][-1])
            capture('02-upper-cutout')
            stage('A1: Window-select objects; Mirror Line; inspect and accept reflection')
            call('sketch.mirrorStart')
            a, b = pixel(-29, 9), pixel(29, 44)
            selection = call('sketch.mirrorWindow', {'x1': a['x'], 'y1': a['y'], 'x2': b['x'], 'y2': b['y']})
            report['mirrorCollection'] = selection
            call('sketch.mirrorRole', {'role': 'axis'})
            call('sketch.mirrorPick', pixel(22, 0))
            call('sketch.mirrorStagePreview')
            capture('03-mirror-preview')
            call('sketch.mirrorAccept')
            assert len(params()['mirrorGroups'][0]['entityPairs']) == 6
            stage('A2: Concentric arcs; Vertical centers; Equal arc radii')
            call('sketch.constraint', {'type': 'concentric', 'entityIds': [e['id'] for e in arcs]})
            call('sketch.constraint', {'type': 'horizontalVertical',
                 'pointIds': [circle['centerPointId'], arcs[0]['centerPointId']]})
            call('sketch.constraint', {'type': 'equal', 'entityIds': [e['id'] for e in arcs]})
            call('sketch.displayOptions', {'showConstraints': True, 'showDimensions': True})
            capture('04-geometric-constraints')
            stage('A3: Overall 70 and 114; circle diameter 12')
            dimension('horizontal', [outer[2]['id']], 70, (0, 65))
            dimension('vertical', [outer[1]['id']], 114, (43, 0))
            dimension('diameter', [circle['id']], 12, (13, -7))
            stage('A3: Parallel-line distances 11 and 28; arc radius 17')
            distance_id = dimension('parallelLineDistance', [upper[0]['id'], axis_id], 11, (23, 5.5))
            dimension('parallelLineDistance', [upper[2]['id'], upper[0]['id']], 28, (26, 25))
            dimension('radius', [arcs[1]['id']], 17, (21, 44))
            report['finalGeometry'] = verify(draft(), active()['solverDiagnostics'])
            final_draft = draft()
            capture('05-fully-defined')
            stage('A3: Finish Sketch')
            call('sketch.finish')
            model = output / 'wall-outlet-sketch.alloy'
            call('document.save', {'path': str(model)})
            capture('06-finished')
            stop()
            authored = raw('document.inspect')['authored']
            assert len(authored['features']) == 1
            call('sketch.edit', {'sketchId': final_draft['id']})
            call('sketch.dimensionUpdate', {'dimensionId': distance_id, 'value': 12, 'units': 'mm'})
            edited = draft()
            assert edited != final_draft and active()['solverDiagnostics']['dof'] == 0
            call('history.undo')
            assert draft() == final_draft
            call('history.redo')
            assert draft() == edited
            call('sketch.cancel')
            call('document.open', {'path': str(model), 'discard': True})
            assert raw('document.inspect')['authored'] == authored
            report.update(result='passed', savedModel=str(model), authored=authored)
    finally:
        report['transcript'] = transcript
        (output / 'wall-outlet-report.json').write_text(json.dumps(report, indent=2)+'\n')
    print('PASS: full wall-outlet sketch source workflow and edit/history/reopen')


if __name__ == '__main__':
    main()
