#!/usr/bin/env python3
"""SOL-087 positive uniform origin Scale, exact lineage and independent STL readback."""
import argparse,json,math,uuid
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
def ref(f):return {k:f[k] for k in ('producerFeatureId','bodyId','faceId')}
def near(a,b,t=1e-5):assert math.isclose(a,b,abs_tol=t,rel_tol=1e-9),(a,b)
def verify(b,factor,offset=0):
 assert b['valid'] and b['closed'] and b['topology']['solids']==1
 near(b['volumeMm3'],4800*factor**3+400*factor**2*offset)
 for observed,expected in zip(b['boundsMm']['min'],[10*factor,-8*factor,4*factor]):near(observed,expected)
 for observed,expected in zip(b['boundsMm']['max'],[30*factor,12*factor,16*factor+offset]):near(observed,expected)
with application(a.app,out) as (call,log):
 call('solid.box',{'width':20,'depth':20,'height':12,'x':10,'y':-8,'z':4,'units':'mm','name':'ScaleFixture'})
 initial=call('document.inspect');stock=initial['evaluation']['bodies'][0];body=stock['bodyId'];args={'targetBodyId':body,'pivot':'documentOrigin','factor':1.5}
 session=call('command.begin',{'command':'solid.scale','inputs':{}});sid=session['id'];assert session['inputs']['factor']==1
 call('command.update',{'session':sid,'inputs':args});preview=call('command.preview',{'session':sid});assert preview['previewed'];verify(preview['geometry'],1.5)
 for sf,old in zip(preview['geometry']['supportFaces'],stock['supportFaces']):
  assert ref(sf)==ref(old)
  for key in ('normal','uAxis','vAxis'):assert sf['frame'][key]==old['frame'][key]
 assert call('document.inspect')['authored']==initial['authored'];call('command.cancel',{'session':sid});cancel=call('document.inspect');assert cancel['authored']==initial['authored'] and cancel['undo']==initial['undo'] and cancel['dirty']==initial['dirty']
 session=call('command.begin',{'command':'solid.scale','inputs':args});call('command.commit',{'session':session['id']});created=call('document.inspect');scaled=created['evaluation']['bodies'][0];verify(scaled,1.5);assert scaled['bodyId']==body;scaleid=created['authored']['features'][-1]['id'];assert created['authored']['features'][-1]['parameters']==args
 top=next(f for f in scaled['supportFaces'] if f['role']=='box.z1');call('solid.faceOffset',{'targetBodyId':body,'face':ref(top),'distanceMm':1});downstream=call('document.inspect');verify(downstream['evaluation']['bodies'][0],1.5,1)
 edit=call('feature.editDialog',{'featureId':scaleid});assert edit['inputs']['factor']==1.5
 call('command.update',{'session':edit['id'],'inputs':{'factor':2}});ep=call('command.preview',{'session':edit['id']});verify(ep['geometry'],2);assert call('document.inspect')['authored']==downstream['authored'];call('command.commit',{'session':edit['id']});edited=call('document.inspect');verify(edited['evaluation']['bodies'][0],2,1);assert edited['authored']['features'][2:]==downstream['authored']['features'][2:]
 call('history.undo');assert call('document.inspect')['authored']==downstream['authored'];call('history.redo');assert call('document.inspect')['authored']==edited['authored']
 # Direct edit shares the candidate and evaluator.
 call('solid.scale',{'featureId':scaleid,'factor':1.5});verify(call('document.inspect')['evaluation']['bodies'][0],1.5,1);call('history.undo');assert call('document.inspect')['authored']==edited['authored']
 for bad in (0,-1):
  call('solid.scale',{'featureId':scaleid,'factor':bad},error='invalid_scale');assert call('document.inspect')['authored']==edited['authored']
 call('solid.scale',{'targetBodyId':body,'factor':2},error='invalid_scale');call('solid.scale',{**args,'pivot':'centroid'},error='invalid_scale')
 missing={**args,'targetBodyId':str(uuid.uuid4())};call('solid.scale',missing,error='evaluation_failed');assert call('document.inspect')['authored']==edited['authored']
 # A real later retained operation fails after shrinking its upstream stock; edit refuses atomically.
 offsetid=edited['authored']['features'][-1]['id'];call('solid.faceOffset',{'featureId':offsetid,'distanceMm':-1});inward=call('document.inspect');verify(inward['evaluation']['bodies'][0],2,-1)
 call('solid.scale',{'featureId':scaleid,'factor':.01},error='evaluation_failed');failed=call('document.inspect');assert failed['authored']==inward['authored'] and failed['evaluation']['bodies']==inward['evaluation']['bodies'] and failed['undo']==inward['undo']
 badsession=call('command.begin',{'command':'solid.scale','inputs':{'featureId':scaleid}});call('command.update',{'session':badsession['id'],'inputs':{'factor':.01}})
 # Preview and commit both refuse the complete downstream graph before mutation.
 from alloyctl import request
 response=request(call.endpoint,'command.preview',{'session':badsession['id']},transport=call.transport);log.append({'command':'command.preview','args':{'session':badsession['id']},'response':response});assert not response['ok'],response
 response2=request(call.endpoint,'command.commit',{'session':badsession['id']},transport=call.transport);log.append({'command':'command.commit','args':{'session':badsession['id']},'response':response2});assert not response2['ok'],response2
 assert call('document.inspect')['authored']==inward['authored'];call('command.cancel',{'session':badsession['id']})
 call('history.undo');assert call('document.inspect')['authored']==edited['authored']
 # Export the unmodified scaled prism for the exact requested factor-2 bounds; then restore downstream.
 call('history.undo');call('history.undo');state=call('document.inspect');assert len(state['authored']['features'])==2
 call('solid.scale',{'featureId':scaleid,'factor':2});prism=call('document.inspect');verify(prism['evaluation']['bodies'][0],2)
 stl=out/'scale-factor-two.stl';call('document.exportStl',{'path':str(stl),'bodyId':body,'units':'mm','format':'binary','surfaceDeviationMm':.02,'normalDeviationDeg':5});mesh=inspect_stl(stl,True);assert len(mesh['components'])==1;near(mesh['signedVolume'],38400,.01)
 for actual,want in zip(mesh['bounds'][0],[20,-16,8]):near(actual,want,.001)
 for actual,want in zip(mesh['bounds'][1],[60,24,32]):near(actual,want,.001)
 call('history.undo');call('history.redo');verify(call('document.inspect')['evaluation']['bodies'][0],2)
 # Recreate the downstream operation after the bounded export branch and save/reopen all retained inputs.
 call('solid.faceOffset',{'targetBodyId':body,'face':ref(top),'distanceMm':1});savedstate=call('document.inspect');verify(savedstate['evaluation']['bodies'][0],2,1)
 saved=out/'scale-with-downstream.alloy';call('document.save',{'path':str(saved)});call('document.new',{'discard':True});call('document.open',{'path':str(saved)});reopened=call('document.inspect');assert reopened['authored']==savedstate['authored'] and reopened['evaluation']['bodies']==savedstate['evaluation']['bodies']
 (out/'receipt.json').write_text(json.dumps({'passed':True,'initial':initial,'preview':preview,'created':created,'downstream':downstream,'edited':edited,'inward':inward,'atomicRefusal':[response,response2],'prism':prism,'reopened':reopened,'stl':mesh,'transcript':log},indent=2)+'\n')
print('PASS Scale: origin bounds/volume, normalized retained references, preview/cancel, downstream edit, atomic refusal, history/reopen and independent STL')
