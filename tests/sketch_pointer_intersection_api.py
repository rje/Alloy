#!/usr/bin/env python3
"""Retained finite line-pair endpoint, edited profile/solid, history and storage."""
import argparse, copy, json, math
from pathlib import Path
from app_harness import application

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--app',required=True);parser.add_argument('--output',required=True);a=parser.parse_args();out=Path(a.output).resolve()
    with application(a.app,out) as (call,transcript):
        state=call('app.state');assert state['viewport']['gpu']['hardwareAccelerated']
        session=call('sketch.begin',{'plane':'XY','name':'Retained intersection plate','units':'mm'})
        def preview():return call('command.preview',{'session':session['id']})
        def params():return preview()['authoredDraft']['parameters']
        def pixel(u,v):
            p=call('view.projectSketchPoint',{'plane':'XY','uMm':u,'vMm':v});return {'x':p['x'],'y':p['y']}
        sources=[]
        for x1,y1,x2,y2 in [(-30,10,40,10),(10,-20,10,40)]:
            call('sketch.line',dict(x1=x1,y1=y1,x2=x2,y2=y2,units='mm'));e=params()['entities'][-1];sources.append(e)
            call('sketch.construction',{'entityIds':[e['id']],'construction':True})
            call('sketch.constraint',{'type':'fixed','entityIds':[e['id']]})
        corners=[]
        for x,y in [(-10,-10),(30,-10)]:
            call('sketch.addPoint',{'x':x,'y':y,'units':'mm'});e=params()['entities'][-1];corners.append(e['pointId'])
            call('sketch.constraint',{'type':'fixed','pointIds':[e['pointId']]})
        call('sketch.lookAt');call('view.zoom',{'factor':0.75});call('sketch.tool',{'tool':'line'})
        base=preview()['authoredDraft']
        call('sketch.displayOptions',{'showConstructionGeometries':False})
        hidden=call('sketch.pointerMove',pixel(10,10));assert hidden['snap'].get('target',{}).get('kind')!='intersection'
        call('sketch.displayOptions',{'showConstructionGeometries':True})
        call('sketch.line',dict(x1=-20,y1=-20,x2=40,y2=40,units='mm'))
        ambiguous=call('sketch.pointerMove',pixel(10,10));assert ambiguous['snap']['status']=='ambiguous',ambiguous
        call('sketch.tool',{'tool':'select'});call('history.undo');assert preview()['authoredDraft']==base;call('sketch.tool',{'tool':'line'})
        before=preview()['authoredDraft'];call('sketch.placePoint',pixel(-10,-10));hover=call('sketch.pointerMove',pixel(10,10))
        assert hover['snap']['status']=='acquired' and hover['snap']['target']=={'kind':'intersection','entityIds':[s['id'] for s in sources]},hover
        assert preview()['authoredDraft']==before
        cue=call('app.state')['viewport']['gpu']['sketchSnapCue'];assert cue['visible']
        call('capture.screenshot',{'path':str(out/'intersection-candidate.png')})
        call('sketch.tool',{'tool':'select'});assert preview()['authoredDraft']==before
        call('sketch.tool',{'tool':'line'});call('sketch.placePoint',pixel(-10,-10));call('sketch.placePoint',pixel(10,10))
        call('sketch.placePoint',pixel(30,-10));call('sketch.placePoint',{**pixel(-10,-10),'suppressInference':True});call('sketch.tool',{'tool':'select'})
        p=params();relations=[c for c in p['constraints'] if c['type']=='intersection'];assert len(relations)==1;relation=relations[0];endpoint=relation['pointIds'][0]
        def endpoint_xy(p):return next((pt['uMm'],pt['vMm']) for pt in p['points'] if pt['id']==endpoint)
        assert math.dist(endpoint_xy(p),(10,10))<1e-6
        glyphs=call('app.state')['viewport']['gpu']['sketchConstraintGlyphs'];assert any(g['kind']=='intersection' and g['constraintId']==relation['id'] for g in glyphs),glyphs
        call('capture.screenshot',{'path':str(out/'retained-intersection-profile.png')})
        info=preview();profile=next(r for r in info['sketchGeometry']['profiles'] if r['valid']);assert math.isclose(profile['areaMm2'],400,abs_tol=1e-5)
        sketch_id=info['authoredDraft']['id'];call('sketch.finish')
        extrude=call('command.begin',{'command':'solid.extrude','inputs':{'sketchId':sketch_id,'profileId':profile['profileId'],'distance':5,'units':'mm'}})
        call('command.preview',{'session':extrude['id']});call('command.commit',{'session':extrude['id']});original=call('document.inspect');assert math.isclose(original['evaluation']['bodies'][0]['volumeMm3'],2000,abs_tol=1e-5)
        baseline=copy.deepcopy(p)
        def edit_sources(horizontal_y,vertical_x):
            changed=copy.deepcopy(baseline)
            for c in changed['constraints']:
                if c['type']=='fixedLine':
                    for pt in c['coordinates']:pt['vMm' if c['entityIds']==[sources[0]['id']] else 'uMm']=horizontal_y if c['entityIds']==[sources[0]['id']] else vertical_x
            return call('feature.edit',{'featureId':sketch_id,'parameters':{'constraints':changed['constraints']}})
        edit_sources(15,10);first_edit=call('document.inspect');assert math.isclose(first_edit['evaluation']['bodies'][0]['volumeMm3'],2500,abs_tol=1e-5)
        edit_sources(15,14);edited=call('document.inspect');assert math.isclose(edited['evaluation']['bodies'][0]['volumeMm3'],2500,abs_tol=1e-5)
        stored=next(f for f in edited['authored']['features'] if f['id']==sketch_id)['parameters'];assert math.dist(endpoint_xy(stored),(14,15))<1e-6
        assert next(c for c in stored['constraints'] if c['id']==relation['id'])==relation
        # Force source span away from the crossing; evaluation must refuse atomically.
        invalid=copy.deepcopy(stored['constraints'])
        for c in invalid:
            if c['type']=='fixedLine' and c['entityIds']==[sources[1]['id']]:
                c['coordinates'][0]['vMm']=20;c['coordinates'][1]['vMm']=40
        call('feature.edit',{'featureId':sketch_id,'parameters':{'constraints':invalid}},'evaluation_failed');assert call('document.inspect')['authored']==edited['authored']
        parallel=copy.deepcopy(stored['constraints'])
        for c in parallel:
            if c['type']=='fixedLine' and c['entityIds']==[sources[1]['id']]:c['coordinates']=[{'uMm':10,'vMm':20},{'uMm':30,'vMm':20}]
        call('feature.edit',{'featureId':sketch_id,'parameters':{'constraints':parallel}},'evaluation_failed');assert call('document.inspect')['authored']==edited['authored']
        call('history.undo');assert call('document.inspect')['authored']==first_edit['authored'];call('history.redo');assert call('document.inspect')['authored']==edited['authored']
        path=str(out/'intersection-plate.alloy');call('document.save',{'path':path});call('document.new',{'discard':False});call('document.open',{'path':path});assert call('document.inspect')['authored']==edited['authored']
        call('view.home');call('capture.screenshot',{'path':str(out/'intersection-plate-reopened.png')})
    (out/'receipt.json').write_text(json.dumps({'result':'passed','initialState':state,'sources':sources,'intersection':relation,'transcript':transcript},indent=2)+'\n')
    print('PASS: retained finite intersection, both source edits, extrusion, refusal, history and reopen')
if __name__=='__main__':main()
