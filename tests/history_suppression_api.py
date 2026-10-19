#!/usr/bin/env python3
"""Bounded real-entrypoint source suppression / restoration contract."""
import argparse,json
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,ledger):
    call('window.resize',{'width':1400,'height':900})
    call('solid.box',{'width':20,'depth':15,'height':10,'units':'mm','name':'Box A'})
    body=call('document.inspect')['evaluation']['bodies'][0];source=body['featureId']
    top=next(f for f in body['supportFaces'] if f['role']=='box.z1');face={k:top[k] for k in ('producerFeatureId','bodyId','faceId')}
    call('solid.faceOffset',{'targetBodyId':body['bodyId'],'face':face,'distanceMm':5})
    call('solid.box',{'width':8,'depth':6,'height':4,'x':40,'units':'mm','name':'Box B'})
    full=call('document.inspect');ids=[f['id'] for f in full['authored']['features']];assert len(ids)==3
    # Box command placement is checked independently from visual separation.
    assert full['evaluation']['bodies'][1]['boundsMm']['min'][0]==40
    call('view.fit');call('selection.set',{'bodyId':body['bodyId']})
    call('feature.visibility',{'featureId':source,'visible':False});hidden=call('document.inspect')
    assert len(hidden['evaluation']['bodies'])==2 and hidden['evaluation']['bodies'][0]['volumeMm3']==4500
    assert not any(s['effectiveSuppressed'] for s in hidden['evaluation']['featureStates'].values())
    call('history.undo');assert call('document.inspect')['authored']==full['authored']
    result=call('history.suppression',{'featureId':source,'suppressed':True});assert result['changed'] and result['affectedDependentIds']==[ids[1]]
    suppressed=call('document.inspect');assert len(suppressed['evaluation']['bodies'])==1 and abs(suppressed['evaluation']['bodies'][0]['volumeMm3']-192)<1e-6, suppressed
    states=suppressed['evaluation']['featureStates'];assert states[source]['explicitSuppressed'] and states[ids[1]]['effectiveSuppressed'] and not states[ids[1]]['explicitSuppressed']
    assert suppressed['evaluation']['featureOrder']==ids and suppressed['authored']['features'][1:]==full['authored']['features'][1:]
    same=call('history.suppression',{'featureId':source,'suppressed':True});assert not same['changed'];assert call('document.inspect')==suppressed
    call('history.suppression',{'featureId':'missing','suppressed':True},error='stale_entity');assert call('document.inspect')==suppressed
    call('feature.edit',{'featureId':source,'parameters':{'widthMm':24}},error='feature_suppressed');assert call('document.inspect')==suppressed
    state=call('app.state');assert state['viewport']['gpu']['hardwareAccelerated'] and state['viewport']['gpu']['selectedSupportFace']=={}
    tiles=state['ui']['timeline']['features'];assert [t['effectiveSuppressed'] for t in tiles]==[True,True,False]
    call('capture.screenshot',{'path':str(out/'suppressed.png')})
    call('history.undo');assert call('document.inspect')['authored']==full['authored'];call('history.redo');assert call('document.inspect')['authored']==suppressed['authored']
    path=str(out/'suppressed.alloy');call('document.save',{'path':path});call('document.open',{'path':path})
    reopened=call('document.inspect');assert reopened['authored']==suppressed['authored'] and reopened['evaluation']['bodies']==suppressed['evaluation']['bodies']
    call('history.suppression',{'featureId':source,'suppressed':False});restored=call('document.inspect')
    assert restored['evaluation']['bodies']==full['evaluation']['bodies'] and restored['authored']['features'][0]['suppressed'] is False
    call('history.undo');assert call('document.inspect')['bodies']==1;call('history.redo');assert call('document.inspect')['evaluation']['bodies']==full['evaluation']['bodies']
    call('document.save',{'path':str(out/'restored.alloy')});call('document.open',{'path':str(out/'restored.alloy')});assert call('document.inspect')['authored']==restored['authored']
    call('feature.edit',{'featureId':source,'parameters':{'widthMm':24}});edited=call('document.inspect');assert abs(edited['evaluation']['bodies'][0]['volumeMm3']-5400)<1e-6
    session=call('feature.editDialog',{'featureId':source});call('command.cancel',{'session':session['id']});assert call('document.inspect')['authored']==edited['authored']
    call('history.undo');call('view.fit');call('capture.screenshot',{'path':str(out/'restored.png')})
    call('timeline.rollbackBefore',{'featureId':ids[1]});call('history.suppression',{'featureId':source,'suppressed':True},error='history_rollback_read_only');call('timeline.returnToLatest')
    # An unsupported history family refuses without changing the current document/history.
    call('solid.cylinder',{'radius':3,'height':4,'units':'mm','name':'Unsupported cylinder'})
    before=call('document.inspect');call('history.suppression',{'featureId':source,'suppressed':True},error='suppression_refused');assert call('document.inspect')==before
    (out/'transcript.json').write_text(json.dumps(ledger,indent=2));(out/'report.json').write_text(json.dumps({'passed':True,'ids':ids,'gpu':state['viewport']['gpu'],'full':full,'suppressed':suppressed,'restored':restored},indent=2))
