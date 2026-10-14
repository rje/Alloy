#!/usr/bin/env python3
"""Two root-sibling components, real Slider controls, world pose and export."""
import argparse
import json
import math
import subprocess
import time
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--app',required=True)
parser.add_argument('--step-tool',required=True)
parser.add_argument('--output',required=True)
args=parser.parse_args()
output=Path(args.output).resolve()

def close(actual,expected,tolerance=1e-4):
    assert math.isclose(actual,expected,abs_tol=tolerance), (actual,expected)

with application(args.app,output) as (call,transcript):
    assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
    depth=call('document.parameterAdd',{'name':'BoxDepth','value':80,'units':'mm'})['userParameters'][0]
    box=call('component.create',{'name':'Box','activate':True})
    call('solid.box',{'width':50,'depthParameter':'BoxDepth','height':40,'units':'mm','name':'Box stock'})
    lid=call('component.create',{'name':'Lid','activate':True})
    call('solid.box',{'width':50,'depth':80,'height':8,'z':40,'units':'mm','name':'Lid stock'})
    call('component.activate',{'definitionId':call('document.inspect')['authored']['documentId']})
    before=call('document.inspect')
    box_body=next(b for b in before['evaluation']['bodies'] if b['ownerId']==box['definitionId'])
    lid_body=next(b for b in before['evaluation']['bodies'] if b['ownerId']==lid['definitionId'])
    edge=next(e for e in box_body['solidEdges'] if e['available'] and
        len(e['provenance'].get('startMm',[]))==3 and
        abs(e['provenance']['startMm'][2]-40)<1e-5 and
        abs(e['provenance']['endMm'][2]-40)<1e-5 and
        abs(e['provenance']['startMm'][1]-80)<1e-5 and
        abs(e['provenance']['endMm'][1]-80)<1e-5)
    position={'bodyId':box_body['bodyId'],'edgeId':edge['edgeId']}
    joint_args={'component1OccurrenceId':lid['occurrenceId'],
        'component2OccurrenceId':box['occurrenceId'],'position':position,
        'axis':'Y','minimumEnabled':True,'minimumMm':0,
        'maximumEnabled':True,'maximumMm':50,'positionMm':0}
    session=call('joint.sliderDialog')
    call('command.update',{'session':session['id'],'inputs':joint_args})
    preview=call('command.preview',{'session':session['id']})
    assert preview['previewed'] and preview['assembly']['joints'][0]['origin']['edgeId']==edge['edgeId']
    call('view.home')
    call('capture.screenshot',{'path':str(output/'slider-dialog-preview.png')})
    call('command.cancel',{'session':session['id']})
    assert call('document.inspect')['authored']==before['authored']
    call('joint.slider',joint_args)
    committed=call('document.inspect')
    assert committed['authored']['features']==before['authored']['features']
    joint=committed['authored']['joints'][0]
    assert joint['position']==position and joint['axis']=='Y'
    assert call('joint.inspect')['joints'][0]['relativeDegreesOfFreedom']==1
    assert call('joint.inspect')['joints'][0]['gaugeOccurrenceId']==box['occurrenceId']
    call('joint.previewPosition',{'positionMm':50})
    preview_state=call('joint.inspect')
    moving=next(o for o in preview_state['occurrences'] if o['occurrenceId']==lid['occurrenceId'])
    close(moving['bodies'][0]['boundsMm']['min'][1],50)
    assert call('document.inspect')['authored']==committed['authored']
    call('joint.stop')
    call('joint.positionSet',{'jointId':joint['id'],'positionMm':25})
    call('document.parameterUpdate',{'parameterId':depth['id'],'value':90})
    driven=call('document.inspect')
    moving=next(o for o in call('joint.inspect')['occurrences'] if o['occurrenceId']==lid['occurrenceId'])
    close(moving['bodies'][0]['boundsMm']['min'][1],25)
    close(call('joint.inspect')['joints'][0]['origin']['pointMm'][1],90)
    close(driven['evaluation']['bodies'][1]['boundsMm']['min'][1],0)
    call('solid.box',{'width':1,'depth':1,'height':1,'units':'mm'},error='assembly_pose_active')
    call('joint.positionSet',{'jointId':joint['id'],'positionMm':51},error='evaluation_failed')
    assert call('document.inspect')['authored']==driven['authored']
    call('view.home')
    moved_top=next(face for face in lid_body['supportFaces'] if face['role']=='box.z1')
    pixel=call('view.projectFacePoint',{'support':{key:moved_top[key] for key in ('producerFeatureId','bodyId','faceId')},'uMm':25,'vMm':40})
    picked=call('selection.facePick',{'x':pixel['x'],'y':pixel['y']})
    assert picked['occurrenceId']==lid['occurrenceId'] and picked['support']=={key:moved_top[key] for key in ('producerFeatureId','bodyId','faceId')}
    close(picked['frame']['origin'][1],25)
    time.sleep(.12)
    call('capture.screenshot',{'path':str(output/'slider-driven-25.png')})
    stl=output/'lid-world-25.stl'
    call('document.exportStl',{'path':str(stl),'occurrenceId':lid['occurrenceId'],
        'units':'mm','format':'binary','surfaceDeviationMm':0.1,'normalDeviationDeg':10})
    mesh=inspect_stl(stl,True)
    for actual,expected in zip(mesh['bounds'][0],[0,25,40]):close(actual,expected,0.01)
    for actual,expected in zip(mesh['bounds'][1],[50,105,48]):close(actual,expected,0.01)
    step=output/'lid-world-25.step'
    call('document.exportStep',{'path':str(step),'bodyId':lid_body['bodyId'],'units':'mm'})
    step_read=json.loads(subprocess.run([str(Path(args.step_tool).resolve()),'read',str(step)],
        check=True,capture_output=True,text=True).stdout.splitlines()[-1])
    assert step_read['valid'] and step_read['solidCount']==1
    for actual,expected in zip(step_read['boundsMm'],[0,25,40,50,105,48]):close(actual,expected)
    session=call('joint.sliderDialog',{'jointId':joint['id']})
    assert session['inputs']['positionMm']==25
    call('command.cancel',{'session':session['id']})
    call('history.undo')
    close(call('joint.inspect')['joints'][0]['origin']['pointMm'][1],80)
    call('history.undo')
    close(call('joint.inspect')['joints'][0]['coordinateMm'],0)
    call('history.redo')
    call('history.redo')
    assert call('document.inspect')['authored']==driven['authored']
    path=output/'slider.alloy'
    call('document.save',{'path':str(path)})
    call('document.open',{'path':str(path),'discard':True})
    assert call('document.inspect')['authored']==driven['authored']
    close(call('joint.inspect')['joints'][0]['coordinateMm'],25)
    call('joint.animate')
    time.sleep(.15)
    assert call('joint.inspect')['joints'][0]['transient']
    call('joint.stop')
    assert call('document.inspect')['authored']==driven['authored']
    call('capture.screenshot',{'path':str(output/'slider-reopened.png')})
    call('joint.slider',{'jointId':joint['id'],'reverseDirection':True})
    reversed_joint=call('document.inspect')['authored']['joints'][0]
    assert reversed_joint['reverseDirection'] and reversed_joint['positionMm']==25
    moving=next(o for o in call('joint.inspect')['occurrences'] if o['occurrenceId']==lid['occurrenceId'])
    close(moving['translationMm'][1],-25)
    close(moving['bodies'][0]['boundsMm']['min'][1],-25)
    call('joint.previewPosition',{'positionMm':50})
    moving=next(o for o in call('joint.inspect')['occurrences'] if o['occurrenceId']==lid['occurrenceId'])
    close(moving['bodies'][0]['boundsMm']['max'][1],30)
    call('joint.stop')
    reverse_stl=output/'lid-world-reversed-25.stl'
    call('document.exportStl',{'path':str(reverse_stl),'occurrenceId':lid['occurrenceId'],
        'units':'mm','format':'binary','surfaceDeviationMm':0.1,'normalDeviationDeg':10})
    reverse_mesh=inspect_stl(reverse_stl,True)
    close(reverse_mesh['bounds'][0][1],-25,0.01)
    close(reverse_mesh['bounds'][1][1],55,0.01)
    call('capture.screenshot',{'path':str(output/'slider-reversed-25.png')})
    call('history.undo')
    assert not call('document.inspect')['authored']['joints'][0].get('reverseDirection',False)
    call('history.redo')
    assert call('document.inspect')['authored']['joints'][0]==reversed_joint
    reverse_path=output/'slider-reversed.alloy'
    call('document.save',{'path':str(reverse_path)})
    call('document.open',{'path':str(reverse_path),'discard':True})
    assert call('document.inspect')['authored']['joints'][0]==reversed_joint

(output/'receipt.json').write_text(json.dumps({'jointId':joint['id'],'boxOccurrenceId':box['occurrenceId'],
    'lidOccurrenceId':lid['occurrenceId'],'position':position,'worldStlBounds':mesh['bounds'],
    'worldStepBounds':step_read['boundsMm'],'reverseStlBounds':reverse_mesh['bounds'],
    'callCount':len(transcript),'transcript':transcript},indent=2)+'\n')
print('PASS: Slider dialog/Cancel, one DOF, drive/limits, both directions, world STEP/STL, undo/reopen, animation')
