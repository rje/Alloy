#!/usr/bin/env python3
"""Bounded real-app user-length expression and retained rectangle/extrusion gate."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application

p = argparse.ArgumentParser()
p.add_argument('--app', required=True)
p.add_argument('--output', required=True)
a = p.parse_args()
out = Path(a.output).resolve()
out.mkdir(parents=True, exist_ok=True)

def close(value, expected):
    assert math.isclose(value, expected, abs_tol=1e-5), (value, expected)

with application(a.app, out) as (call, transcript):
    def inspect(): return call('document.inspect')
    assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
    call('document.parameterAdd', {'name':'Width', 'value':40, 'units':'mm'})
    call('document.parameterAdd', {'name':'Length', 'expression':'Width*2', 'units':'mm'})
    call('document.parameterAdd', {'name':'Depth', 'value':5, 'units':'mm'})
    rows = {r['name']:r for r in inspect()['userParameters']}
    assert rows['Length']['expression']=='Width*2'
    assert rows['Length']['referenceIds']==[rows['Width']['id']]
    close(rows['Length']['valueMm'], 80)
    call('sketch.begin', {'plane':'XY', 'units':'mm', 'name':'Parameter rectangle'})
    call('sketch.rectangle', {'x1':-40, 'y1':-20, 'x2':40, 'y2':20, 'units':'mm'})
    draft = call('app.state')['activeSession']['draftSketch']
    edges = draft['parameters']['entities']
    call('sketch.dimension', {'kind':'horizontal', 'entityId':edges[0]['id'], 'parameterName':'Length'})
    call('sketch.dimension', {'kind':'vertical', 'entityId':edges[1]['id'], 'parameterName':'Width'})
    call('sketch.finish')
    sketch = inspect()['evaluation']['sketches'][0]
    call('solid.extrude', {'sketchId':sketch['sketchId'], 'profileId':sketch['profiles'][0]['profileId'],
                           'distanceParameter':'Depth', 'units':'mm'})
    initial = inspect()
    close(initial['evaluation']['bodies'][0]['volumeMm3'], 16000)
    call('document.parametersDialog')
    call('capture.dialog', {'path':str(out/'expression-table.png')})
    call('document.parametersDialog', {'visible':False})
    call('document.parameterUpdate', {'parameterId':rows['Width']['id'], 'value':45})
    edited = inspect()
    close(next(r for r in edited['userParameters'] if r['name']=='Length')['valueMm'], 90)
    close(edited['evaluation']['bodies'][0]['volumeMm3'], 20250)
    before_refusal = edited['authored']
    call('document.parameterUpdate', {'parameterId':rows['Length']['id'], 'expression':'Width+5'}, error='invalid_parameter')
    assert inspect()['authored']==before_refusal
    call('document.parameterUpdate', {'parameterId':rows['Width']['id'], 'name':'PanelWidth'})
    renamed = inspect()
    formula = next(r for r in renamed['userParameters'] if r['name']=='Length')
    assert formula['expression']=='PanelWidth*2' and formula['referenceIds']==[rows['Width']['id']]
    close(renamed['evaluation']['bodies'][0]['volumeMm3'], 20250)
    call('history.undo')
    assert next(r for r in inspect()['userParameters'] if r['name']=='Length')['expression']=='Width*2'
    call('history.redo')
    assert inspect()['authored']==renamed['authored']
    path = out/'expression-rectangle.alloy'
    call('document.save', {'path':str(path)})
    call('document.open', {'path':str(path), 'discard':True})
    assert inspect()['authored']==renamed['authored']
    call('view.home')
    call('capture.screenshot', {'path':str(out/'expression-solid.png')})
    (out/'result.json').write_text(json.dumps({'initial':initial, 'edited':edited,
        'renamed':renamed, 'transcript':transcript}, indent=2))
print('PASS: user formula drives bound rectangle/extrude, refusal, rename, history and reopen')
