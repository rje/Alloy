#!/usr/bin/env python3
"""Bounded remaining circle constructor Fix ownership through the real entrypoint."""
import argparse
import json
import math
import time
from pathlib import Path
from app_harness import application

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--app',required=True);p.add_argument('--output',required=True)
a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,ledger):
    def session(): return call('app.state')['activeSession']
    def draft(): return session()['draftSketch']
    def params(): return draft()['parameters']
    def geometry(cid):
        e=next(e for e in params()['entities'] if e['id']==cid)
        c=next(p for p in params()['points'] if p['id']==e['centerPointId'])
        return c['uMm'],c['vMm'],e['radiusMm']
    results=[]
    for mode in ['three-point','two-tangent','three-tangent','center-coincident','center-snap']:
        call('sketch.begin',{'plane':'XY','units':'mm','name':mode+' Fix'})
        call('sketch.rectangle',{'x1':0,'y1':0,'x2':30,'y2':30,'units':'mm'})
        lines=params()['entities'];bottom,right,top=lines[:3]
        call('sketch.constraint',{'type':'fixed','pointIds':[bottom['startPointId']]})
        for kind,e in [('horizontal',bottom),('vertical',right)]:
            call('sketch.dimension',{'kind':kind,'entityId':e['id'],'value':30,'units':'mm'})
        dims=params()['dimensions'];edit=dims[1]['id']
        call('sketch.construction',{'entityIds':[e['id'] for e in lines],'construction':True})
        if mode=='three-point':
            call('sketch.threePointCircle',{'x1':0,'y1':0,'x2':30,'y2':0,'x3':30,'y3':30,'units':'mm',
                **{key:{'kind':'point','pointId':point} for key,point in zip(['firstSnapTarget','secondSnapTarget','thirdSnapTarget'],[bottom['startPointId'],bottom['endPointId'],right['endPointId']])}})
            expected=(15,20,25)
        elif mode=='two-tangent':
            call('sketch.twoTangentCircle',{'lineIds':[bottom['id'],right['id']],'x':20,'y':10,'radius':10,'units':'mm'})
            edit=dims[0]['id'];expected=(30,10,10)
        elif mode=='three-tangent':
            call('sketch.threeTangentCircle',{'lineIds':[bottom['id'],right['id'],top['id']]})
            expected=(10,20,20)
        else:
            pos=32 if mode=='center-coincident' else 30
            call('sketch.addCircle',{'centerX':pos,'centerY':pos,'diameter':20,'units':'mm'})
            circle=params()['entities'][-1]
            if mode=='center-coincident':
                assert circle['centerPointId']!=right['endPointId']
                call('sketch.constraint',{'type':'coincident','pointIds':[circle['centerPointId'],right['endPointId']]})
            else: assert circle['centerPointId']==right['endPointId']
            call('sketch.dimension',{'kind':'radius','entityId':circle['id'],'value':10,'units':'mm'})
            expected=(30,40,10)
        circle=params()['entities'][-1];cid=circle['id']
        call('sketch.tool',{'tool':'select'});call('sketch.selectionSet');baseline=draft();initial=geometry(cid)
        call('sketch.fixToggle',{'entityIds':[cid]});fixed=draft();relation=params()['constraints'][-1]
        assert relation['type']=='fixedCircle' and relation['entityIds']==[cid]
        assert geometry(cid)==initial
        history=session()['draftHistory']
        call('sketch.dimensionUpdate',{'dimensionId':edit,'value':40,'units':'mm'},error='sketch_solve_failed')
        assert draft()==fixed and session()['draftHistory']==history
        call('sketch.constraint',{'type':'fixed','entityIds':[cid]});assert draft()==baseline
        call('history.undo');assert draft()==fixed
        call('history.redo');assert draft()==baseline
        call('sketch.dimensionUpdate',{'dimensionId':edit,'value':40,'units':'mm'})
        assert all(math.isclose(x,y,abs_tol=1e-5) for x,y in zip(geometry(cid),expected)),(mode,geometry(cid),expected)
        assert params()['constraints']==baseline['parameters']['constraints']
        call('sketch.fixToggle',{'entityIds':[cid]});final=draft();call('sketch.lookAt');time.sleep(.15)
        modeout=out/mode;modeout.mkdir();call('capture.screenshot',{'path':str(modeout/'fixed.png')})
        state=call('app.state');assert next(r['role'] for r in state['activeSession']['solverDiagnostics']['entityRoles'] if r['entityId']==cid)=='fixed'
        assert 'fixedCircle' not in {g['kind'] for g in state['viewport']['gpu']['sketchConstraintGlyphs']}
        call('sketch.finish');authored=call('document.inspect')['authored'];model=modeout/'fixed.alloy'
        call('document.save',{'path':str(model)});call('document.open',{'path':str(model)})
        assert call('document.inspect')['authored']==authored
        call('sketch.edit',{'sketchId':final['id']});assert draft()==final
        call('sketch.cancel');results.append({'mode':mode,'circleId':cid,'initial':initial,'editedExpected':expected,'final':final,'gpu':state['viewport']['gpu']})
    (out/'receipt.json').write_text(json.dumps({'result':'passed','modes':results,'transcript':ledger},indent=2)+'\n')
print('PASS five remaining circle Fix/UnFix source fixtures, independent geometry, atomic refusal, history/reopen and GPU')
