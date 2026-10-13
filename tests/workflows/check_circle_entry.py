#!/usr/bin/env python3
"""Command-first circle, pending support, typed diameter and cancellation through live Alloy."""
import argparse,json,sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'tools'))
from alloyctl import choose_endpoint,request
p=argparse.ArgumentParser();p.add_argument('--ready-file',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True);ep=choose_endpoint(ready_file=a.ready_file);log=[]
def call(c,args=None):
 r=request(ep,c,args);log.append({'command':c,'args':args or {},'response':r});assert r['ok'],r;return r['result']
def circle(d,support=None):
 pending=call('sketch.circle');assert pending['command']=='sketch.plane' and pending['pendingTool']=='circle'
 started=call('sketch.begin',{'support':support} if support else {'plane':'XY'});assert started['tool']=='circle' and 'pendingTool' not in started
 sketch=started['draftSketch']['id'];point=call('view.projectSketchPoint',{'sketchId':sketch,'uMm':0,'vMm':0});call('sketch.placePoint',{k:point[k] for k in ('x','y')})
 call('sketch.circleDiameterPreview',{'text':str(d)});call('sketch.circleDiameterAccept',{'text':str(d)})
 call('capture.screenshot',{'path':str(out/f'circle-{d}.png')});call('sketch.finish');doc=call('document.inspect');s=doc['evaluation']['sketches'][-1];return s['sketchId'],s['profiles'][0]['profileId']
result={'status':'failed','transcript':log}
try:
 call('document.new',{'discard':True});before=call('document.inspect')['authored'];pending=call('sketch.circle');call('sketch.cancel');assert call('document.inspect')['authored']==before
 s,pid=circle(26);call('solid.extrude',{'sketchId':s,'profileId':pid,'distance':3});doc=call('document.inspect');cap=doc['evaluation']['bodies'][0]['supportFaces'][0];support={k:cap[k] for k in ('producerFeatureId','bodyId','faceId')}
 s,pid=circle(19,support);call('solid.extrude',{'sketchId':s,'profileId':pid,'distance':8,'operation':'join'});doc=call('document.inspect');cap=doc['evaluation']['bodies'][0]['supportFaces'][0];support={k:cap[k] for k in ('producerFeatureId','bodyId','faceId')};third,pid=circle(6.2,support)
 stable=call('document.inspect')['authored'];call('sketch.circle');call('sketch.begin',{'plane':'XY'});call('sketch.cancel');assert call('document.inspect')['authored']==stable
 call('document.save',{'path':str(out/'circle-entry.alloy')});call('document.new',{'discard':True});call('document.open',{'path':str(out/'circle-entry.alloy'),'discard':True});assert call('document.inspect')['authored']==stable
 result['status']='passed'
finally:(out/'circle-entry.json').write_text(json.dumps(result,indent=2)+'\n')
