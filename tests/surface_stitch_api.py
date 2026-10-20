#!/usr/bin/env python3
"""SRF-056 bounded real-entrypoint Stitch geometry/history/GPU fixture."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application


def near(a, b, tolerance=1e-7):
    assert math.isclose(a, b, rel_tol=0, abs_tol=tolerance), (a, b)


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
            call('window.resize', {'width': 1280, 'height': 900})
            environment = call('app.state')
            assert environment['viewport']['gpu']['hardwareAccelerated'], environment['viewport']
            assert 'surface.stitch' in {c['id'] for c in call('commands.list')['commands']}
            results = []

            def unchanged(before):
                after = inspect()
                for key in ('authored', 'revision', 'undo', 'evaluation'):
                    assert after[key] == before[key], key

            def verify(state, gap, area=400):
                body, = state['evaluation']['bodies']
                assert body['valid'] and not body['closed'] and not body['isSolid'], body
                assert body['shellClassification'] == 'open'
                assert body['topology'] == {'faces': 2, 'edges': 7, 'vertices': 6, 'shells': 1, 'solids': 0}, body['topology']
                assert body['sewnEdgeCount'] == 1 and body['freeEdgeCount'] == 6, body
                near(body['measuredSeamGapMm'], gap)
                near(body['areaMm2'], area)
                assert body['maxInputToleranceMm'] <= body['requestedToleranceMm']
                assert body['maxOutputToleranceMm'] <= body['requestedToleranceMm'] + 1e-12
                assert len(body['supportFaces']) == 2 and len(body['solidEdges']) == 7
                return body

            for gap in (0, 0.005, 0.02):
                call('document.new', {'discard': True})
                for y in (0, 10 + gap):
                    call('sketch.create', {'plane': 'XY', 'name': 'Alloy rectangular Stitch source'})
                    call('sketch.rectangle', {'x1': 0, 'y1': y, 'x2': 20, 'y2': y + 10, 'units': 'mm'})
                    sketch = call('app.state')['activeSession']['draftSketch']
                    sketch_id = sketch['id']
                    edges = sketch['parameters']['entities']
                    call('sketch.constraint', {'type': 'fixed', 'pointIds': [edges[0]['startPointId']]})
                    call('sketch.dimension', {'kind': 'horizontal', 'entityId': edges[0]['id'], 'value': 20, 'units': 'mm'})
                    call('sketch.dimension', {'kind': 'vertical', 'entityId': edges[1]['id'], 'value': 10, 'units': 'mm'})
                    source_height_dimension = call('app.state')['activeSession']['draftSketch']['parameters']['dimensions'][-1]['id']
                    call('sketch.finish')
                    source = inspect()['evaluation']['sketches'][-1]
                    call('surface.patch', {'sketchId': sketch_id, 'profileId': source['profiles'][0]['profileId']})
                before = inspect()
                refs = [{key: b['supportFaces'][0][key] for key in ('bodyId', 'producerFeatureId', 'faceId')}
                        for b in before['evaluation']['bodies']]
                inputs = {'sourceFaces': refs, 'toleranceMm': 0.01, 'name': 'Alloy two-sheet Stitch'}
                call('document.save', {'path': str(out / f'sources-gap-{gap}.alloy')})
                before = inspect()
                if gap > 0.01:
                    call('surface.stitch', inputs, error='evaluation_failed')
                    diagnostics = transcript[-1]['response']['error']['diagnostics']
                    assert diagnostics['code'] == 'stitch_gap_exceeded', diagnostics
                    near(diagnostics['measuredSeamGapMm'], gap)
                    unchanged(before)
                    results.append({'gapMm': gap, 'refusal': diagnostics})
                    continue
                empty = call('surface.stitchDialog')
                assert not empty['previewed']
                call('command.cancel', {'session': empty['id']})
                unchanged(before)
                preview = call('surface.stitchDialog', inputs)
                assert preview['previewed'] and preview['geometryEvaluated'], preview
                unchanged(before)
                call('view.home')
                call('capture.screenshot', {'path': str(out / f'preview-gap-{gap}.png')})
                preview_environment = call('app.state')
                assert preview_environment['viewport']['gpu']['previewReplacedBodyId'] == preview['bodyId']
                assert preview_environment['viewport']['gpu']['bodyStageKey'].startswith('surface-stitch-preview:')
                call('command.cancel', {'session': preview['id']})
                unchanged(before)
                preview = call('surface.stitchDialog', inputs)
                call('command.commit', {'session': preview['id']})
                committed = inspect()
                result = verify(committed, gap)
                feature_id, body_id = result['featureId'], result['bodyId']
                assert result['sourceFaces'] == refs
                call('history.undo')
                assert inspect()['authored'] == before['authored']
                assert len(inspect()['evaluation']['bodies']) == 2
                call('history.redo')
                assert verify(inspect(), gap)['bodyId'] == body_id
                edit = call('surface.stitchDialog', {'featureId': feature_id})
                call('command.update', {'session': edit['id'], 'inputs': {'toleranceMm': 0.009}})
                call('command.commit', {'session': edit['id']})
                edited = verify(inspect(), gap)
                assert edited['bodyId'] == body_id
                assert [f['faceId'] for f in edited['supportFaces']] == [f['faceId'] for f in result['supportFaces']]
                before_source_edit = inspect()
                call('sketch.edit', {'sketchId': sketch_id})
                call('sketch.dimensionUpdate', {'dimensionId': source_height_dimension, 'value': 11, 'units': 'mm'})
                call('sketch.finish')
                source_edited = verify(inspect(), gap, 420)
                assert source_edited['bodyId'] == body_id
                assert [f['faceId'] for f in source_edited['supportFaces']] == [f['faceId'] for f in result['supportFaces']]
                call('history.undo')
                assert inspect()['authored'] == before_source_edit['authored']
                verify(inspect(), gap)
                call('history.redo')
                verify(inspect(), gap, 420)
                model = out / f'stitch-gap-{gap}.alloy'
                call('document.save', {'path': str(model)})
                saved = inspect()
                call('document.new', {'discard': True})
                call('document.open', {'path': str(model), 'discard': True})
                reopened = inspect()
                assert reopened['authored'] == saved['authored']
                assert verify(reopened, gap, 420)['bodyId'] == body_id
                call('view.home')
                call('capture.screenshot', {'path': str(out / f'reopened-gap-{gap}.png')})
                results.append({'gapMm': gap, 'result': result, 'reopened': reopened, 'previewEnvironment': preview_environment, 'captureEnvironment': call('app.state')})
            (out / 'result.json').write_text(json.dumps({'status': 'passed', 'environment': environment,
                'results': results, 'scope': 'Shared command/API, preview/cancel/commit, retained edit/source recompute, Undo/Redo, save/reopen, geometry inspection and GPU captures. Native actual-hit input and matched visual parity remain SRF-057.'}, indent=2) + '\n')
        finally:
            (out / 'transcript.json').write_text(json.dumps(transcript, indent=2) + '\n')


if __name__ == '__main__':
    main()
