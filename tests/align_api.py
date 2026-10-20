#!/usr/bin/env python3
"""SOL-084 bounded retained face Align through the real local application."""
import argparse,json,math,uuid
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
def ref(f):return {k:f[k] for k in ('producerFeatureId','bodyId','faceId')}
def verify(state,source,target,sf,tf,flip=False):
    bs=state['evaluation']['bodies'];assert len(bs)==2 and bs[0]['bodyId']==source and bs[1]==target
    b=bs[0];assert b['valid'] and b['closed'] and b['topology']['solids']==1
    f=next(f for f in b['supportFaces'] if f['faceId']==sf['faceId'])['frame'];t=tf['frame']
    for key,sign in [('origin',1),('uAxis',1),('normal',-1 if flip else 1)]:assert all(math.isclose(x,sign*y,abs_tol=1e-7) for x,y in zip(f[key],t[key])),(key,f,t)
with application(a.app,out) as (call,log):
    call('solid.box',{'width':10,'depth':15,'height':20,'units':'mm','name':'Moving'})
    call('solid.box',{'width':12,'depth':18,'height':25,'x':50,'z':30,'units':'mm','name':'Stationary'})
    initial=call('document.inspect');bs=initial['evaluation']['bodies'];source=bs[0]['bodyId'];target=bs[1];sf=bs[0]['supportFaces'][0];tf=bs[1]['supportFaces'][3];args={'sourceFace':ref(sf),'targetFace':ref(tf)}
    session=call('command.begin',{'command':'solid.align','inputs':{}});sid=session['id'];call('command.update',{'session':sid,'inputs':args});preview=call('command.preview',{'session':sid});assert preview['previewed'] and len(preview['geometry']['align']['transform'])==4
    assert call('document.inspect')['authored']==initial['authored'];call('command.update',{'session':sid,'inputs':{'flip':True}});flipped=call('command.preview',{'session':sid});assert flipped['geometry']['align']['flip'];call('command.cancel',{'session':sid});cancel=call('document.inspect');assert cancel['authored']==initial['authored'] and cancel['undo']==initial['undo']
    session=call('command.begin',{'command':'solid.align','inputs':args});call('command.commit',{'session':session['id']});aligned=call('document.inspect');verify(aligned,source,target,sf,tf);assert aligned['undo']['index']==initial['undo']['index']+1;alignid=aligned['authored']['features'][-1]['id']
    call('history.undo');assert call('document.inspect')['authored']==initial['authored'];call('history.redo');assert call('document.inspect')['authored']==aligned['authored']
    call('solid.align',{**args,'featureId':alignid,'flip':True});flipped=call('document.inspect');verify(flipped,source,target,sf,tf,True)
    # A later retained face operation consumes the moved exact support.
    other=next(f for f in flipped['evaluation']['bodies'][0]['supportFaces'] if f['faceId']!=sf['faceId'])
    call('solid.faceOffset',{'targetBodyId':source,'face':ref(other),'distanceMm':1});downstream=call('document.inspect');assert downstream['evaluation']['bodies'][0]['volumeMm3']!=3000 and downstream['evaluation']['bodies'][1]==target
    call('solid.align',{**args,'featureId':alignid,'flip':False});edited=call('document.inspect');assert edited['evaluation']['bodies'][0]['valid'] and edited['evaluation']['bodies'][1]==target
    edit_session=call('command.begin',{'command':'solid.align','inputs':{**args,'featureId':alignid,'flip':True}});edit_preview=call('command.preview',{'session':edit_session['id']});assert edit_preview['geometry']['align']['flip'];call('command.cancel',{'session':edit_session['id']});assert call('document.inspect')['authored']==edited['authored']
    before=call('document.inspect');call('solid.align',{'sourceFace':ref(sf),'targetFace':ref(sf)},error='invalid_align');assert call('document.inspect')['authored']==before['authored']
    missing=ref(sf);missing['faceId']=str(uuid.uuid4());call('solid.align',{'sourceFace':missing,'targetFace':ref(tf)},error='evaluation_failed');assert call('document.inspect')['authored']==before['authored']
    path=out/'aligned-downstream.alloy';call('document.save',{'path':str(path)});call('document.new',{'discard':True});call('document.open',{'path':str(path)});reopened=call('document.inspect');assert reopened['authored']==edited['authored'] and reopened['evaluation']['bodies']==edited['evaluation']['bodies']
    (out/'receipt.json').write_text(json.dumps({'passed':True,'initial':initial,'preview':preview,'flippedPreview':flipped,'aligned':aligned,'downstream':downstream,'reopened':reopened,'transcript':log},indent=2)+'\n')
print('PASS retained Align: preview/flip/cancel, exact frames, target fixed, downstream, undo/edit/reopen and atomic refusal')
