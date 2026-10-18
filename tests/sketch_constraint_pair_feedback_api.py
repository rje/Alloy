#!/usr/bin/env python3
"""Retained native-tool pick state and atomic refusal through the real local API."""
import argparse
import json
from pathlib import Path
from app_harness import application


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    output = Path(args.output).resolve()
    with application(args.app, output) as (call, transcript):
        call('sketch.begin', {'plane': 'XY', 'name': 'Analytic pair eligibility'})

        def state():
            return call('app.state')['activeSession']

        def last():
            return state()['draftSketch']['parameters']['entities'][-1]

        call('sketch.line', {'x1': -35, 'y1': -10, 'x2': -35, 'y2': 10, 'units': 'mm'})
        line = last()
        call('sketch.addCircle', {'centerX': 0, 'centerY': 0, 'diameter': 16, 'units': 'mm'})
        circle = last()
        call('sketch.addArc', {'mode': 'center', 'centerX': 30, 'centerY': 0, 'radius': 8,
                             'startAngleDeg': -90, 'sweepAngleDeg': 180, 'units': 'mm'})
        arc = last()
        rows = [('concentric', circle, line, 'wrong_type'),
                ('collinear', line, circle, 'wrong_type'),
                ('symmetry', arc, line, 'unsupported_pair'),
                ('tangent', circle, arc, 'unsupported_pair')]
        receipt = []
        for family, first, wrong, expected in rows:
            call('sketch.tool', {'tool': 'select'})
            call('sketch.selectionSet')
            call('sketch.constraintTool', {'type': family})
            assert state()['constraintPickStatus'] == 'pending_first'
            call('sketch.selectionSet', {'entityIds': [first['id']]})
            before = state()
            document = call('document.inspect')
            call('sketch.selectionSet', {'entityIds': [first['id'], wrong['id']]}, 'invalid_constraint_target')
            after = state()
            assert after['constraintPickStatus'] == expected
            for key in ['selection', 'draftSketch', 'draftHistory', 'solverDiagnostics']:
                assert after[key] == before[key], key
            assert call('document.inspect') == document
            receipt.append({'family': family, 'status': expected, 'prompt': after['constraintPrompt']})

        call('sketch.tool', {'tool': 'select'})
        call('sketch.selectionSet')
        call('sketch.constraintTool', {'type': 'concentric'})
        call('sketch.selectionSet', {'entityIds': [circle['id']]})
        call('sketch.selectionSet', {'entityIds': [circle['id'], line['id']]}, 'invalid_constraint_target')
        call('sketch.selectionSet', {'entityIds': [circle['id'], arc['id']]})
        draft = state()['draftSketch']
        assert state()['constraintPickStatus'] == 'applied'
        relation = draft['parameters']['constraints'][-1]
        assert relation['type'] == 'concentric'
        call('sketch.tool', {'tool': 'select'})
        assert 'constraintPickStatus' not in state()
        call('sketch.finish')
        saved = output / 'recovered-concentric.alloy'
        call('document.save', {'path': str(saved)})
        authored = call('document.inspect')['authored']
        call('history.undo')
        call('history.redo')
        assert call('document.inspect')['authored'] == authored
        call('document.new', {'discard': True})
        call('document.open', {'path': str(saved)})
        assert call('document.inspect')['authored'] == authored
        call('sketch.edit', {'sketchId': draft['id']})
        assert state()['draftSketch'] == draft
        call('sketch.lookAt')
        call('capture.screenshot', {'path': str(output / 'api-reopened-concentric.png')})
        gpu = call('app.state')['viewport']['gpu']
        assert any(g['constraintId'] == relation['id'] for g in gpu['sketchConstraintGlyphs'])
        call('sketch.cancel')
        (output / 'receipt.json').write_text(json.dumps({'refusals': receipt, 'gpu': gpu,
            'relation': relation, 'undoRedoReopen': True, 'transcript': transcript}, indent=2))
    print(f'PASS: pair feedback, retained recovery and undo/reopen: {output}')


if __name__ == '__main__':
    main()
