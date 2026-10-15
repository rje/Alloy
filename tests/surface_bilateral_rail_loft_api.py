#!/usr/bin/env python3
"""Bounded solved Mirror pair to exact bilateral exterior and two side holes."""
import argparse
import copy
import json
from pathlib import Path
from app_harness import application

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--app', required=True); p.add_argument('--source', required=True)
p.add_argument('--output', required=True, type=Path)
a = p.parse_args(); out = a.output.resolve()
with application(a.app, out, command_timeout=120) as (call, ledger):
    call('window.resize', {'width': 1500, 'height': 950})
    call('document.open', {'path': str(Path(a.source).resolve()), 'discard': True})
    before = call('document.inspect')['authored']
    back = next(f for f in before['features'] if f['name'] == 'Back arc')
    lower = next(f for f in before['features'] if f['name'] == 'Lower Mirror pair')
    rail = next(f for f in before['features'] if f['name'] == 'Crown rail')
    pair = lower['parameters']['mirrorGroups'][0]['entityPairs'][0]
    inputs = {'operation': 'newComponent', 'guideType': 'rail', 'componentName': 'Iron exterior',
              'name': 'Bilateral exterior',
              'sections': [{'sketchId': back['id'], 'entityIds': [back['parameters']['entities'][0]['id']]},
                           {'sketchId': lower['id'], 'entityIds': [pair['resultEntityId'], pair['sourceEntityId']]}],
              'rail': {'sketchId': rail['id'], 'entityId': rail['parameters']['entities'][0]['id']}}
    call('view.home')
    session = call('surface.loftDialog', inputs)
    assert session['previewed'] and session['geometry']['interpolation']['mode'] == 'bilateralInteriorRail'
    call('capture.dialog', {'path': str(out / 'loft-controls.png')})
    call('capture.screenshot', {'path': str(out / 'loft-preview.png')})
    call('command.cancel', {'session': session['id']})
    assert call('document.inspect')['authored'] == before
    session = call('surface.loftDialog', inputs); call('command.commit', {'session': session['id']})
    inspected = call('document.inspect'); source = inspected['evaluation']['bodies'][0]
    assert source['valid'] and source['topology']['faces'] == 2 and source['topology']['edges'] == 7
    assert len(source['solidEdges']) == 5 and source['topology']['solids'] == 0
    call('view.home'); call('capture.screenshot', {'path': str(out / 'bilateral-exterior.png')})
    call('sketch.begin', {'plane': 'XZ', 'name': 'Handle lens'})
    for middle in [30, 10]:
        call('sketch.splineAdd', {'points': [{'uMm': 30, 'vMm': 15}, {'uMm': 60, 'vMm': middle}, {'uMm': 90, 'vMm': 15}]})
    tool = call('app.state')['activeSession']['draftSketch']; call('sketch.finish')
    tool_ids = [e['id'] for e in tool['parameters']['entities']]
    session = call('surface.trimDialog', {'targetBodyId': source['bodyId'], 'toolSketchId': tool['id'],
                                         'toolEntityIds': tool_ids, 'removedRegionIds': [], 'name': 'Bilateral handle openings'})
    regions = call('app.state')['activeSession']['regions']
    assert len(regions) == 4 and len([r for r in regions if r['inside']]) == 2
    for region in regions:
        if region['inside']: call('surface.trimRegion', {'regionId': region['regionId']})
    assert call('app.state')['activeSession']['previewed']
    call('capture.screenshot', {'path': str(out / 'trim-preview.png')})
    call('command.commit', {'session': session['id']})
    call('view.home'); call('capture.screenshot', {'path': str(out / 'bilateral-holes.png')})
    stable = call('document.inspect'); body = stable['evaluation']['bodies'][0]
    assert body['valid'] and body['topology']['faces'] == 2 and body['topology']['solids'] == 0
    assert len([e for e in body['solidEdges'] if e['provenance'].get('role') == 'trimBoundary']) == 4
    face_ids = [f['faceId'] for f in body['supportFaces']]
    edge_ids = [e['edgeId'] for e in body['solidEdges']]
    changed = copy.deepcopy(rail['parameters']); middle = changed['entities'][0]['pointIds'][1]
    for point in changed['points']:
        if point['id'] == middle: point['vMm'] = 55
    call('feature.edit', {'featureId': rail['id'], 'parameters': changed})
    edited = call('document.inspect'); edited_body = edited['evaluation']['bodies'][0]
    assert [f['faceId'] for f in edited_body['supportFaces']] == face_ids
    assert [e['edgeId'] for e in edited_body['solidEdges']] == edge_ids
    assert abs(edited_body['areaMm2'] - body['areaMm2']) > 100
    call('capture.screenshot', {'path': str(out / 'edited-crown-holes.png')})
    call('history.undo'); assert call('document.inspect')['authored'] == stable['authored']
    call('history.redo'); assert call('document.inspect')['authored'] == edited['authored']
    model = out / 'bilateral-iron-exterior.alloy'; call('document.save', {'path': str(model)})
    call('document.new', {'discard': True}); call('document.open', {'path': str(model)})
    reopened = call('document.inspect'); assert reopened['authored'] == edited['authored']
    call('view.home'); call('capture.screenshot', {'path': str(out / 'reopened-holes.png')})
    state = call('app.state')
    (out / 'result.json').write_text(json.dumps({'passed': True, 'calls': len(ledger), 'source': a.source,
        'gpu': state['viewport']['gpu'], 'originalExterior': source, 'trimmed': body, 'edited': edited_body,
        'scope': 'Declared independent prepared sketches; exact bilateral retained Loft, paired-sketch Trim, crown edit/history/reopen. Closed-hole topology separately checked by core.'}, indent=2))
    (out / 'transcript.json').write_text(json.dumps(ledger, indent=2))
print('PASS retained bilateral rail Loft and side holes')
