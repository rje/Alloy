#!/usr/bin/env python3
"""Bounded post-Trim straw-support geometry fixture through the real Alloy API.

This authors equivalent post-Trim geometry to qualify SKT-027 partitions. It
never claims to execute the separate SKT-029 Trim command or the full tutorial.
"""
import argparse,json,math,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools'))
from alloyctl import choose_endpoint,request


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--ready-file',required=True);p.add_argument('--output',required=True);a=p.parse_args()
    out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True);endpoint=choose_endpoint(ready_file=a.ready_file);transcript=[]
    def call(command,args=None,ok=True):
        response=request(endpoint,command,args,timeout=30);transcript.append({'command':command,'args':args or {},'response':response})
        assert response['ok']==ok,response
        return response['result'] if ok else response
    def inspect():return call('document.inspect')
    def snap(name):return call('capture.screenshot',{'path':str(out/name)})
    result={'status':'failed','fixtureKind':'Explicitly authored post-Trim equivalent, not Trim/native tutorial evidence','transcript':transcript}
    try:
        call('document.new',{'discard':True});call('sketch.begin',{'plane':'XY','name':'Post-Trim support fixture'})
        right=7.5+math.sqrt(31.25);left=15-right
        for x1,y1,x2,y2 in [(0,0,left,0),(0,-10,left,-10),(right,0,32,0),(32,0,32,-10),(32,-10,right,-10)]:
            call('sketch.line',{'x1':x1,'y1':y1,'x2':x2,'y2':y2,'units':'mm'})
        circle=call('sketch.addCircle',{'centerX':7.5,'centerY':-5,'diameter':15})['draftSketch']['parameters']['entities'][-1]
        call('sketch.constraint',{'type':'fixed','pointIds':[circle['centerPointId']]})
        call('sketch.dimension',{'kind':'diameter','entityId':circle['id'],'value':15})
        for x in [23,26,29]:call('sketch.rectangle',{'x1':x,'y1':0,'x2':x+3,'y2':-10})
        call('sketch.finish');doc=inspect();sketch=doc['evaluation']['sketches'][0];profiles=sketch['profiles'];assert len(profiles)==5
        sketch_id=sketch['sketchId'];diameter=doc['authored']['features'][0]['parameters']['dimensions'][0]['id']
        band_half=5*math.sqrt(31.25)+56.25*math.asin(2/3)
        assert math.isclose(sum(p['areaMm2'] for p in profiles),math.pi*56.25+155-band_half+90,abs_tol=1e-5)
        assert any(len(e['sourceEntityIds'])>1 for p in profiles for e in p['edgeProvenance'])
        call('view.home');call('view.orientation',{'name':'top'})
        selected={}
        for name,x in [('disk',7.5),('bridge',20),('inner',24.5),('middle',27.5),('outer',30.5)]:
            point=call('view.projectSketchPoint',{'sketchId':sketch_id,'uMm':x,'vMm':-5})
            picked=call('selection.pick',{k:point[k] for k in ('x','y')});assert picked['kind']=='profile'
            selected[name]=picked['profileId']
            if name=='disk':snap('disk-region.png')
        assert len(set(selected.values()))==5
        call('document.save',{'path':str(out/'post-trim-support.alloy')})
        call('solid.extrude',{'sketchId':sketch_id,'profileId':selected['disk'],'distance':20})
        call('feature.visibility',{'featureId':sketch_id,'visible':True})
        for index,name in enumerate(['bridge','middle']):
            point=call('view.projectSketchPoint',{'sketchId':sketch_id,'uMm':20 if name=='bridge' else 27.5,'vMm':-5})
            picked=call('selection.pick',{**{k:point[k] for k in ('x','y')},'additive':bool(index)})
        assert set(picked['profileIds'])=={selected['bridge'],selected['middle']}
        snap('two-separated-side-regions.png')
        call('sketch.edit',{'sketchId':sketch_id});call('sketch.dimensionUpdate',{'dimensionId':diameter,'value':16});call('sketch.finish')
        assert math.isclose(inspect()['evaluation']['bodies'][0]['volumeMm3'],math.pi*64*20,abs_tol=1e-5)
        call('history.undo');call('solid.extrude',{'sketchId':sketch_id,'profileId':selected['bridge'],'distance':4});stable=inspect()['authored']
        call('sketch.edit',{'sketchId':sketch_id});call('sketch.dimensionUpdate',{'dimensionId':diameter,'value':16})
        refused=call('sketch.finish',ok=False);assert refused['error']['diagnostics']['code']=='profile_unavailable'
        call('sketch.cancel');assert inspect()['authored']==stable
        call('history.undo');call('history.redo');assert inspect()['authored']==stable
        call('document.save',{'path':str(out/'support-reference-check.alloy')});call('document.new',{'discard':True})
        call('document.open',{'path':str(out/'support-reference-check.alloy'),'discard':True});assert inspect()['authored']==stable
        result.update(status='passed',regionIds=selected,covers=['five actual post-Trim cells','coincident source sets','real viewport region picking','two separated selections','disk dimension edit','explicit changed-bridge reference refusal','exact history and save/reopen'])
    finally:(out/'overlap-regions.json').write_text(json.dumps(result,indent=2)+'\n')

if __name__=='__main__':main()
