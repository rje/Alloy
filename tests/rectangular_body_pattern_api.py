#!/usr/bin/env python3
"""Bounded retained body-pattern API fixture; native input and GPU parity are separate."""
import argparse,json,math,uuid
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,log):
    call('solid.box',{'width':12,'depth':3,'height':7,'units':'mm','name':'Seed'})
    initial=call('document.inspect');seed=initial['evaluation']['bodies'][0];source=seed['bodyId']
    def axis(coordinate,quantity,spacing):
        for edge in seed['solidEdges']:
            if not edge['available']:continue
            v=[b-a for a,b in zip(edge['startMm'],edge['endMm'])]
            if abs(v[coordinate])>1 and math.isclose(sum(x*x for x in v),v[coordinate]**2,abs_tol=1e-8):return {'edgeId':edge['edgeId'],'quantity':quantity,'spacingMm':spacing}
        raise AssertionError('No retained straight axis')
    row=[axis(0,3,20)];args={'sourceBodyId':source,'axes':row}
    session=call('command.begin',{'command':'solid.rectangularPattern','inputs':args});sid=session['id']
    preview=call('command.preview',{'session':sid});assert call('app.state')['selection']['kind']=='bodyPattern';assert len(preview['geometry']['occurrences'])==3
    assert call('document.inspect')['authored']==initial['authored'];call('command.cancel',{'session':sid});assert call('document.inspect')['authored']==initial['authored']
    call('solid.rectangularPattern',args);pattern=call('document.inspect');feature=pattern['authored']['features'][-1];assert feature['type']=='rectangularBodyPattern'
    assert len(pattern['evaluation']['bodies'])==3 and pattern['evaluation']['bodies'][0]==seed
    oldids=[b['bodyId'] for b in pattern['evaluation']['bodies']]
    grid=[axis(0,4,25),axis(1,2,15)]
    edit=call('feature.editDialog',{'featureId':feature['id']});assert edit['inputs']['sourceBodyId']==source
    call('command.update',{'session':edit['id'],'inputs':{'axes':grid}});preview=call('command.preview',{'session':edit['id']});assert len(preview['geometry']['occurrences'])==8
    call('command.commit',{'session':edit['id']});state=call('document.inspect');bs=state['evaluation']['bodies'];assert len(bs)==8 and [b['bodyId'] for b in bs[:3]]==oldids and bs[0]==seed
    for b in bs:
        assert b['valid'] and b['closed'] and math.isclose(b['volumeMm3'],252,abs_tol=1e-6)
        if b['bodyId']==source:continue
        t=b['rectangularBodyPattern']['translationMm'];low=b['boundsMm']['min'];orig=seed['boundsMm']['min'];assert all(math.isclose(x,y+z,abs_tol=1e-6) for x,y,z in zip(low,orig,t))
    call('selection.set',{'bodyId':bs[2]['bodyId']});assert call('app.state')['selection']['bodyId']==bs[2]['bodyId']
    call('history.undo');assert call('document.inspect')['authored']==pattern['authored'];call('history.redo');assert call('document.inspect')['authored']==state['authored']
    for bad in [0,-1]:
        call('solid.rectangularPattern',{**args,'featureId':feature['id'],'axes':[dict(row[0],spacingMm=bad)]},error='invalid_arguments');assert call('document.inspect')['authored']==state['authored']
    call('solid.rectangularPattern',{**args,'featureId':feature['id'],'axes':[dict(row[0],quantity=2.5)]},error='invalid_arguments')
    call('solid.rectangularPattern',{**args,'featureId':feature['id'],'axes':[grid[0],grid[0]]},error='evaluation_failed')
    call('solid.rectangularPattern',{**args,'featureId':feature['id'],'axes':[dict(row[0],edgeId=str(uuid.uuid4()))]},error='evaluation_failed');assert call('document.inspect')['authored']==state['authored']
    call('solid.box',{'featureId':initial['authored']['features'][0]['id'],'width':12,'depth':3,'height':9,'units':'mm'});edited=call('document.inspect');assert all(math.isclose(b['volumeMm3'],324,abs_tol=1e-6) for b in edited['evaluation']['bodies'])
    path=out/'rectangular-bodies.alloy';call('document.save',{'path':str(path)});call('document.new',{'discard':True});call('document.open',{'path':str(path)});reopened=call('document.inspect');assert reopened['authored']==edited['authored'] and reopened['evaluation']['bodies']==edited['evaluation']['bodies']
    stl=out/'rectangular-bodies.stl';call('document.exportStl',{'path':str(stl),'units':'mm','format':'binary','surfaceDeviationMm':0.05,'normalDeviationDeg':5,'allVisible':True})
    mesh=inspect_stl(stl,True);assert len(mesh['components'])==8 and all(math.isclose(b['signedVolume'],324,abs_tol=1e-4) for b in mesh['components'])
    actual=sorted(tuple(round(v,5) for v in b['bounds'][0]) for b in mesh['components']);expected=sorted(tuple(round(v,5) for v in b['boundsMm']['min']) for b in reopened['evaluation']['bodies']);assert actual==expected
    call('document.new',{'discard':True});call('sketch.begin',{'plane':'XY'})
    for coords in [(0,0,10,0),(10,0,4,6),(4,6,0,0)]:call('sketch.line',dict(zip(('x1','y1','x2','y2'),coords),units='mm'))
    call('sketch.finish');sketch=call('document.inspect')['evaluation']['sketches'][0];call('solid.extrude',{'sketchId':sketch['sketchId'],'profileId':sketch['profiles'][0]['profileId'],'distance':5,'units':'mm'})
    tri=call('document.inspect')['evaluation']['bodies'][0];directions=[]
    for edge in tri['solidEdges']:
        if not edge['available']:continue
        v=[b-a for a,b in zip(edge['startMm'],edge['endMm'])];length=math.sqrt(sum(x*x for x in v))
        if abs(v[2])>1e-6 or length<1e-6:continue
        direction=[x/length for x in v]
        if not directions:directions.append((edge['edgeId'],direction))
        elif 1e-4<abs(sum(x*y for x,y in zip(direction,directions[0][1])))<1-1e-4:directions.append((edge['edgeId'],direction));break
    assert len(directions)==2
    call('solid.rectangularPattern',{'sourceBodyId':tri['bodyId'],'axes':[{'edgeId':e,'quantity':2,'spacingMm':s} for (e,d),s in zip(directions,[20,30])]})
    oblique=call('document.inspect');assert len(oblique['evaluation']['bodies'])==4
    wanted=[20*x+30*y for x,y in zip(directions[0][1],directions[1][1])];last=oblique['evaluation']['bodies'][3];assert all(math.isclose(a,b+t,abs_tol=1e-6) for a,b,t in zip(last['boundsMm']['min'],tri['boundsMm']['min'],wanted))
    call('document.new',{'discard':True});call('solid.cylinder',{'radius':3,'height':5,'units':'mm'});curved=call('document.inspect');cylinder=curved['evaluation']['bodies'][0];rim=next(e for e in cylinder['solidEdges'] if e['available'])
    call('solid.rectangularPattern',{'sourceBodyId':cylinder['bodyId'],'axes':[{'edgeId':rim['edgeId'],'quantity':2,'spacingMm':10}]},error='evaluation_failed');assert call('document.inspect')['authored']==curved['authored']
    (out/'receipt.json').write_text(json.dumps({'passed':True,'initial':initial,'preview':preview,'grid':state,'edited':edited,'reopened':reopened,'stl':mesh,'oblique':oblique,'transcript':log},indent=2)+'\n')
print('PASS retained body pattern: row/grid, preview/cancel/edit, independent selection, coordinate identities, source recompute, refusals, undo/reopen')
