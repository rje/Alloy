#!/usr/bin/env python3
"""Bounded point/retained-edge redundant Driven offer; native input is separate."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--app', required=True)
parser.add_argument('--output', required=True)
args = parser.parse_args()
out = Path(args.output).resolve()
with application(args.app, out) as (call, transcript):
    def session(): return call('app.state')['activeSession']
    def draft(): return session()['draftSketch']
    def inspect(): return call('document.inspect')
    def measured(dimension_id):
        return next(v['valueMm'] for v in session()['solverDiagnostics']['dimensionValues'] if v['id'] == dimension_id)

    call('sketch.begin', {'plane': 'XY'})
    call('sketch.rectangle', {'x1': 0, 'y1': 0, 'x2': 31.8, 'y2': 15.8, 'units': 'mm'})
    base = draft()
    call('sketch.finish')
    profile = inspect()['evaluation']['sketches'][0]['profiles'][0]['profileId']
    call('solid.extrude', {'sketchId': base['id'], 'profileId': profile, 'distance': 9.6, 'units': 'mm'})
    face = next(f for f in inspect()['evaluation']['bodies'][0]['supportFaces'] if f['role'] == 'extrude.endCap')
    call('sketch.begin', {'support': {k: face[k] for k in ('bodyId', 'producerFeatureId', 'faceId')}})
    call('sketch.addCircle', {'centerX': 8, 'centerY': 8, 'diameter': 5, 'units': 'mm'})
    circle = draft()['parameters']['entities'][0]
    lines = session()['projectionContext']['lines']
    left = next(e for e in lines if abs(e['u1Mm']) < 1e-6 and abs(e['u2Mm']) < 1e-6)
    top = next(e for e in lines if abs(e['v1Mm'] - 15.8) < 1e-6 and abs(e['v2Mm'] - 15.8) < 1e-6)
    for edge in (top, left):
        call('sketch.dimension', {'kind': 'pointLineDistance', 'pointIds': [circle['centerPointId']], 'entityId': edge['id'], 'value': 3.9, 'units': 'mm'})
    source_id = draft()['parameters']['dimensions'][0]['id']
    duplicate = {'kind': 'pointLineDistance', 'pointIds': [circle['centerPointId']], 'entityId': top['id'], 'value': 3.9, 'units': 'mm', 'annotation': {'uMm': 24, 'vMm': 12}}
    before, journal = draft(), session()['draftHistory']
    offer = call('sketch.dimension', duplicate)
    assert offer['pending'] and offer['kind'] == 'pointLineDistance'
    assert draft() == before and session()['draftHistory'] == journal
    call('sketch.dimensionOfferCancel', {'offerId': offer['offerId']})
    assert draft() == before and session()['draftHistory'] == journal
    assert 'pendingDrivenOffer' not in session()
    call('sketch.dimension', {**duplicate, 'value': 4.1}, error='sketch_solve_failed')
    assert draft() == before and session()['draftHistory'] == journal
    assert 'pendingDrivenOffer' not in session()
    offer = call('sketch.dimension', duplicate)
    call('sketch.dimensionOfferAccept', {'offerId': offer['offerId']})
    accepted = draft()
    measurement = accepted['parameters']['dimensions'][-1]
    assert measurement['id'] == offer['dimensionId'] and not measurement['driving'] and 'valueMm' not in measurement
    assert measurement['pointIds'] == duplicate['pointIds'] and measurement['entityId'] == top['id']
    for key in ('points', 'entities', 'projections', 'constraints'):
        assert accepted['parameters'].get(key) == before['parameters'].get(key)
    assert session()['draftHistory']['count'] == journal['count'] + 1
    call('sketch.tool', {'tool': 'select'})
    call('history.undo'); assert draft() == before
    call('history.redo'); assert draft() == accepted
    read_only = call('sketch.dimensionEdit', {'dimensionId': measurement['id']})
    assert read_only['driving'] is False
    call('sketch.dimensionCancel')
    call('sketch.dimensionUpdate', {'dimensionId': measurement['id'], 'value': 5}, error='sketch_solve_failed')
    assert draft() == accepted
    call('sketch.dimensionUpdate', {'dimensionId': source_id, 'value': 4.9, 'units': 'mm'})
    assert math.isclose(measured(measurement['id']), 4.9, abs_tol=1e-6)
    assert draft()['parameters']['dimensions'][-1] == measurement
    center = next(p for p in draft()['parameters']['points'] if p['id'] == circle['centerPointId'])
    assert math.isclose(center['uMm'], 3.9, abs_tol=1e-6) and math.isclose(center['vMm'], 10.9, abs_tol=1e-6)
    call('view.orientation', {'name': 'top'}); call('view.home')
    call('capture.screenshot', {'path': str(out / 'point-edge-driven-4.9.png')})
    state = call('app.state')
    assert state['viewport']['gpu']['hardwareAccelerated']
    call('sketch.finish')
    authored = inspect()['authored']
    path = out / 'point-edge-driven.alloy'
    call('document.save', {'path': str(path)})
    call('document.open', {'path': str(path)})
    assert inspect()['authored'] == authored
    call('sketch.edit', {'sketchId': accepted['id']})
    assert math.isclose(measured(measurement['id']), 4.9, abs_tol=1e-6)
    call('sketch.finish')
    (out / 'report.json').write_text(json.dumps({'passed': True, 'scope': 'point-edge redundant Driven offer only; no native input claim', 'measurement': measurement, 'centerAfterSourceEdit': center, 'gpu': state['viewport']['gpu'], 'calls': len(transcript)}, indent=2))
    (out / 'state.json').write_text(json.dumps(state, indent=2))
    (out / 'transcript.json').write_text(json.dumps(transcript, indent=2))
