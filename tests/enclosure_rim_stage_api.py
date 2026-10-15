#!/usr/bin/env python3
"""Record the rim/fit continuation from the accepted 29-feature enclosure."""
import argparse
import json
import math
from pathlib import Path
import time
from app_harness import application

p = argparse.ArgumentParser()
for key in ('app', 'source', 'output'):
    p.add_argument('--' + key, required=True)
a = p.parse_args()
out = Path(a.output).resolve()
assert not out.is_relative_to(Path(__file__).resolve().parents[1])
with application(a.app, out, command_timeout=180) as (raw, ledger):
    recording = False
    stages = []
    def call(command, inputs=None):
        result = raw(command, inputs)
        if recording:
            time.sleep(.28)
        return result
    def stage(text, capture=None):
        stages.append(text)
        raw('recording.step', {'text': 'API | ' + text})
        time.sleep(1.2)
        if capture:
            raw('capture.screenshot', {'path': str(out / capture)})
    def inspect():
        return call('document.inspect')
    def body(state, identity):
        value = next(b for b in state['evaluation']['bodies'] if b['bodyId'] == identity)
        assert value['valid'] and value['closed'] and value['topology']['solids'] == 1
        return value
    try:
        call('document.open', {'path': str(Path(a.source).resolve())})
        call('window.resize', {'width': 1400, 'height': 1000})
        initial = inspect()
        assert len(initial['authored']['features']) == 29
        assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
        split = next(f for f in initial['authored']['features'] if f['type'] == 'splitBody')
        bottom_id, top_id = split['parameters']['targetBodyId'], split['bodyId']
        bottom = body(initial, bottom_id)
        support = next(f for f in bottom['supportFaces'] if f['role'] == 'split.negative.cap')
        call('component.activate', {'definitionId': split['ownerId']})
        call('view.grid', {'visible': False})
        call('view.origin', {'visible': False})
        call('view.home')
        raw('recording.start', {'path': str(out/'enclosure-rim-fit-stage.mp4'), 'fps': 5})
        recording = True
        stage('Rim and fit continuation — saved 29-feature enclosure', 'stage-start.png')
        for analysis in initial['authored'].get('analyses', []):
            if analysis.get('visible'):
                call('analysis.visibility', {'analysisId': analysis['id'], 'visible': False})
        board = next(f for f in initial['authored']['features'] if f['type'] == 'stepAssemblyImport')
        call('feature.visibility', {'featureId': board['id'], 'visible': False})
        call('feature.visibility', {'featureId': split['id'], 'visible': False})
        call('view.orientation', {'name': 'top'})
        stage('Hide Board and Top; use the actual U-shaped split cap')
        sketch = call('sketch.begin', {'support': {k: support[k] for k in
            ('producerFeatureId', 'bodyId', 'faceId')}, 'name': 'Rim inset'})['draftSketch']['id']
        call('sketch.tool', {'tool': 'offset'})
        call('sketch.loopOffsetPreview', {'kind': 'planarInsetV1', 'distance': -1.25,
                                         'chain': True, 'units': 'mm'})
        stage('Adapted single U boundary: inward offset 1.25 mm', 'inset-preview.png')
        call('sketch.loopOffsetAccept')
        call('sketch.finish')
        offset = inspect()
        profile = next(s for s in offset['evaluation']['sketches'] if s['sketchId'] == sketch)['profiles'][0]['profileId']
        session = call('solid.extrudeDialog', {'sketchId': sketch, 'profileId': profile,
            'distance': 2, 'units': 'mm', 'operation': 'join', 'targetBodyIds': [bottom_id]})
        call('command.preview', {'session': session['id']})
        call('view.orientation', {'name': 'rearIsometric'})
        stage('Join the inset profile by 2 mm', 'join-preview.png')
        call('command.commit', {'session': session['id']})
        joined = inspect()
        rails = [e for e in body(joined, bottom_id)['solidEdges'] if e['available']
                 and e['provenance'].get('sourceSketchId') == sketch
                 and e['provenance'].get('role') == 'endRim'
                 and math.isclose(math.dist(e['startMm'], e['endMm']), 54, abs_tol=1e-6)
                 and any(math.isclose(e['startMm'][1], y, abs_tol=1e-6) for y in (-45.25, -44.25))]
        assert len(rails) == 2
        seeds = [e['edgeId'] for e in rails]
        session = call('solid.filletDialog', {'targetBodyId': bottom_id, 'edgeIds': seeds,
            'radiusMm': .5, 'tangentChain': True, 'name': 'Paired rim round'})
        call('command.preview', {'session': session['id']})
        stage('Two tangent chains selected, radius 0.5 mm', 'rim-selection-preview.png')
        call('capture.dialog', {'path': str(out/'rim-selection-panel.png')})
        call('command.commit', {'session': session['id']})
        rounded = inspect()
        rim = body(rounded, bottom_id)
        assert rim['construction'] == 'pairedFullRoundCommon'
        assert rim['selectedEdgeIds'] == seeds
        stage('Rounded U ridge and its mouth transitions', 'rounded-rim.png')
        call('feature.visibility', {'featureId': split['id'], 'visible': True})
        call('view.home')
        session = call('solid.combineDialog', {'targetBodyId': top_id,
            'toolBodyIds': [bottom_id], 'operation': 'cut', 'keepTools': True,
            'name': 'Ridge mating groove'})
        call('command.preview', {'session': session['id']})
        stage('Cut: Top target, Bottom tool, Keep Tools checked', 'cut-tool-selection.png')
        call('capture.dialog', {'path': str(out/'cut-tool-panel.png')})
        call('command.commit', {'session': session['id']})
        cut = inspect()
        assert cut['authored']['features'][-1]['parameters']['keepTools']
        assert math.isclose(body(cut, bottom_id)['volumeMm3'], rim['volumeMm3'], abs_tol=1e-6)
        assert body(cut, top_id)['volumeMm3'] < body(initial, top_id)['volumeMm3']
        call('feature.visibility', {'featureId': body(cut, bottom_id)['producerFeatureId'], 'visible': False})
        call('view.orientation', {'name': 'bottom'})
        stage('Top underside: complementary U groove', 'top-groove.png')
        call('feature.visibility', {'featureId': body(cut, bottom_id)['producerFeatureId'], 'visible': True})
        call('feature.visibility', {'featureId': split['id'], 'visible': False})
        call('view.orientation', {'name': 'rearIsometric'})
        stage('Bottom retained independently after the Cut', 'bottom-retained.png')
        exports = []
        for name, identity in (('Bottom', bottom_id), ('Top', top_id)):
            stage('Export ' + name + ' separately as STL and STEP')
            stl = call('document.exportStl', {'path': str(out/(name.lower()+'.stl')),
                'bodyId': identity, 'format': 'binary', 'units': 'mm',
                'surfaceDeviationMm': .1, 'normalDeviationDeg': 10})
            step = call('document.exportStep', {'path': str(out/(name.lower()+'.step')),
                'bodyId': identity, 'units': 'mm'})
            exports.append({'name': name, 'stl': stl, 'step': step})
        call('feature.visibility', {'featureId': split['id'], 'visible': True})
        call('view.home')
        call('document.save', {'path': str(out/'enclosure-rim-fit.alloy')})
        stage('Rim and retained-tool fit stage complete — two separate exports', 'stage-finish.png')
        video = raw('recording.stop')
        recording = False
        (out/'result.json').write_text(json.dumps({'passed': True, 'stages': stages,
            'source': str(Path(a.source).resolve()), 'sourceFeatureCount': 29,
            'scope': 'API stage continuation; earlier creation footage separate',
            'cut': cut, 'exports': exports, 'video': video,
            'gpu': call('app.state')['viewport']['gpu']['details']}, indent=2))
    finally:
        if recording:
            raw('recording.stop')
        (out/'transcript.json').write_text(json.dumps(ledger, indent=2))
