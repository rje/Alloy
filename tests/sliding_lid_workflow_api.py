#!/usr/bin/env python3
"""Cumulative sliding-lid workflow with optional paced shared-API creation movie."""
import argparse
from contextlib import ExitStack
import json
import math
import subprocess
import time
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl

p = argparse.ArgumentParser()
p.add_argument('--app', required=True)
p.add_argument('--step-tool', required=True)
p.add_argument('--output', required=True)
p.add_argument('--record-video', type=Path)
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

def close(actual, expected, tolerance=1e-5):
    assert math.isclose(actual, expected, abs_tol=tolerance), (actual, expected)

with application(a.app, out) as (raw, transcript), ExitStack() as cleanup:
    recording = False
    recording_result = {}
    stages = []
    def stop_recording():
        global recording
        if recording:
            recording = False
            recording_result.update(raw('recording.stop'))
    cleanup.callback(stop_recording)
    raw('window.resize', {'width':1400, 'height':1000})
    if a.record_video:
        movie = a.record_video.resolve()
        assert not movie.is_relative_to(Path(__file__).resolve().parents[1])
        movie.parent.mkdir(parents=True, exist_ok=True)
        raw('recording.start', {'path':str(movie), 'fps':5})
        recording = True
    def call(command, args=None):
        result = raw(command, args)
        if recording and command.startswith(('sketch.', 'solid.', 'component.',
                'command.', 'feature.', 'history.', 'document.parameter',
                'document.modelParameter', 'joint.', 'view.')) and command not in (
                'view.projectSketchPoint', 'joint.inspect'):
            time.sleep(.35)
        return result
    def stage(name):
        stages.append(name)
        if recording:
            raw('recording.step', {'text':'API | ' + name})
            time.sleep(.8)
    stage('Create Box component: 50 x 80 x 40 mm; top Shell 4 mm')
    assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
    box_component = call('component.create', {'name':'Box'})
    owner = box_component['definitionId']
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
    stage('Front sketch: project face and inner edge reference')
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
    stage('Type 4 mm / 55 degrees; constrain the dovetail side')
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
    stage('Midpoint construction axis; connector and Mirror')
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
    stage('Cut the dovetail To Object: retained inner back face')
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
    # Project the actual cut-front lineage into the root-sibling Lid sketch.
    cut_body = call('document.inspect')['evaluation']['bodies'][0]
    cut_front = next(f for f in cut_body['supportFaces'] if f['role']=='box.y0')
    assert len(cut_front['edges'])==8
    stage('Create Lid component; project the actual cut front')
    lid = call('component.create', {'name':'Lid'})
    call('component.activate', {'definitionId':lid['definitionId']})
    call('sketch.begin', {'support':ref(cut_front), 'name':'Lid clearance'})
    call('sketch.lookAt')
    call('sketch.projectBegin')
    call('sketch.projectFace', ref(cut_front))
    call('sketch.projectCommit')
    lines = state()['projectionContext']['lines']
    def chosen(predicate):
        matches = [line for line in lines if predicate(line)]
        assert len(matches)==1, matches
        return matches[0]
    shoulder_l = chosen(lambda l: abs(l['v1Mm']-40)<1e-5 and abs(l['v2Mm']-40)<1e-5
                        and max(l['u1Mm'],l['u2Mm'])<=4.001)
    slant_l = chosen(lambda l: abs(max(l['v1Mm'],l['v2Mm'])-40)<1e-5
                     and 35<min(l['v1Mm'],l['v2Mm'])<40 and max(l['u1Mm'],l['u2Mm'])<25)
    lower = chosen(lambda l: abs(l['v1Mm']-l['v2Mm'])<1e-5 and 35<l['v1Mm']<40
                   and abs(l['u2Mm']-l['u1Mm'])>40)
    slant_r = chosen(lambda l: abs(max(l['v1Mm'],l['v2Mm'])-40)<1e-5
                     and 35<min(l['v1Mm'],l['v2Mm'])<40 and min(l['u1Mm'],l['u2Mm'])>25)
    shoulder_r = chosen(lambda l: abs(l['v1Mm']-40)<1e-5 and abs(l['v2Mm']-40)<1e-5
                        and min(l['u1Mm'],l['u2Mm'])>=45.999)
    source_ids = [line['id'] for line in (shoulder_l,slant_l,lower,slant_r,shoulder_r)]
    stage('Offset five-edge chain inward 0.25 mm; close at original top')
    original_lid_draft=draft()
    call('sketch.tool', {'tool':'offset'})
    preview=call('sketch.offsetPreview', {'sourceEntityIds':source_ids,'distance':-0.25,'units':'mm'})
    assert preview['pendingOpenChainOffset']['valid'] and draft()==original_lid_draft
    call('capture.screenshot', {'path':str(out/'lid-offset-preview.png')})
    call('sketch.offsetAccept')
    offset_group=draft()['parameters']['openChainOffsets'][0]
    assert offset_group['sourceEntityIds']==source_ids and len(offset_group['generatedEntityIds'])==5
    hits=offset_group['topIntersectionPointIds']
    assert len(hits)==2
    lid_points={point['id']:point for point in draft()['parameters']['points']}
    hit_l,hit_r=(lid_points[point_id] for point_id in hits)
    close(hit_l['vMm'],40);close(hit_r['vMm'],40)
    call('sketch.line', {'x1':hit_l['uMm'],'y1':hit_l['vMm'],
                         'x2':hit_r['uMm'],'y2':hit_r['vMm'],'units':'mm',
                         'startSnapTarget':{'kind':'point','pointId':hits[0]},
                         'endSnapTarget':{'kind':'point','pointId':hits[1]}})
    closure=draft()['parameters']['entities'][-1]
    assert closure['id'] not in offset_group['generatedEntityIds']
    lid_sketch_id=draft()['id']
    call('sketch.finish')
    lid_sketch=call('document.inspect')
    lid_profiles=[profile for profile in lid_sketch['evaluation']['sketches'][-1]['profiles']
                  if closure['id'] in profile['sourceEntityIds'] and lower['id'] not in profile['sourceEntityIds']]
    assert len(lid_profiles)==1, lid_profiles
    call('capture.screenshot', {'path':str(out/'lid-offset-closed.png')})
    stage('Extrude Lid To Object with 0.25 mm end clearance')
    call('solid.extrude', {'sketchId':lid_sketch_id,'profileId':lid_profiles[0]['profileId'],
        'extent':'toObject','endSupport':ref(back),'endOffset':0.25,'units':'mm'})
    two_solid=call('document.inspect')
    assert len(two_solid['evaluation']['bodies'])==2
    lid_body=next(b for b in two_solid['evaluation']['bodies'] if b['ownerId']==lid['definitionId'])
    box_body=next(b for b in two_solid['evaluation']['bodies'] if b['ownerId']==owner)
    assert lid_body['valid'] and box_body['valid'] and lid_body['closed'] and box_body['closed']
    assert two_solid['authored']['features'][-1]['parameters']['endSupport']==ref(back)
    assert 'distanceMm' not in two_solid['authored']['features'][-1]['parameters']
    call('view.home')
    call('capture.screenshot', {'path':str(out/'two-solid-lid.png')})

    # Only after both source solids exist, add the five user names and bind the
    # seven existing model rows through their retained targets.
    stage('Add named parameters and bind seven existing model fields')
    for name,value in (('Length',80),('Width',50),('Height',40),('Thickness',4),('Clearance',0.25)):
        call('document.parameterAdd', {'name':name,'value':value,'units':'mm'})
    def inspect(): return call('document.inspect')
    parameter_ids={row['name']:row['id'] for row in inspect()['userParameters']}
    lid_extrude_id=two_solid['authored']['features'][-1]['id']
    def row(name,kind,feature_id=None):
        found=[entry for entry in inspect()['modelParameters']
               if entry['name']==name and entry['featureType']==kind and
               (feature_id is None or entry['featureId']==feature_id)]
        assert len(found)==1, (name,kind,feature_id,found)
        return found[0]
    def bind(name,kind,expression,feature_id=None):
        target=row(name,kind,feature_id)['target']
        call('document.modelParameterSet', {'target':target,'expression':expression})
        current=row(name,kind,feature_id)
        assert current['parameterId']==parameter_ids[expression]
    assert len(inspect()['modelParameters'])>=7
    for name,kind,source_name in (('Width','box','Width'),('Depth','box','Length'),
                                  ('Height','box','Height'),('Thickness','shell','Thickness'),
                                  ('linear Dimension','sketch','Thickness'),
                                  ('Chain Offset','sketch','Clearance')):
        bind(name,kind,source_name)
    bind('End Offset','extrude','Clearance',lid_extrude_id)
    table_bound=inspect()
    call('document.parametersDialog')
    call('capture.dialog', {'path':str(out/'model-rows.png')})
    call('document.parametersDialog', {'visible':False})
    stage('Resize both parts: Length 40, Width 74, Height 100 mm')
    for name,value in (('Length',40),('Width',74),('Height',100)):
        call('document.parameterUpdate', {'parameterId':parameter_ids[name],'value':value})
    resized=inspect()
    assert len(resized['evaluation']['bodies'])==2
    close(row('Width','box')['valueMm'],74)
    close(row('Depth','box')['valueMm'],40)
    close(row('Height','box')['valueMm'],100)
    close(row('Chain Offset','sketch')['valueMm'],-0.25)
    close(row('End Offset','extrude',lid_extrude_id)['valueMm'],0.25)
    close(row('Thickness','shell')['valueMm'],4)
    close(row('linear Dimension','sketch')['valueMm'],4)
    box_resized=next(b for b in resized['evaluation']['bodies'] if b['ownerId']==owner)
    lid_resized=next(b for b in resized['evaluation']['bodies'] if b['ownerId']==lid['definitionId'])
    close(box_resized['boundsMm']['size'][0],74)
    close(box_resized['boundsMm']['size'][1],40)
    close(box_resized['boundsMm']['size'][2],100)
    assert lid_resized['valid'] and lid_resized['closed']
    close(lid_resized['boundsMm']['max'][1],40-4-0.25)
    features_by_id={feature['id']:feature for feature in resized['authored']['features']}
    retained_group=features_by_id[lid_sketch_id]['parameters']['openChainOffsets'][0]
    assert retained_group['id']==offset_group['id']
    for field in ('sourceEntityIds','generatedEntityIds','topIntersectionPointIds'):
        assert retained_group[field]==offset_group[field]
    assert features_by_id[lid_sketch_id]['parameters']['support']==ref(cut_front)
    assert features_by_id[lid_extrude_id]['parameters']['endSupport']==ref(back)
    assert len(next(f for f in box_resized['supportFaces'] if f['role']=='box.y0')['edges'])==8
    assert next(s for s in resized['evaluation']['sketches'] if s['featureId']==sketch_id)['solver']['dof']==0
    assert next(s for s in resized['evaluation']['sketches'] if s['featureId']==lid_sketch_id)['solver']['dof']==0
    call('view.home')
    call('capture.screenshot', {'path':str(out/'resized-pair.png')})
    # The Lid front is a retained base cap. Project its own top edge and snap
    # the 15 × 2 ellipse center to that stable edge's midpoint.
    stage('Thumb relief: midpoint ellipse, 15 x 2 mm')
    call('component.activate', {'definitionId':lid['definitionId']})
    lid_base=next(face for face in lid_resized['supportFaces'] if face['role']=='extrude.base')
    assert len(lid_base['edges'])==4
    call('sketch.begin', {'support':ref(lid_base),'name':'Thumb relief'})
    call('sketch.lookAt')
    call('sketch.projectBegin')
    call('sketch.projectFace', ref(lid_base))
    thumb_project=call('sketch.projectCommit')
    top_candidates=[line for line in thumb_project['sketchGeometry']['projectedLines']
                    if abs(line['v1Mm']-100)<1e-5 and abs(line['v2Mm']-100)<1e-5]
    assert len(top_candidates)==1,top_candidates
    lid_top=top_candidates[0]
    top_center=(lid_top['u1Mm']+lid_top['u2Mm'])/2
    close(top_center,37)
    call('sketch.centerPointEllipse', {'centerX':top_center,'centerY':100,
        'firstX':top_center+7.5,'firstY':100,'contourX':top_center,'contourY':101,
        'units':'mm','centerSnapTarget':{'kind':'midpoint','entityId':lid_top['id']}})
    ellipse=next(e for e in draft()['parameters']['entities'] if e['type']=='ellipse')
    call('sketch.constraint', {'type':'horizontal','entityIds':[ellipse['firstAxisId']]})
    for axis,value in ((ellipse['firstAxisId'],15),(ellipse['secondAxisId'],2)):
        call('sketch.dimension', {'kind':'linear','entityId':axis,'value':value,'units':'mm'})
    assert state()['solverDiagnostics']['dof']==0
    thumb_id=draft()['id']
    call('capture.screenshot', {'path':str(out/'thumb-sketch.png')})
    call('sketch.finish')
    thumb_source=inspect()
    thumb_geometry=next(s for s in thumb_source['evaluation']['sketches'] if s['featureId']==thumb_id)
    actual_ellipse=next(entity for entity in thumb_geometry['entities'] if entity['id']==ellipse['id'])
    close(actual_ellipse['centerMm'][0],37)
    close(actual_ellipse['centerMm'][2],100)
    assert thumb_geometry['solver']['dof']==0
    thumb_regions=[p for p in thumb_geometry['profiles'] if ellipse['id'] in p['sourceEntityIds']
                   and p['areaMm2']<30]
    assert thumb_regions,thumb_geometry['profileDiagnostics']
    before_thumb=next(b for b in thumb_source['evaluation']['bodies'] if b['ownerId']==lid['definitionId'])
    stage('Cut thumb relief: Start Offset -10 mm; Distance 5 mm')
    call('solid.extrude', {'sketchId':thumb_id,'profileIds':[p['profileId'] for p in thumb_regions],
        'start':'offset','startOffset':-10,'distance':5,'units':'mm',
        'operation':'cut','targetBodyIds':[before_thumb['bodyId']]})
    thumb_cut=inspect()
    after_thumb=next(b for b in thumb_cut['evaluation']['bodies'] if b['ownerId']==lid['definitionId'])
    close(before_thumb['volumeMm3']-after_thumb['volumeMm3'],math.pi*7.5*1*5/2,1e-3)
    assert after_thumb['valid'] and after_thumb['closed']
    call('view.home')
    call('capture.screenshot', {'path':str(out/'thumb-cut.png')})

    # A root Slider moves the actual Lid occurrence along the fixed Box's
    # retained top back ledge; source modeling stays in definition/rest space.
    call('component.activate', {'definitionId':thumb_cut['authored']['documentId']})
    fixed=next(b for b in thumb_cut['evaluation']['bodies'] if b['ownerId']==owner)
    ledges=[edge for edge in fixed['solidEdges'] if edge['available'] and
            len(edge['provenance'].get('startMm',[]))==3 and
            abs(edge['provenance']['startMm'][1]-40)<1e-5 and
            abs(edge['provenance']['endMm'][1]-40)<1e-5 and
            abs(edge['provenance']['startMm'][2]-100)<1e-5 and
            abs(edge['provenance']['endMm'][2]-100)<1e-5]
    assert len(ledges)==1,ledges
    position={'bodyId':fixed['bodyId'],'edgeId':ledges[0]['edgeId']}
    stage('As-built Slider: positive 0..50 mm opens through the front')
    joint=call('joint.slider', {'component1OccurrenceId':lid['occurrenceId'],
        'component2OccurrenceId':box_component['occurrenceId'],
        'position':position,'axis':'Y','minimumEnabled':True,'minimumMm':0,
        'maximumEnabled':True,'maximumMm':50,'positionMm':0})
    joint_state=call('joint.inspect')
    assert joint_state['joints'][0]['relativeDegreesOfFreedom']==1
    call('joint.previewPosition', {'positionMm':50})
    assert call('joint.inspect')['joints'][0]['transient']
    call('capture.screenshot', {'path':str(out/'slider-open.png')})
    call('joint.stop')
    call('joint.animate')
    time.sleep(.15)
    assert call('joint.inspect')['joints'][0]['transient']
    call('joint.stop')
    authored_joint=inspect()['authored']
    call('joint.positionSet', {'jointId':authored_joint['joints'][0]['id'],'positionMm':25})
    posed=inspect()
    moving=next(o for o in call('joint.inspect')['occurrences'] if o['occurrenceId']==lid['occurrenceId'])
    close(moving['bodies'][0]['boundsMm']['min'][1],25)
    call('document.parameterUpdate', {'parameterId':parameter_ids['Length'],'value':42})
    assert len(inspect()['evaluation']['bodies'])==2
    close(call('joint.inspect')['joints'][0]['origin']['pointMm'][1],42)
    moving=next(o for o in call('joint.inspect')['occurrences'] if o['occurrenceId']==lid['occurrenceId'])
    close(moving['bodies'][0]['boundsMm']['min'][1],25)
    call('document.parameterUpdate', {'parameterId':parameter_ids['Length'],'value':40})
    call('joint.positionSet', {'jointId':authored_joint['joints'][0]['id'],'positionMm':0})
    final=inspect()
    assert len(final['evaluation']['bodies'])==2
    call('view.home')
    call('capture.screenshot', {'path':str(out/'assembled-final.png')})
    call('history.undo');call('history.redo')
    assert inspect()['authored']==final['authored']
    stage('Save and reopen the editable assembly; export each printable part')
    final_path=out/'sliding-lid-complete.alloy'
    call('document.save', {'path':str(final_path)})
    call('document.open', {'path':str(final_path),'discard':True})
    assert inspect()['authored']==final['authored']
    rest_meshes={}
    for part,occurrence,body_id in (('box',box_component['occurrenceId'],fixed['bodyId']),
                                    ('lid',lid['occurrenceId'],after_thumb['bodyId'])):
        path=out/f'{part}-printable-rest.stl'
        call('document.exportStl', {'path':str(path),'occurrenceId':occurrence,
            'units':'mm','format':'binary','surfaceDeviationMm':0.1,'normalDeviationDeg':10})
        rest_meshes[part]=inspect_stl(path,True)
    for actual,expected in zip(rest_meshes['box']['bounds'][0],[0,0,0]):close(actual,expected,0.01)
    for actual,expected in zip(rest_meshes['box']['bounds'][1],[74,40,100]):close(actual,expected,0.01)
    for actual,expected in zip(rest_meshes['lid']['bounds'][0],
            [lid_resized['boundsMm']['min'][0],0,lid_resized['boundsMm']['min'][2]]):close(actual,expected,0.01)
    for actual,expected in zip(rest_meshes['lid']['bounds'][1],
            [lid_resized['boundsMm']['max'][0],35.75,100]):close(actual,expected,0.01)
    call('joint.positionSet', {'jointId':authored_joint['joints'][0]['id'],'positionMm':25})
    pose_export=inspect()
    lid_final=next(b for b in pose_export['evaluation']['bodies'] if b['ownerId']==lid['definitionId'])
    stl=out/'lid-world-25.stl'
    call('document.exportStl', {'path':str(stl),'occurrenceId':lid['occurrenceId'],
        'units':'mm','format':'binary','surfaceDeviationMm':0.1,'normalDeviationDeg':10})
    mesh=inspect_stl(stl,True)
    close(mesh['bounds'][0][1],25,0.01)
    close(mesh['bounds'][1][1],25+lid_final['boundsMm']['max'][1],0.01)
    step=out/'lid-world-25.step'
    call('document.exportStep', {'path':str(step),'bodyId':lid_final['bodyId'],'units':'mm'})
    step_read=json.loads(subprocess.run([str(Path(a.step_tool).resolve()),'read',str(step)],
        check=True,capture_output=True,text=True).stdout.splitlines()[-1])
    assert step_read['valid'] and step_read['solidCount']==1
    for actual,expected in zip(step_read['boundsMm'],
            [mesh['bounds'][0][0],mesh['bounds'][0][1],mesh['bounds'][0][2],
             mesh['bounds'][1][0],mesh['bounds'][1][1],mesh['bounds'][1][2]]):
        close(actual,expected,0.01)
    call('joint.positionSet', {'jointId':authored_joint['joints'][0]['id'],'positionMm':0})
    stage('Complete: separate Box and Lid printing files; editable assembly')
    if recording:
        time.sleep(2)
    stop_recording()
    (out/'receipt.json').write_text(json.dumps({'source':source, 'cut':cut,
        'twoSolid':two_solid,'tableBound':table_bound,'resized':resized,
        'thumbSource':thumb_source,'thumbCut':thumb_cut,'posed':posed,'final':final,
        'recording':recording_result, 'stages':stages,
        'restMeshes':rest_meshes,'stlBounds':mesh['bounds'],'step':step_read,
        'transcript':transcript},indent=2)+'\n')
print('PASS: cumulative source-order Box/Lid, parameter resize, thumb Cut, Slider and posed export')
