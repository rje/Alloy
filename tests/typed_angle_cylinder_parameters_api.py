#!/usr/bin/env python3
"""Typed Angle/Cylinder Height lifecycle on one printable wedge-cut cylinder."""
import argparse,json,math
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl

p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
with application(a.app,out) as (call,log):
    inspect=lambda:call('document.inspect')
    draft=lambda:call('app.state')['activeSession']['draftSketch']
    def volume(angle,height):
        state=inspect();body=state['evaluation']['bodies'][0]
        assert body['valid'] and body['closed'] and body['topology']['solids']==1,body
        assert math.isclose(body['volumeMm3'],(math.pi-math.radians(angle)/2)*100*height,abs_tol=1e-4),body
        assert math.isclose(body['boundsMm']['size'][2],height,abs_tol=1e-6)
        return state
    for name,value,units in [('Opening',30,'deg'),('Height',10,'mm')]:call('document.parameterAdd',{'name':name,'value':value,'units':units})
    ids={r['name']:r['id'] for r in inspect()['userParameters']}
    call('solid.cylinder',{'radius':10,'heightParameter':ids['Height'],'units':'mm','name':'Printable stock'})
    cylinder=inspect()['authored']['features'][0];body=inspect()['evaluation']['bodies'][0]['bodyId']
    edit=call('feature.editDialog',{'featureId':cylinder['id']});assert edit['inputs']['heightParameter']=='Height'
    call('command.commit',{'session':edit['id']});assert inspect()['authored']['features'][0]['parameters']['heightParameterId']==ids['Height']
    call('sketch.begin',{'plane':'XY','units':'mm','name':'Angular wedge cutter'})
    for coords in [(0,0,30,0),(0,0,30*math.cos(math.pi/6),15),(30,0,30*math.cos(math.pi/6),15)]:
        call('sketch.line',dict(zip(('x1','y1','x2','y2'),coords),units='mm'))
    sk=draft();sid=sk['id'];base,side,closing=sk['parameters']['entities']
    # Coincident intent follows the independently authored endpoint pairs.
    for x,y in [(base['startPointId'],side['startPointId']),(base['endPointId'],closing['startPointId']),(side['endPointId'],closing['endPointId'])]:
        if x!=y:call('sketch.constraint',{'type':'coincident','pointIds':[x,y]})
    call('sketch.constraint',{'type':'fixed','pointIds':[base['startPointId']]})
    call('sketch.constraint',{'type':'horizontal','entityIds':[base['id']]})
    for e in [base,side]:call('sketch.dimension',{'kind':'linear','entityId':e['id'],'value':30})
    call('sketch.dimension',{'kind':'angle','entityIds':[base['id'],side['id']],'raySigns':[1,1],'parameterId':ids['Opening'],'annotation':{'uMm':12,'vMm':4}})
    did=draft()['parameters']['dimensions'][-1]['id'];bound=draft()
    call('sketch.dimensionUpdate',{'dimensionId':did,'parameterName':'Height'},error='unknown_parameter');assert draft()==bound
    call('sketch.dimensionEdit',{'dimensionId':did});assert call('app.state')['activeSession']['pendingDimension']['parameterId']==ids['Opening']
    call('sketch.dimensionAccept',{'text':'Opening'});assert draft()['parameters']['dimensions'][-1]['parameterId']==ids['Opening']
    call('sketch.finish');profile=inspect()['evaluation']['sketches'][0]['profiles'][0]['profileId']
    call('solid.extrude',{'sketchId':sid,'profileId':profile,'distance':30,'operation':'cut','targetBodyIds':[body]})
    initial=volume(30,10)
    call('document.parameterUpdate',{'parameterId':ids['Opening'],'value':60,'name':'CutAngle'});volume(60,10)
    call('document.parameterUpdate',{'parameterId':ids['Height'],'value':15,'name':'StockHeight'});changed=volume(60,15)
    call('history.undo');volume(60,10);call('history.redo');assert inspect()['authored']==changed['authored'];changed=inspect()
    at={'featureId':sid,'dimensionId':did};ht={'featureId':cylinder['id'],'field':'heightMm'}
    for target,expr in [(at,'StockHeight'),(ht,'CutAngle')]:
        call('document.modelParameterSet',{'target':target,'expression':expr},error='invalid_model_expression');assert inspect()['authored']==changed['authored'] and inspect()['revision']==changed['revision']
    call('solid.cylinder',{'featureId':cylinder['id'],'radius':10,'heightParameter':'CutAngle'},error='command_failed');assert inspect()['authored']==changed['authored']
    call('sketch.edit',{'sketchId':sid});call('sketch.dimensionEdit',{'dimensionId':did});call('sketch.dimensionAccept',{'text':'CutAngle'});call('sketch.finish')
    call('document.parametersDialog');call('capture.screenshot',{'path':str(out/'typed-parameter-table.png')});call('document.parametersDialog',{'visible':False})
    call('view.home');call('view.orientation',{'name':'isometric'});call('capture.screenshot',{'path':str(out/'wedge-cylinder.png')})
    retained=inspect();save=out/'typed-wedge.alloy';call('document.save',{'path':str(save)});call('document.open',{'path':str(save),'discard':True});reopened=volume(60,15);assert reopened['authored']==retained['authored']
    path=out/'typed-wedge.stl';export=call('document.exportStl',{'path':str(path),'bodyId':body,'units':'mm','format':'binary','surfaceDeviationMm':0.01,'normalDeviationDeg':5});mesh=inspect_stl(path,True)
    assert len(mesh['components'])==1 and mesh['eulerCharacteristic']==2
    assert math.isclose(mesh['signedVolume'],reopened['evaluation']['bodies'][0]['volumeMm3'],rel_tol=0.003)
    assert math.isclose(mesh['bounds'][1][2],15,abs_tol=1e-5)
    call('document.modelParameterSet',{'target':at,'expression':'60 deg'});call('history.undo');assert inspect()['authored']==retained['authored'];call('history.redo')
    call('document.modelParameterSet',{'target':ht,'expression':'15 mm'})
    call('document.parameterUpdate',{'parameterId':ids['Opening'],'value':45});call('document.parameterUpdate',{'parameterId':ids['Height'],'value':20});unbound=volume(60,15)
    assert 'parameterId' not in unbound['authored']['features'][1]['parameters']['dimensions'][-1]
    assert 'heightParameterId' not in unbound['authored']['features'][0]['parameters']
    (out/'geometry.json').write_text(json.dumps({'initial':initial,'changed':changed,'reopened':reopened,'unbound':unbound,'export':export,'mesh':mesh},indent=2))
    (out/'state.json').write_text(json.dumps(call('app.state'),indent=2));(out/'transcript.json').write_text(json.dumps(log,indent=2));(out/'report.json').write_text(json.dumps({'passed':True,'calls':len(log),'angleId':ids['Opening'],'heightId':ids['Height'],'mesh':mesh},indent=2))
