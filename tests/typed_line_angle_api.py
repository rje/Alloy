#!/usr/bin/env python3
"""Real-app Box/Shell Project to typed angled Line and retained corner relation."""
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

def ref(face):
    return {key: face[key] for key in ('producerFeatureId', 'bodyId', 'faceId')}

def along_y(edge):
    source = edge.get('provenance', edge)
    x, y, z = source['startMm']
    xx, yy, zz = source['endMm']
    return abs(x-xx)<1e-6 and abs(z-zz)<1e-6 and abs(y-yy)>1

with application(a.app, out) as (call, transcript):
    for name, value in [('Width', 50), ('Length', 80), ('Height', 40), ('Thickness', 4)]:
        call('document.parameterAdd', {'name':name, 'value':value, 'units':'mm'})
    ids = {row['name']:row['id'] for row in call('document.inspect')['userParameters']}
    call('solid.box', {'widthParameter':'Width', 'depthParameter':'Length',
                       'heightParameter':'Height', 'units':'mm', 'name':'Box'})
    body = call('document.inspect')['evaluation']['bodies'][0]
    top = next(f for f in body['supportFaces'] if f['role']=='box.z1')
    call('solid.shell', {'targetBodyId':body['bodyId'], 'removedFace':ref(top),
                         'thicknessParameter':'Thickness'})
    body = call('document.inspect')['evaluation']['bodies'][0]
    front = next(f for f in body['supportFaces'] if f['role']=='box.y0')
    inner = next(e for f in body['supportFaces'] if f['role']=='shell-inner'
                 for e in f['edges'] if along_y(e) and abs(e.get('provenance',e)['startMm'][0]-4)<1e-6)
    call('sketch.begin', {'support':ref(front), 'name':'Dovetail front'})
    call('sketch.lookAt')
    call('sketch.projectBegin')
    call('sketch.projectFace', ref(front))
    call('sketch.projectSelect', {'bodyId':body['bodyId'], 'edgeId':inner['edgeId']})
    projected = call('sketch.projectCommit')
    sketch_id = projected['draftSketch']['id']
    geometry = projected['sketchGeometry']
    inner_point = next(p for p in geometry['projectedPoints'] if abs(p['uMm']-4)<1e-6 and abs(p['vMm']-40)<1e-6)
    corner = next(l['endPointId'] if abs(l['u2Mm'])<1e-6 and abs(l['v2Mm']-40)<1e-6 else l['startPointId']
                  for l in geometry['projectedLines'] if
                  (abs(l['u1Mm'])<1e-6 and abs(l['v1Mm']-40)<1e-6) or
                  (abs(l['u2Mm'])<1e-6 and abs(l['v2Mm']-40)<1e-6))
    def pixel(u, v):
        p = call('view.projectSketchPoint', {'sketchId':sketch_id, 'uMm':u, 'vMm':v})
        return {'x':p['x'], 'y':p['y']}
    def state(): return call('app.state')['activeSession']
    def draft(): return state()['draftSketch']
    call('sketch.tool', {'tool':'line'})
    call('sketch.placePoint', pixel(4,40))
    baseline = draft()
    call('sketch.lineLengthPreview', {'text':'4'})
    call('sketch.lineLengthLock')
    call('sketch.lineAnglePreview', {'text':'180'})
    call('sketch.placePoint', pixel(2,37), error='invalid_line_angle')
    assert draft()==baseline
    call('sketch.lineLengthCancel')
    assert draft()==baseline
    call('sketch.lineLengthPreview', {'text':'4'})
    call('sketch.lineLengthLock')
    call('sketch.lineAnglePreview', {'text':'55'})
    call('sketch.pointerMove', pixel(2,37))
    assert draft()==baseline
    call('capture.screenshot', {'path':str(out/'typed-preview.png')})
    call('sketch.lineAngleLock')
    placed = call('sketch.placePoint', pixel(2,37))
    parameters = draft()['parameters']
    helper, edge = parameters['entities'][-2:]
    assert helper['construction'] and helper['startPointId']==edge['startPointId']
    assert parameters['dimensions'][-2]['kind']=='linear' and parameters['dimensions'][-2]['valueMm']==4
    assert parameters['dimensions'][-1]['kind']=='angle' and parameters['dimensions'][-1]['valueDeg']==55
    assert state()['firstPoint']['target']['pointId']==edge['endPointId']
    assert any(c['type']=='coincident' and inner_point['id'] in c['pointIds']
               for c in parameters['constraints'])
    assert state()['solverDiagnostics']['dof']==1
    points = {p['id']:p for p in parameters['points']}
    start, end = points[edge['startPointId']], points[edge['endPointId']]
    assert abs(math.hypot(end['uMm']-start['uMm'], end['vMm']-start['vMm'])-4)<1e-6
    call('capture.screenshot', {'path':str(out/'typed-accepted.png')})
    call('sketch.constraint', {'type':'coincident', 'pointIds':[helper['endPointId'],corner]})
    assert state()['solverDiagnostics']['dof']==0
    linear = parameters['dimensions'][-2]
    call('sketch.dimensionUpdate', {'dimensionId':linear['id'], 'parameterName':'Thickness'})
    top_edge = next(l for l in geometry['projectedLines'] if
                    abs(l['v1Mm']-40)<1e-6 and abs(l['v2Mm']-40)<1e-6 and
                    abs(l['u2Mm']-l['u1Mm'])>40)
    bottom_edge = next(l for l in geometry['projectedLines'] if
                       abs(l['v1Mm'])<1e-6 and abs(l['v2Mm'])<1e-6 and
                       abs(l['u2Mm']-l['u1Mm'])>40)
    call('sketch.addPoint', {'x':25, 'y':40, 'units':'mm'})
    center = draft()['parameters']['points'][-1]['id']
    call('sketch.constraint', {'type':'midpoint', 'pointIds':[center], 'entityIds':[top_edge['id']]})
    call('sketch.line', {'x1':25, 'y1':40, 'x2':25, 'y2':0, 'units':'mm',
                         'startSnapTarget':{'kind':'point', 'pointId':center}})
    axis = draft()['parameters']['entities'][-1]
    call('sketch.construction', {'entityIds':[axis['id']], 'construction':True})
    call('sketch.constraint', {'type':'vertical', 'entityIds':[axis['id']]})
    call('sketch.constraint', {'type':'pointOnCurve', 'pointIds':[axis['endPointId']], 'entityIds':[bottom_edge['id']]})
    call('sketch.line', {'x1':end['uMm'], 'y1':end['vMm'], 'x2':25, 'y2':end['vMm'], 'units':'mm',
                         'startSnapTarget':{'kind':'point', 'pointId':edge['endPointId']}})
    connector = draft()['parameters']['entities'][-1]
    call('sketch.constraint', {'type':'horizontal', 'entityIds':[connector['id']]})
    call('sketch.constraint', {'type':'pointOnCurve', 'pointIds':[connector['endPointId']], 'entityIds':[axis['id']]})
    half = draft()
    mirror = call('sketch.mirrorPreview', {'sourceEntityIds':[edge['id'],connector['id']], 'axisEntityId':axis['id']})
    assert mirror['pending'] and draft()==half
    call('sketch.mirrorAccept')
    assert len(draft()['parameters']['mirrorGroups'][-1]['entityPairs'])==2
    call('capture.screenshot', {'path':str(out/'dovetail-mirror.png')})
    bound = draft()
    assert bound['parameters']['dimensions'][-2]['parameterId']==ids['Thickness']
    call('sketch.tool', {'tool':'select'})
    call('history.undo')
    call('history.redo')
    assert draft()==bound
    call('sketch.finish')
    authored = call('document.inspect')['authored']
    call('document.parameterUpdate', {'parameterId':ids['Width'], 'value':74})
    call('document.parameterUpdate', {'parameterId':ids['Thickness'], 'value':3.5})
    edited = call('document.inspect')
    assert edited['authored']['features'][-1]['parameters']['dimensions'][-2]['valueMm']==3.5
    path = out/'typed-angle.alloy'
    call('document.save', {'path':str(path)})
    call('document.open', {'path':str(path), 'discard':True})
    assert call('document.inspect')['authored']==edited['authored']
    (out/'result.json').write_text(json.dumps({'baseline':baseline, 'placed':placed,
        'bound':bound, 'initial':authored, 'edited':edited, 'transcript':transcript},indent=2))
print('PASS: typed 4 mm/55 deg Line on Box/Shell Project, retained helper, Coincident, Thickness, resize/reopen')
