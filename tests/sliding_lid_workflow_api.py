#!/usr/bin/env python3
"""Cumulative sliding-lid tutorial prefix; extended as prerequisites are accepted."""
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
    assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
    owner = call('component.create', {'name':'Box'})['definitionId']
    call('component.activate', {'definitionId':owner})
    call('solid.box', {'width':50, 'depth':80, 'height':40, 'units':'mm', 'name':'Box'})
    body = call('document.inspect')['evaluation']['bodies'][0]
    top = next(f for f in body['supportFaces'] if f['role']=='box.z1')
    call('solid.shell', {'targetBodyId':body['bodyId'], 'removedFace':ref(top),
                         'thicknessMm':4})
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
    top_edge = next(l for l in geometry['projectedLines'] if
                    abs(l['v1Mm']-40)<1e-6 and abs(l['v2Mm']-40)<1e-6 and
                    abs(l['u2Mm']-l['u1Mm'])>40)
    call('sketch.addPoint', {'x':25, 'y':40, 'units':'mm'})
    center = draft()['parameters']['points'][-1]['id']
    call('sketch.constraint', {'type':'midpoint', 'pointIds':[center], 'entityIds':[top_edge['id']]})
    call('sketch.line', {'x1':25, 'y1':40, 'x2':25, 'y2':end['vMm'], 'units':'mm',
                         'startSnapTarget':{'kind':'point', 'pointId':center}})
    axis = draft()['parameters']['entities'][-1]
    call('sketch.construction', {'entityIds':[axis['id']], 'construction':True})
    call('sketch.constraint', {'type':'vertical', 'entityIds':[axis['id']]})
    call('sketch.constraint', {'type':'horizontal', 'pointIds':[axis['endPointId'],edge['endPointId']]})
    call('sketch.line', {'x1':end['uMm'], 'y1':end['vMm'], 'x2':25, 'y2':end['vMm'], 'units':'mm',
                         'startSnapTarget':{'kind':'point', 'pointId':edge['endPointId']},
                         'endSnapTarget':{'kind':'point', 'pointId':axis['endPointId']}})
    connector = draft()['parameters']['entities'][-1]
    assert state()['solverDiagnostics']['dof']==0
    half = draft()
    mirror = call('sketch.mirrorPreview', {'sourceEntityIds':[edge['id'],connector['id']], 'axisEntityId':axis['id']})
    assert mirror['pending'] and draft()==half
    call('sketch.mirrorAccept')
    assert len(draft()['parameters']['mirrorGroups'][-1]['entityPairs'])==2
    call('capture.screenshot', {'path':str(out/'dovetail-mirror.png')})
    bound = draft()
    call('sketch.tool', {'tool':'select'})
    call('history.undo')
    call('history.redo')
    assert draft()==bound
    call('sketch.finish')
    source = call('document.inspect')
    profiles = source['evaluation']['sketches'][-1]['profiles']
    cutter = next(p for p in profiles if edge['id'] in p['sourceEntityIds'] and p['areaMm2'] < 200)
    back = next(f for f in body['supportFaces'] if f['role']=='shell-inner' and
                abs(f['frame']['origin'][1]-76)<1e-6)
    inputs = {'sketchId':sketch_id, 'profileId':cutter['profileId'], 'extent':'toObject',
              'endSupport':ref(back), 'operation':'cut', 'targetBodyIds':[body['bodyId']]}
    session = call('solid.extrudeDialog', inputs)
    preview = call('command.preview', {'session':session['id']})
    assert abs(preview['resolvedDistanceMm']+76)<1e-6
    assert call('document.inspect')['authored']==source['authored']
    call('view.home')
    call('capture.screenshot', {'path':str(out/'dovetail-cut-preview.png')})
    call('command.commit', {'session':session['id']})
    cut = call('document.inspect')
    result = cut['evaluation']['bodies'][0]
    assert result['valid'] and result['closed'] and result['volumeMm3'] < body['volumeMm3']
    assert cut['authored']['features'][-1]['parameters']['endSupport']==ref(back)
    call('history.undo'); assert call('document.inspect')['authored']==source['authored']
    call('history.redo'); assert call('document.inspect')['authored']==cut['authored']
    box_feature = next(f for f in cut['authored']['features'] if f['type']=='box')
    edit = call('feature.editDialog', {'featureId':box_feature['id']})
    call('command.update', {'session':edit['id'], 'inputs':{'width':60}})
    call('command.commit', {'session':edit['id']})
    wider = call('document.inspect')
    assert abs(wider['evaluation']['bodies'][0]['boundsMm']['max'][0]-60)<1e-6
    call('history.undo'); assert call('document.inspect')['authored']==cut['authored']
    path = out/'sliding-lid-prefix.alloy'
    call('document.save', {'path':str(path)})
    call('document.open', {'path':str(path), 'discard':True})
    assert call('document.inspect')['authored']==cut['authored']
    call('capture.screenshot', {'path':str(out/'dovetail-cut-reopened.png')})
    (out/'receipt.json').write_text(json.dumps({'source':source, 'cut':cut,
        'transcript':transcript},indent=2)+'\n')
print('PASS: source numeric Box/Shell, projected 4/55 dovetail, mirror, retained To Object Cut and reopen')
