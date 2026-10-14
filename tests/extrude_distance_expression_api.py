#!/usr/bin/env python3
"""One fixed-profile Extrude formula through the real shared command authority."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application

parser = argparse.ArgumentParser()
parser.add_argument('--app', required=True)
parser.add_argument('--output', required=True)
args = parser.parse_args()
out = Path(args.output).resolve()
out.mkdir(parents=True, exist_ok=True)

def close(actual, expected):
    assert math.isclose(actual, expected, abs_tol=1e-5), (actual, expected)

with application(args.app, out) as (call, transcript):
    row = call('document.parameterAdd', {'name':'Width', 'value':40, 'units':'mm'})
    parameter_id = next(r['id'] for r in call('document.inspect')['userParameters'] if r['name']=='Width')
    call('sketch.begin', {'plane':'XY', 'units':'mm', 'name':'Fixed profile'})
    call('sketch.rectangle', {'x1':0, 'y1':0, 'x2':40, 'y2':30, 'units':'mm'})
    call('sketch.finish')
    sketch = call('document.inspect')['evaluation']['sketches'][0]
    profile = sketch['profiles'][0]['profileId']
    call('solid.extrude', {'sketchId':sketch['sketchId'], 'profileId':profile,
                           'distanceExpression':'Width*2', 'units':'mm'})
    first = call('document.inspect')
    feature = first['authored']['features'][-1]
    feature_id = feature['id']
    p = feature['parameters']
    assert p['distanceExpression']=='Width*2' and p['distanceReferenceIds']==[parameter_id]
    model_row = next(r for r in first['modelParameters'] if r['featureId']==feature_id and r['name']=='Distance')
    assert model_row['expression']=='Width*2' and model_row['readOnly'] and not model_row['editable']
    call('document.modelParameterSet', {'target':model_row['target'], 'expression':'60 mm'}, error='invalid_model_expression')
    assert call('document.inspect')['authored']==first['authored']
    close(p['distanceMm'],80)
    close(first['evaluation']['bodies'][0]['volumeMm3'],96000)
    call('document.parameterUpdate', {'parameterId':parameter_id, 'value':45})
    updated = call('document.inspect')
    assert updated['authored']['features'][-1]['parameters']['distanceExpression']=='Width*2'
    close(updated['evaluation']['bodies'][0]['volumeMm3'],108000)
    close(updated['authored']['features'][-1]['parameters']['distanceMm'],90)
    unchanged = updated['authored']
    call('document.parameterUpdate', {'parameterId':parameter_id, 'value':60000}, error='parameter_evaluation_failed')
    assert call('document.inspect')['authored']==unchanged
    call('solid.extrude', {'featureId':feature_id, 'sketchId':sketch['sketchId'], 'profileId':profile,
                           'distanceExpression':'Width+5', 'units':'mm'}, error='invalid_arguments')
    assert call('document.inspect')['authored']==unchanged
    call('document.parameterUpdate', {'parameterId':parameter_id, 'name':'Span'})
    renamed = call('document.inspect')
    p = renamed['authored']['features'][-1]['parameters']
    assert p['distanceExpression']=='Span*2' and p['distanceReferenceIds']==[parameter_id]
    close(renamed['evaluation']['bodies'][0]['volumeMm3'],108000)
    call('history.undo')
    assert call('document.inspect')['authored']==unchanged
    call('history.redo')
    assert call('document.inspect')['authored']==renamed['authored']
    saved = out/'extrude-distance-expression.alloy'
    call('document.save', {'path':str(saved)})
    call('document.open', {'path':str(saved), 'discard':True})
    reopened = call('document.inspect')
    assert reopened['authored']==renamed['authored']
    close(reopened['evaluation']['bodies'][0]['volumeMm3'],108000)
    call('solid.extrude', {'featureId':feature_id, 'sketchId':sketch['sketchId'], 'profileId':profile,
                           'distanceExpression':'-Span*2', 'units':'mm'})
    negative = call('document.inspect')
    close(negative['authored']['features'][-1]['parameters']['distanceMm'],-90)
    close(negative['evaluation']['bodies'][0]['volumeMm3'],108000)
    call('history.undo')
    assert call('document.inspect')['authored']==renamed['authored']
    call('solid.extrude', {'featureId':feature_id, 'sketchId':sketch['sketchId'], 'profileId':profile,
                           'distanceExpression':'2*3', 'units':'in'})
    call('document.units', {'units':'mm'})
    session = call('feature.editDialog', {'featureId':feature_id})
    assert session['inputs']['distanceExpression']=='2*3'
    call('command.preview', {'session':session['id']})
    call('command.commit', {'session':session['id']})
    scalar = call('document.inspect')
    assert scalar['authored']['features'][-1]['parameters']['distanceExpressionUnits']=='in'
    close(scalar['authored']['features'][-1]['parameters']['distanceMm'],152.4)
    close(scalar['evaluation']['bodies'][0]['volumeMm3'],182880)
    (out/'result.json').write_text(json.dumps({'first':first,'updated':updated,'renamed':renamed,
        'negative':negative,'scalar':scalar,'transcript':transcript},indent=2))
print('PASS: retained Extrude formula, stable UUID, update, refusal, rename, history and reopen')
