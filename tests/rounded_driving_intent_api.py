#!/usr/bin/env python3
"""Focused real-app driving rounded rectangle edit and presentation receipt."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--app',required=True);p.add_argument('--output',required=True)
a=p.parse_args();out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
with application(a.app,out) as (call,log):
    def state():return call('app.state')
    def draft():return state()['activeSession']['draftSketch']
    def inspect():return call('document.inspect')
    def pixel(u,v):
        q=call('view.projectSketchPoint',{'sketchId':draft()['id'],'uMm':u,'vMm':v})
        return {k:q[k] for k in ('x','y')}
    def capture(name):call('capture.screenshot',{'path':str(out/(name+'.png'))})
    def dims(sketch):return {d['id']:d for d in sketch['parameters']['dimensions']}
    try:
        call('window.resize',{'width':1400,'height':1000})
        call('sketch.twoPointRectangle')
        call('sketch.begin',{'plane':'XY','units':'mm','name':'Driving rounded source'})
        call('sketch.lookAt')
        call('sketch.placePoint',pixel(0,0))
        call('sketch.pointerMove',pixel(20,-10))
        call('sketch.twoPointRectanglePreview',{'widthText':'60','heightText':'30'})
        call('sketch.twoPointRectangleAccept')
        call('view.zoom',{'factor':.7})
        center=pixel(30,-15);viewport=state()['viewport']
        call('view.pan',{'x':viewport['width']/2-center['x'],
                         'y':viewport['height']/2-center['y']})
        source=draft();lines=[e['id'] for e in source['parameters']['entities']]
        width,height=[d['id'] for d in source['parameters']['dimensions']]
        for i in range(4):
            call('sketch.fillet',{'corners':[{'entityIds':[lines[i],lines[(i+1)%4]]}],
                                  'value':6,'units':'mm'})
        rounded=draft();assert rounded['parameters']['roundedRectangleIntent']['sideIds']==lines
        assert [dims(rounded)[v]['valueMm'] for v in (width,height)]==[60,30]
        assert all(dims(rounded)[v]['driving'] for v in (width,height))
        assert sum(c['type']=='supportLine' for c in rounded['parameters']['constraints'])==2
        radius=next(d['id'] for d in reversed(rounded['parameters']['dimensions']) if d['kind']=='radius')
        call('sketch.dimensionEdit',{'dimensionId':width})
        assert state()['inlineEditor']['visible'] and not state()['inlineEditor']['readOnly']
        capture('driving-width-editor')
        call('sketch.dimensionCancel')
        assert draft()==rounded
        call('sketch.dimensionUpdate',{'dimensionId':radius,'value':5.5,'units':'mm'})
        call('sketch.dimensionUpdate',{'dimensionId':width,'value':70,'units':'mm'})
        call('sketch.dimensionUpdate',{'dimensionId':height,'value':35,'units':'mm'})
        resized=draft()
        assert [dims(resized)[v]['valueMm'] for v in (width,height)]==[70,35]
        bad=call('sketch.dimensionUpdate',{'dimensionId':radius,'value':30,'units':'mm'},'sketch_solve_failed')
        assert draft()==resized
        capture('driving-rounded-edited')
        call('sketch.finish')
        profile=inspect()['evaluation']['sketches'][0]['profiles'][0]
        area=70*35-(1-math.pi/4)*(3*36+30.25)
        assert math.isclose(profile['areaMm2'],area,abs_tol=1e-4)
        call('solid.extrude',{'sketchId':resized['id'],'profileId':profile['profileId'],
                              'distance':2,'units':'mm'})
        assert math.isclose(inspect()['evaluation']['bodies'][0]['volumeMm3'],2*area,abs_tol=1e-4)
        call('view.home');capture('extruded-rounded-edited')
        saved=inspect()['authored'];path=out/'driving-rounded.alloy'
        call('document.save',{'path':str(path)})
        call('document.open',{'path':str(path)})
        assert inspect()['authored']==saved
        gpu=state()['viewport']['gpu']
        report={'passed':True,'widthId':width,'heightId':height,'radiusId':radius,
                'profileId':profile['profileId'],'areaMm2':area,
                'gpu':{key:gpu.get(key) for key in ('hardwareAccelerated','vendor','renderer','version','backend')}}
        (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    finally:
        (out/'transcript.json').write_text(json.dumps(log,indent=2)+'\n')
