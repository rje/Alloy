#!/usr/bin/env python3
"""Focused real-app solved-color evidence; consumes the accepted lesson model."""
import argparse
import hashlib
import json
from pathlib import Path
from app_harness import application


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--app', required=True)
    parser.add_argument('--model', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    out = Path(args.output).resolve()
    out.mkdir(parents=True, exist_ok=True)
    report = {'status': 'incomplete', 'appSha256': hashlib.sha256(Path(args.app).read_bytes()).hexdigest()}
    transcript = []
    try:
        with application(args.app, out) as (call, transcript):
            call('document.open', {'path': str(Path(args.model).resolve())})
            original = call('document.inspect')['authored']
            sketch = original['features'][2]
            parameters = sketch['parameters']
            ellipse = next(e for e in parameters['entities'] if e['type'] == 'ellipse')
            conic = next(e for e in parameters['entities'] if e['type'] == 'conic')
            axis = next(e for e in parameters['entities'] if e['id'] == ellipse['firstAxisId'])
            axis_points = {axis['startPointId'], axis['endPointId']}
            dimension = next(d for d in parameters['dimensions'] if set(d.get('pointIds', [])) == axis_points)
            report.update(ellipseId=ellipse['id'], conicId=conic['id'], removedDimensionId=dimension['id'])
            call('window.resize', {'width': 1500, 'height': 1000})
            call('sketch.edit', {'sketchId': sketch['id']})
            call('sketch.lookAt')
            call('sketch.selectionSet', {'entityIds': [], 'pointIds': [], 'profileIds': []})

            def capture(name, expected_dof):
                state = call('app.state')
                diagnostics = state['activeSession']['solverDiagnostics']
                assert diagnostics['dof'] == expected_dof
                freedom = diagnostics['entityFreedom']
                assert freedom[ellipse['id']]['determinate']
                assert freedom[ellipse['id']]['fullyConstrained'] == (expected_dof == 7)
                assert not freedom[conic['id']]['fullyConstrained']
                call('capture.screenshot', {'path': str(out/(name+'.png'))})
                (out/(name+'.json')).write_text(json.dumps(state, indent=2)+'\n')
                report[name] = state['activeSession']['solverDiagnostics']
                return state

            capture('defined-ellipse-free-conic', 7)
            call('sketch.selectionSet', {'entityIds': [ellipse['id']]})
            capture('selected-defined-ellipse', 7)
            call('sketch.selectionSet', {'entityIds': []})
            call('sketch.deleteSelection', {'dimensionId': dimension['id']})
            capture('underdefined-ellipse-free-conic', 8)
            call('sketch.cancel')
            restored = call('document.inspect')
            assert restored['authored'] == original
            finished = next(s for s in restored['evaluation']['sketches'] if s['sketchId'] == sketch['id'])
            assert finished['solver']['entityFreedom'][ellipse['id']]['fullyConstrained']
            assert not finished['solver']['entityFreedom'][conic['id']]['fullyConstrained']
            call('capture.screenshot', {'path': str(out/'finished-mixed-sketch.png')})
            report['finishedSolver'] = finished['solver']
            report['status'] = 'api-pass'
    finally:
        report['callCount'] = len(transcript)
        (out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
        (out/'transcript.json').write_text(json.dumps(transcript, indent=2)+'\n')
    print('PASS: mixed sketch solved/free/selected evidence; original document unchanged')


if __name__ == '__main__':
    main()
