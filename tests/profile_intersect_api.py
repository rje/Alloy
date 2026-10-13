#!/usr/bin/env python3
"""Retained Extrude Intersect through the real app; native menu input is separate."""
import argparse,json,math
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,transcript):
 def inspect():return call('document.inspect')
 def dialog(inputs):return call('command.begin',{'command':'solid.extrude','inputs':inputs})
 def rectangle(x1,x2):
  call('sketch.create',{'plane':'XY'});call('sketch.rectangle',{'x1':x1,'y1':0,'x2':x2,'y2':20,'units':'mm'});call('sketch.finish');return inspect()['evaluation']['sketches'][-1]
 base=rectangle(0,20);call('solid.extrude',{'sketchId':base['sketchId'],'profileId':base['profiles'][0]['profileId'],'distance':10,'units':'mm'});stock=inspect()['evaluation']['bodies'][0]
 s=rectangle(5,25);args={'sketchId':s['sketchId'],'profileId':s['profiles'][0]['profileId'],'operation':'intersect','distance':6/25.4,'units':'in'};before=inspect();session=dialog(args);preview=call('command.preview',{'session':session['id']});assert preview['inputs']['targetBodyIds']==[stock['bodyId']];assert math.isclose(preview['geometry']['volumeMm3'],1800,abs_tol=1e-5);assert preview['geometry']['operation']=='intersect'
 call('view.home');call('capture.screenshot',{'path':str(out/'intersect-preview.png')});call('command.cancel',{'session':session['id']});assert inspect()['authored']==before['authored'] and inspect()['undo']==before['undo']
 session=dialog(args);call('command.commit',{'session':session['id']});full=inspect();feature=full['authored']['features'][-1];body=full['evaluation']['bodies'][0];assert not 'bodyId' in feature and body['bodyId']==stock['bodyId'] and body['featureId']==stock['featureId'];assert len(full['authored']['features'])==len(before['authored']['features'])+1 and len(full['evaluation']['bodies'])==1
 edit=call('feature.editDialog',{'featureId':feature['id']});call('command.update',{'session':edit['id'],'inputs':{'distance':8/25.4,'units':'in'}});call('command.preview',{'session':edit['id']});call('command.commit',{'session':edit['id']});edited=inspect();assert math.isclose(edited['evaluation']['bodies'][0]['volumeMm3'],2400,abs_tol=1e-5)
 call('history.undo');assert inspect()['authored']==full['authored'];call('history.redo');assert inspect()['authored']==edited['authored']
 edit=call('feature.editDialog',{'featureId':feature['id']});call('command.update',{'session':edit['id'],'inputs':{'distance':-6/25.4,'units':'in'}});call('command.preview',{'session':edit['id']},error='evaluation_failed');call('command.commit',{'session':edit['id']},error='evaluation_failed');assert inspect()['authored']==edited['authored'];call('command.cancel',{'session':edit['id']})
 path=out/'retained-intersect.alloy';call('document.save',{'path':str(path)});call('document.open',{'path':str(path)});assert inspect()['authored']==edited['authored'];call('view.home');call('capture.screenshot',{'path':str(out/'retained-intersect.png')})
 # Multiple eligible targets must not guess; explicit selection stays supported.
 call('solid.box',{'width':20,'depth':20,'height':10,'x':10,'units':'mm'});stable=inspect();call('solid.extrude',args,error='cut_targets_ambiguous');assert inspect()['authored']==stable['authored']
 # Compound selected prisms may produce one valid Common result; preview must match.
 call('sketch.create',{'plane':'XY'})
 for x1,x2 in [(6,8),(50,52)]:call('sketch.rectangle',{'x1':x1,'y1':2,'x2':x2,'y2':4,'units':'mm'})
 call('sketch.finish');multi=inspect()['evaluation']['sketches'][-1];stable=inspect();session=dialog({'sketchId':multi['sketchId'],'profileIds':[q['profileId'] for q in multi['profiles']],'distance':5,'units':'mm','operation':'intersect','targetBodyIds':[stock['bodyId']]});compound_preview=call('command.preview',{'session':session['id']});assert math.isclose(compound_preview['geometry']['volumeMm3'],20,abs_tol=1e-5);assert len(compound_preview['inputs']['profileIds'])==2;call('command.cancel',{'session':session['id']});assert inspect()['authored']==stable['authored']
 (out/'report.json').write_text(json.dumps({'passed':True,'scope':'shared API and real OCCT overlap; native Qt menu proof separate','preview':preview,'compoundPreview':compound_preview,'final':edited,'transcript':transcript},indent=2))
