#!/usr/bin/env python3
"""SRF-034 independent transient planar/cylinder zebra API and GPU fixture."""
import argparse
import copy
import json
import math
import sys
from pathlib import Path
from app_harness import application


def reference(face):
    return {k: face[k] for k in ('producerFeatureId', 'bodyId', 'faceId')}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    out = Path(args.output).resolve()
    out.mkdir(parents=True, exist_ok=True)
    with application(args.app, out, command_timeout=90) as (call, transcript):
        try:
            inspect = lambda: call('document.inspect')
            result = lambda: call('inspect.zebraResult')
            captures = {}
            checkpoints = {}
            def unchanged(before):
                after = inspect()
                for key in ('authored', 'revision', 'undo', 'evaluation', 'dirty'):
                    assert after[key] == before[key], key
            def capture(name):
                captures[name] = call('capture.screenshot', {'path': str(out/(name+'.png'))})
                assert captures[name]['source'] == 'QWidget chrome + OCCT OpenGL framebuffer'
                assert captures[name]['viewportPixelWidth'] > 0
                captures[name]['viewportState'] = call('app.state')['viewport']
                return captures[name]['viewportSha256']
            settings = {'direction': 'vertical', 'angularRepeats': 12, 'stripeBlend': .9,
                        'materialAlphaPreserved': True, 'antiAliasing': 'fragment_derivative_smoothstep'}
            def ready(source, kind):
                state = result()
                assert state['enabled'] and state['visible'] and state['status'] == 'ready', state
                actual_source = {k: v for k, v in state['source'].items() if v is not None}
                assert actual_source == source and state['surfaceType'] == kind, state
                assert state['scope'] == 'single_current_planar_or_cylindrical_face', state
                assert state['persistence'] == 'session_only' and not state['authoredChange'], state
                assert not state['numericContinuityResult'] and state['settings'] == settings, state
                assert state['method'] == 'view_fixed_angular_environment_reflection_from_interpolated_tessellation_normals', state
                return state
            call('window.resize', {'width': 1280, 'height': 900})
            assert {'inspect.zebra', 'inspect.zebraResult'} <= {c['id'] for c in call('commands.list')['commands']}
            assert not result()['enabled'] and not result()['visible']
            call('sketch.create', {'plane': 'XY', 'name': 'Alloy planar zebra boundary'})
            call('sketch.rectangle', {'x1': 30, 'y1': 0, 'x2': 70, 'y2': 30, 'units': 'mm'})
            sid = call('app.state')['activeSession']['draftSketch']['id']
            call('sketch.finish')
            profile, = inspect()['evaluation']['sketches'][0]['profiles']
            call('surface.patch', {'sketchId': sid, 'profileId': profile['profileId']})
            planar = reference(inspect()['evaluation']['bodies'][0]['supportFaces'][0])
            call('sketch.create', {'plane': 'XZ', 'name': 'Alloy R10 H30 cylinder section'})
            call('sketch.line', {'x1': 10, 'y1': 0, 'x2': 10, 'y2': 30, 'units': 'mm'})
            call('sketch.line', {'x1': 0, 'y1': 0, 'x2': 0, 'y2': 30, 'units': 'mm'})
            draft = call('app.state')['activeSession']['draftSketch']
            section, axis = draft['parameters']['entities']
            call('sketch.construction', {'entityIds': [axis['id']], 'construction': True})
            call('sketch.finish')
            call('surface.revolve', {'sketchId': draft['id'], 'entityId': section['id'],
                                    'axisEntityId': axis['id'], 'angleDeg': 120})
            initial = inspect()
            curved = reference(initial['evaluation']['bodies'][-1]['supportFaces'][0])
            source = initial['authored']['features'][-1]
            for body, area in zip(initial['evaluation']['bodies'], [1200, 10*math.radians(120)*30]):
                assert body['valid'] and body['topology']['faces'] == 1 and body['topology']['solids'] == 0
                assert math.isclose(body['areaMm2'], area, rel_tol=1e-9)
            model = out/'zebra.alloy'
            call('document.save', {'path': str(model)})
            baseline = inspect()
            checkpoints['baseline'] = baseline
            call('view.home')
            call('selection.faceSet', {'support': planar})
            off = capture('plane-off-before')
            call('inspect.zebra', {'enabled': True})
            plane_state = ready(planar, 'plane')
            assert capture('plane-on') != off, 'Planar shader has no visible effect'
            call('inspect.zebra')  # Exercise the same no-argument toggle as the UI.
            assert not result()['enabled'] and not result()['visible']
            assert capture('plane-off-after') == off, 'Ordinary planar material not exactly restored'
            unchanged(baseline)
            call('selection.faceSet', {'support': curved})
            call('view.fitSelection')
            off = capture('cylinder-off-before')
            call('inspect.zebra')
            curved_state = ready(curved, 'cylinder')
            assert capture('cylinder-on') != off, 'Cylinder shader has no visible effect'
            call('inspect.zebra')
            assert capture('cylinder-off-after') == off, 'Ordinary curved material not exactly restored'
            call('inspect.zebra')
            call('selection.faceSet', {'support': planar})
            ready(planar, 'plane')
            capture('switched-plane')
            call('selection.set', {'bodyId': ''})
            assert result()['status'] == 'no_selection' and not result()['visible']
            call('selection.faceSet', {'support': curved})
            ready(curved, 'cylinder')
            call('view.orbit', {'deltaX': 18, 'deltaY': 12})
            call('window.resize', {'width': 1100, 'height': 760})
            ready(curved, 'cylinder')
            capture('cylinder-resized-orbit')
            environment = call('app.state')
            gpu = environment['viewport']['gpu']
            assert gpu['hardwareAccelerated'] and gpu['vendor'] and gpu['version'], gpu
            dpr = captures['cylinder-resized-orbit']['devicePixelRatio']
            assert gpu['nativeWidth']*dpr == gpu['framebufferWidth'] and gpu['nativeHeight']*dpr == gpu['framebufferHeight'], gpu
            if sys.platform == 'darwin':
                assert 'apple' in json.dumps(gpu).lower(), gpu
            # Large deterministic orbit exposes the sheet from its other side;
            # visual review records which side is actually shown.
            call('view.orbit', {'deltaX': 600, 'deltaY': 0})
            capture('cylinder-opposite-view')
            unchanged(baseline)
            call('feature.visibility', {'featureId': source['id'], 'visible': False})
            assert not result()['visible']
            hidden = inspect()
            call('inspect.zebra', {'enabled': False})
            call('inspect.zebra', {'enabled': True})
            unchanged(hidden)
            call('history.undo')
            assert inspect()['authored'] == baseline['authored']
            call('selection.faceSet', {'support': curved})
            ready(curved, 'cylinder')
            parameters = copy.deepcopy(source['parameters'])
            parameters['angleDeg'] = 90
            call('feature.edit', {'featureId': source['id'], 'parameters': parameters})
            edited = inspect()
            ready(curved, 'cylinder')
            assert math.isclose(edited['evaluation']['bodies'][-1]['areaMm2'], 10*math.pi/2*30, rel_tol=1e-9)
            call('view.home')
            call('view.fitSelection')
            capture('edited-cylinder-on')
            call('inspect.zebra', {'enabled': False})
            capture('edited-cylinder-off')
            call('inspect.zebra', {'enabled': True})
            unchanged(edited)
            checkpoints['edited'] = edited
            call('document.save', {'path': str(out/'zebra-edited.alloy')})
            call('history.undo')
            assert inspect()['authored'] == baseline['authored']
            ready(curved, 'cylinder')
            call('history.redo')
            assert inspect()['authored'] == edited['authored']
            ready(curved, 'cylinder')
            call('history.undo')
            call('document.save', {'path': str(out/'zebra-after.alloy')})
            assert model.read_bytes() == (out/'zebra-after.alloy').read_bytes()
            call('document.new', {'discard': True})
            assert not result()['enabled'] and not result()['visible']
            call('inspect.zebra', {'enabled': True})
            call('document.open', {'path': str(model), 'discard': True})
            assert not result()['enabled'] and not result()['visible']
            reopened = inspect()
            assert reopened['authored'] == baseline['authored'] and not reopened['dirty']
            call('selection.faceSet', {'support': curved})
            call('inspect.zebra', {'enabled': True})
            ready(curved, 'cylinder')
            call('window.resize', {'width': 1280, 'height': 900})
            call('view.home')
            capture('reopened-cylinder-on')
            report = {'passed': True, 'planar': plane_state, 'cylinder': curved_state,
                      'environment': environment, 'finalEnvironment': call('app.state'),
                      'captures': captures, 'checkpoints': checkpoints, 'reopened': reopened,
                      'scope': 'Shared API, exact inspection invariants and saved bytes, one planar and one separate partial cylinder, current-target refresh, toggle/material restoration, orbit/resize, hide/show, edit/undo/redo, new/open reset and GPU framebuffer. Independent BRep readback and visual review separate. No joined seam, physical input, public pixel parity or numeric continuity claim.'}
            (out/'result.json').write_text(json.dumps(report, indent=2)+'\n')
        finally:
            (out/'transcript.json').write_text(json.dumps(transcript, indent=2)+'\n')


if __name__ == '__main__':
    main()
