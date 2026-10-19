#!/usr/bin/env python3
"""Bounded DOC-039 real-entrypoint exact measurements and read-only checks."""
import argparse,json,math
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
def near(a,b): assert math.isclose(a,b,rel_tol=1e-8,abs_tol=1e-6),(a,b)
with application(a.app,out) as (call,transcript):
    call('solid.box',{'width':20,'depth':10,'height':5,'x':60,'units':'mm'})
    body=call('document.inspect')['evaluation']['bodies'][0]
    face=next(f for f in body['supportFaces'] if f['role']=='box.z1')
    planar={'kind':'face',**{k:face[k] for k in ('bodyId','faceId','producerFeatureId')}}
    call('solid.cylinder',{'radius':5,'height':12,'x':90,'units':'mm'})
    cylinder=call('document.inspect')['evaluation']['bodies'][-1]
    curved=next(f for f in cylinder['supportFaces'] if f['role']=='cylinder.lateral')
    lateral={'kind':'face',**{k:curved[k] for k in ('bodyId','faceId','producerFeatureId')}}
    call('sketch.begin',{'plane':'XY'})
    call('sketch.line',{'x1':0,'y1':0,'x2':3,'y2':4,'units':'mm'})
    call('sketch.addPoint',{'x':-3.3,'y':5.6,'units':'mm'})
    call('sketch.line',{'x1':40,'y1':0,'x2':45,'y2':0,'units':'mm'})
    call('sketch.line',{'x1':45,'y1':0,'x2':45,'y2':5,'units':'mm'})
    draft=call('app.state')['activeSession']['draftSketch'];sid=draft['id'];entities=draft['parameters']['entities']
    lines=[{'kind':'sketchEdge','sketchId':sid,'entityId':e['id']} for e in entities if e['type']=='line']
    point=next(e for e in entities if e['type']=='point');point={'kind':'sketchPoint','sketchId':sid,'pointId':point['pointId']}
    call('sketch.finish')
    before=call('document.inspect')
    def measured(sources):
        r=call('inspect.measureResult',{'sources':sources});assert r['sources']==sources and not r['authoredChange'];return {m['name']:m for m in r['measurements']}
    near(measured([lines[0]])['Length']['value'],5)
    near(measured([point,lines[0]])['Distance']['value'],6)
    near(measured(lines[1:])['Angle']['value'],90)
    near(measured([planar])['Area']['value'],200)
    near(measured([lateral])['Area']['value'],120*math.pi)
    for sources,code in [([], 'measure_selection_count'),([planar]*3,'measure_selection_count'),([planar,lines[0]],'measure_unsupported'),([{'kind':'edge','bodyId':'missing','edgeId':'missing'}],'measure_unavailable')]:call('inspect.measureResult',{'sources':sources},error=code)
    assert call('document.inspect')==before
    call('document.units',{'units':'in'});inch_before=call('document.inspect')
    length=measured([lines[0]])['Length'];near(length['value'],5/25.4);assert length['unit']=='in'
    area=measured([planar])['Area'];near(area['value'],200/25.4**2);assert area['unit']=='in²'
    assert call('document.inspect')==inch_before
    call('inspect.measure');call('inspect.measureClear');call('view.orientation',{'name':'top'});call('view.fit')
    call('inspect.measureFilter',{'filter':'Face'})
    face_pixel=call('view.projectSketchPoint',{'plane':'XY','uMm':70,'vMm':5})
    face_pick=call('selection.pick',{'x':face_pixel['x'],'y':face_pixel['y']})
    near(face_pick['measurements'][0]['value'],200/25.4**2)
    call('capture.screenshot',{'path':str(out/'measure-planar-inch.png')})
    call('inspect.measureClear');call('inspect.measureFilter',{'filter':'Point'})
    point_pixel=call('view.projectSketchPoint',{'sketchId':sid,'uMm':-3.3,'vMm':5.6})
    call('selection.pick',{'x':point_pixel['x'],'y':point_pixel['y']},error='measure_unsupported')
    call('inspect.measureFilter',{'filter':'Edge'})
    pixel=call('view.projectSketchPoint',{'sketchId':sid,'uMm':1.5,'vMm':2})
    pair_pick=call('selection.pick',{'x':pixel['x'],'y':pixel['y']})
    near(pair_pick['measurements'][0]['value'],6/25.4)
    call('capture.screenshot',{'path':str(out/'measure-point-line-inch.png')})
    call('inspect.measureClear')
    picked=call('selection.pick',{'x':pixel['x'],'y':pixel['y']})
    near(picked['measurements'][0]['value'],5/25.4)
    assert picked['sources'][0]['entityId']==lines[0]['entityId']
    call('capture.screenshot',{'path':str(out/'measure-inch.png')})
    assert call('document.inspect')==inch_before
    call('document.save',{'path':str(out/'measure-fixture.alloy')})
    measured([planar]);measured([point,lines[0]])
    call('document.save',{'path':str(out/'measure-fixture-after.alloy')})
    assert (out/'measure-fixture.alloy').read_bytes()==(out/'measure-fixture-after.alloy').read_bytes()
    gpu=call('app.state')['viewport']['gpu']
    (out/'receipt.json').write_text(json.dumps({'status':'passed','calls':len(transcript),'gpu':gpu,'sources':{'lines':lines,'point':point,'planar':planar,'cylinder':lateral},'transcript':transcript},indent=2)+'\n')
print('PASS bounded analytic Measure')
