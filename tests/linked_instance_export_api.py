#!/usr/bin/env python3
"""Real-app linked instance export, independent STL readback and STEP reimport."""
import argparse
import json
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl

p = argparse.ArgumentParser()
p.add_argument('--app', required=True)
p.add_argument('--fixture', required=True)
p.add_argument('--coupled-fixture', help='Optional committed seven-joint pose for staged refusal and posed STEP readback')
p.add_argument('--skip-dialog-check', action='store_true', help='Keep a shared recording session free of modal windows')
p.add_argument('--output', required=True)
args = p.parse_args()
out = Path(args.output).resolve()
out.mkdir(parents=True, exist_ok=True)


def bounds_key(bounds):
    return tuple(bounds['min']) + tuple(bounds['max'])


def compare_bounds(actual, expected, tolerance):
    actual, expected = sorted(actual), sorted(expected)
    assert len(actual) == len(expected), (len(actual), len(expected))
    for a, b in zip(actual, expected):
        assert len(a) == len(b) == 6
        assert all(abs(x-y) <= tolerance for x, y in zip(a, b)), (a, b)


with application(args.app, out, command_timeout=90) as (call, ledger):
    call('document.open', {'path': str(Path(args.fixture).resolve()), 'discard': True})
    original = call('document.inspect')['authored']
    instances = call('component.inspect')['localInstances']
    parent_definition = next(c['id'] for c in original['components'] if c['name'] == 'Piston subassembly')
    parent = next(r['occurrenceId'] for r in instances
                  if r['definitionId'] == parent_definition and abs(r['worldPose'][3]+180) < 1e-5)
    all_bodies = [b for r in instances for b in r['bodies']]
    copied_bodies = [b for r in instances if r.get('parentOccurrenceId') == parent for b in r['bodies']]
    assert len(all_bodies) == 6 and len(copied_bodies) == 2
    reports = []
    for label, scope, bodies in (
            ('whole-assembly', {'allVisible': True}, all_bodies),
            ('copied-piston', {'occurrenceId': parent}, copied_bodies)):
        expected = [bounds_key(b['boundsMm']) for b in bodies]
        stl = out/f'{label}.stl'
        result = call('document.exportStl', {**scope, 'path': str(stl), 'units': 'mm',
            'format': 'binary', 'surfaceDeviationMm': .1, 'normalDeviationDeg': 10})
        assert result['bodyCount'] == len(bodies) and not result['authoredChanged']
        assert len({(r['occurrenceId'], r['bodyId']) for r in result['instances']}) == len(bodies)
        mesh = inspect_stl(stl, True)
        compare_bounds([tuple(c['bounds'][0])+tuple(c['bounds'][1]) for c in mesh['components']], expected, .11)
        step = out/f'{label}.step'
        exchanged = call('document.exportStep', {**scope, 'path': str(step), 'units': 'mm'})
        assert exchanged['bodyCount'] == len(bodies) and not exchanged['authoredChanged']
        compare_bounds([bounds_key(r['boundsMm']) for r in exchanged['instances']], expected, 1e-5)
        assert call('document.inspect')['authored'] == original
        reports.append({'scope': label, 'expectedBounds': expected, 'stl': result, 'mesh': mesh,
                        'step': exchanged, 'stepPath': str(step)})
    for report in reports:
        call('document.new', {'discard': True})
        call('document.importStep', {'path': report['stepPath']})
        imported = call('document.inspect')['evaluation']['bodies']
        assert all(b['valid'] for b in imported)
        compare_bounds([bounds_key(b['boundsMm']) for b in imported], report['expectedBounds'], 1e-5)
        call('view.home')
        call('capture.screenshot', {'path': str(out/f"{report['scope']}-step-reimport.png")})
        report['reimportedBodies'] = imported
    if args.coupled_fixture:
        call('document.open', {'path': str(Path(args.coupled_fixture).resolve()), 'discard': True})
        committed = call('document.inspect')['authored']
        driver = next(j['id'] for j in call('joint.inspect')['joints'] if j['type'] == 'asBuiltRevolute')
        call('joint.previewAngle', {'jointId': driver, 'angleDeg': -15})
        posed_step = out/'committed-pose.step'
        call('document.exportStep', {'path': str(posed_step), 'units': 'mm', 'allVisible': True}, error='staged_pose')
        assert not posed_step.exists()
        call('position.revert')
        exported = call('document.exportStep', {'path': str(posed_step), 'units': 'mm', 'allVisible': True})
        assert exported['bodyCount'] == 6
        assert call('document.inspect')['authored'] == committed
        call('document.new', {'discard': True})
        call('document.importStep', {'path': str(posed_step)})
        placed = call('document.inspect')['evaluation']['bodies']
        compare_bounds([bounds_key(b['boundsMm']) for b in placed],
                       [bounds_key(b['boundsMm']) for b in exported['instances']], 1e-5)
        call('view.home')
        call('capture.screenshot', {'path': str(out/'committed-pose-step-reimport.png')})
        reports.append({'scope': 'committed seven-joint pose; staged pose refused', 'step': exported,
                        'reimportedBodies': placed})
    if not args.skip_dialog_check:
        call('document.open', {'path': str(Path(args.fixture).resolve()), 'discard': True})
        copied_leaf = next(r for r in instances if r.get('parentOccurrenceId') == parent and r['bodies'])
        selected_body = copied_leaf['bodies'][0]['bodyId']
        call('selection.set', {'bodyId': selected_body, 'occurrenceId': copied_leaf['occurrenceId']})
        dialog = call('document.exportStepDialog')
        assert dialog['bodyId'] == selected_body and dialog['occurrenceId'] == copied_leaf['occurrenceId']
        call('capture.dialog', {'path': str(out/'copied-body-step-dialog.png')})
    (out/'receipt.json').write_text(json.dumps({'passed': True, 'reports': reports, 'ledger': ledger}, indent=2)+'\n')
    print('PASS whole and copied-parent exports, independent closed STL components and placed STEP reimport', flush=True)
