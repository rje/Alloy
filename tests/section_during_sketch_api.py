#!/usr/bin/env python3
"""Retained section changes during an active sketch use the same document authority."""
import argparse
import json
from pathlib import Path
from app_harness import application

p = argparse.ArgumentParser()
p.add_argument('--app', required=True)
p.add_argument('--output', required=True)
a = p.parse_args()
out = Path(a.output).resolve()
out.mkdir(parents=True, exist_ok=True)

def ref(face):
    return {key: face[key] for key in ('producerFeatureId', 'bodyId', 'faceId')}

with application(a.app, out) as (call, ledger):
    call('solid.box', {'width': 40, 'depth': 20, 'height': 20, 'units': 'mm'})
    body = call('document.inspect')['evaluation']['bodies'][0]
    face = next(f for f in body['supportFaces'] if f['role'] == 'box.y1')
    plane = call('construction.plane', {'source': {'support': ref(face)},
        'offset': 0, 'units': 'mm'})['planeId']
    width_dialog = call('inspect.sectionDialog', {'plane': 'XZ', 'name': 'Width'})
    call('command.commit', {'session': width_dialog['id']})
    width = call('analysis.inspect')['analyses'][-1]['id']
    top = call('inspect.section', {'plane': 'XY', 'name': 'Top'})['analyses'][-1]['id']
    feature_count = len(call('document.inspect')['authored']['features'])

    call('sketch.begin', {'planeId': plane, 'name': 'Section-aware Project'})
    started = call('app.state')['activeSession']
    call('analysis.visibility', {'analysisId': top, 'visible': False})
    call('inspect.section', {'analysisId': width, 'offsetMm': 32})
    call('analysis.visibility', {'analysisId': width, 'visible': True})
    active = call('app.state')['activeSession']
    assert active['id'] == started['id']
    assert active['draftSketch'] == started['draftSketch']
    assert active['baseDocumentRevision'] > started['baseDocumentRevision']
    assert len(call('document.inspect')['authored']['features']) == feature_count
    call('sketch.projectBegin')
    pending = call('app.state')['activeSession']
    history_before = call('document.inspect')['undo']['count']
    nested = call('inspect.sectionDialog', {'analysisId': width})
    assert call('app.state')['activeSession'] == pending
    assert call('app.state')['sectionEdit']['id'] == nested['id']
    call('command.update', {'session': nested['id'], 'inputs': {'offsetMm': 30}})
    assert call('app.state')['sectionEdit']['previewed']
    call('sketch.finish', error='section_edit_active')
    call('history.undo', error='section_edit_active')
    call('command.cancel', {'session': nested['id']})
    assert call('document.inspect')['undo']['count'] == history_before
    assert call('app.state')['activeSession'] == pending
    assert next(row for row in call('analysis.inspect')['analyses'] if row['id'] == width)['offsetMm'] == 32
    nested = call('inspect.sectionDialog', {'analysisId': width})
    call('command.update', {'session': nested['id'], 'inputs': {'offsetMm': 33, 'hatch': False}})
    call('command.commit', {'session': nested['id']})
    assert call('document.inspect')['undo']['count'] == history_before + 1
    assert call('app.state')['sectionEdit'] == {}
    assert call('app.state')['activeSession']['pendingProject'] == pending['pendingProject']
    assert call('app.state')['activeSession']['draftSketch'] == pending['draftSketch']
    assert next(row for row in call('analysis.inspect')['analyses'] if row['id'] == width)['offsetMm'] == 33
    call('analysis.visibility', {'analysisId': width, 'visible': False})
    still_pending = call('app.state')['activeSession']
    assert still_pending['pendingProject'] == pending['pendingProject']
    assert still_pending['draftSketch'] == pending['draftSketch']
    assert still_pending['id'] == pending['id']
    assert call('sketch.projectFace', ref(face))['added'] == 4
    call('sketch.projectCommit')
    draft = call('app.state')['activeSession']['draftSketch']
    assert len(draft['parameters']['projections']) == 4
    call('sketch.tool', {'tool': 'offset'})
    source_ids = [row['id'] for row in draft['parameters']['projections']]
    offset = call('sketch.offsetPreview', {'sourceEntityIds': source_ids,
        'distance': 1, 'units': 'mm'})
    assert offset['pendingClosedRectOffset']['valid']
    call('sketch.offsetAccept')
    assert len(call('app.state')['activeSession']['draftSketch']['parameters']['closedRectOffsets']) == 1
    call('sketch.finish')
    finished = call('document.inspect')
    assert len(finished['authored']['features']) == feature_count + 1
    assert next(row for row in finished['authored']['analyses'] if row['id'] == width)['offsetMm'] == 33

    call('sketch.begin', {'planeId': plane, 'name': 'Canceled after section edit'})
    before_invalid = call('app.state')['activeSession']['draftSketch']
    prior_analyses = call('document.inspect')['authored']['analyses']
    call('inspect.section', {'analysisId': width, 'offsetMm': 1000000}, error='invalid_arguments')
    assert call('app.state')['activeSession']['draftSketch'] == before_invalid
    assert call('document.inspect')['authored']['analyses'] == prior_analyses
    nested = call('inspect.sectionDialog', {'analysisId': width})
    call('command.update', {'session': nested['id'], 'inputs': {'offsetMm': 31}})
    call('command.commit', {'session': nested['id']})
    call('sketch.cancel')
    canceled = call('document.inspect')
    assert len(canceled['authored']['features']) == feature_count + 1
    assert next(row for row in canceled['authored']['analyses'] if row['id'] == width)['offsetMm'] == 31
    (out / 'receipt.json').write_text(json.dumps({'calls': len(ledger),
        'sketchId': draft['id'], 'widthAnalysisId': width,
        'pendingProjectPreserved': True, 'finishAndCancelPreserveAnalyses': True}, indent=2) + '\n')
    (out / 'ledger.json').write_text(json.dumps(ledger, indent=2) + '\n')
