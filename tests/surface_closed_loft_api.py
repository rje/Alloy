#!/usr/bin/env python3
"""Bounded three-closed-section no-rail surface Loft through the real editor API."""
import argparse
import copy
import json
from pathlib import Path
from app_harness import application

parser=argparse.ArgumentParser()
parser.add_argument('--app',required=True)
parser.add_argument('--source',required=True)
parser.add_argument('--output',required=True)
args=parser.parse_args()
out=Path(args.output).resolve();out.mkdir(parents=True,exist_ok=True)
with application(args.app,out,command_timeout=90) as (call,transcript):
    call('window.resize',{'width':1440,'height':900})
    call('document.open',{'path':str(Path(args.source).resolve()),'discard':True})
    initial=call('document.inspect')
    features=initial['authored']['features']
    first=next(f for f in features if f['name']=='First closed spline section')
    middle=next(f for f in features if f['name']=='Middle full ellipse')
    last=next(f for f in features if f['name']=='Last closed spline section')
    def ids(feature,kind):
        return [e['id'] for e in feature['parameters']['entities'] if e['type']==kind]
    sections=[{'sketchId':first['id'],'entityIds':ids(first,'fitSpline')},
              {'sketchId':middle['id'],'entityIds':ids(middle,'ellipse')},
              {'sketchId':last['id'],'entityIds':ids(last,'fitSpline')}]
    assert list(map(lambda row:len(row['entityIds']),sections))==[2,1,2]
    call('view.home')
    dialog=call('surface.loftDialog',{'guideType':'none','componentName':'Iron upper',
                                      'name':'Three-section open sheet'})
    assert not dialog['previewed']
    for index,feature in enumerate((first,middle,last),1):
        for entity in sections[index-1]['entityIds']:
            call('solid.loftEntity',{'role':f'section{index}','sketchId':feature['id'],'entityId':entity})
    preview=call('command.preview',{'session':dialog['id']})
    assert preview['previewed'] and preview['geometry']['topology']['faces']==2
    assert preview['geometry']['topology']['solids']==0
    call('capture.dialog',{'path':str(out/'three-section-loft-dock.png')})
    call('capture.screenshot',{'path':str(out/'three-section-loft-preview.png')})
    call('command.cancel',{'session':dialog['id']})
    assert call('document.inspect')['authored']==initial['authored']
    inputs={'guideType':'none','operation':'newComponent','componentName':'Iron upper',
            'name':'Three-section open sheet','sections':sections}
    session=call('surface.loftDialog',inputs)
    assert session['previewed']
    call('command.commit',{'session':session['id']})
    committed=call('document.inspect')
    loft=committed['authored']['features'][-1]
    body=committed['evaluation']['bodies'][0]
    faces=[f['faceId'] for f in body['supportFaces']]
    edges=sorted(e['edgeId'] for e in body['solidEdges'])
    assert loft['parameters']=={'guideType':'none','operation':'newComponent','sections':sections}
    assert body['valid'] and body['type']=='surface' and not body['closed']
    assert body['topology']['faces']==2 and body['topology']['solids']==0
    assert len(faces)==2 and len(set(faces))==2 and len(edges)==6
    for f in (first,middle,last):call('feature.visibility',{'featureId':f['id'],'visible':False})
    hidden=call('document.inspect')
    assert [f['faceId'] for f in hidden['evaluation']['bodies'][0]['supportFaces']]==faces
    params=copy.deepcopy(first['parameters'])
    midpoint=params['entities'][0]['pointIds'][2]
    for point in params['points']:
        if point['id']==midpoint:point['vMm']=48
    call('feature.edit',{'featureId':first['id'],'parameters':params})
    edited=call('document.inspect');current=edited['evaluation']['bodies'][0]
    assert [f['faceId'] for f in current['supportFaces']]==faces
    assert sorted(e['edgeId'] for e in current['solidEdges'])==edges
    assert abs(current['areaMm2']-body['areaMm2'])>1
    call('history.undo');call('history.redo')
    assert call('document.inspect')['authored']==edited['authored']
    saved=out/'three-section-open-sheet.alloy'
    call('document.save',{'path':str(saved)})
    call('document.new',{'discard':True});call('document.open',{'path':str(saved),'discard':True})
    reopened=call('document.inspect')
    assert reopened['authored']==edited['authored']
    assert [f['faceId'] for f in reopened['evaluation']['bodies'][0]['supportFaces']]==faces
    edit=call('feature.editDialog',{'featureId':loft['id']})
    assert edit['previewed'] and edit['inputs']['guideType']=='none'
    assert edit['inputs']['sections']==sections
    call('command.cancel',{'session':edit['id']})
    assert call('document.inspect')['authored']==edited['authored']
    call('view.home');call('capture.screenshot',{'path':str(out/'three-section-loft-reopened.png')})
    (out/'result.json').write_text(json.dumps({'passed':True,'featureId':loft['id'],
        'bodyId':body['bodyId'],'faceIds':faces,'boundaryEdgeIds':edges,
        'initialAreaMm2':body['areaMm2'],'editedAreaMm2':current['areaMm2']},indent=2)+'\n')
    (out/'transcript.json').write_text(json.dumps(transcript,indent=2)+'\n')
