#!/usr/bin/env python3
"""Real-entrypoint transient history prefix, full restore, history and file proof."""
import argparse
import json
from pathlib import Path
from app_harness import application

parser=argparse.ArgumentParser()
parser.add_argument('--app',required=True)
parser.add_argument('--output',required=True)
a=parser.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,ledger):
    call('window.resize',{'width':1400,'height':900})
    call('solid.box',{'width':20,'depth':15,'height':10,'units':'mm','name':'First box'})
    stock=call('document.inspect')
    body=stock['evaluation']['bodies'][0]
    top=next(f for f in body['supportFaces'] if f['role']=='box.z1')
    support={k:top[k] for k in ('producerFeatureId','bodyId','faceId')}
    call('solid.faceOffset',{'targetBodyId':body['bodyId'],'face':support,'distanceMm':5})
    latest=call('document.inspect');features=latest['authored']['features']
    assert len(features)==2
    first,second=(f['id'] for f in features)
    call('view.fit');call('capture.screenshot',{'path':str(out/'latest.png')})
    call('timeline.rollbackBefore',{'featureId':second})
    prefix=call('document.inspect')
    assert prefix['authored']==latest['authored'] and prefix['revision']==latest['revision']
    assert prefix['undo']==latest['undo'] and prefix['dirty']==latest['dirty']
    assert prefix['bodies']==1 and prefix['evaluation']['featureOrder']==[first]
    old,current=prefix['evaluation']['bodies'][0],latest['evaluation']['bodies'][0]
    assert old['bodyId']==current['bodyId']==body['bodyId']
    assert abs(old['volumeMm3']-3000)<1e-6 and abs(current['volumeMm3']-4500)<1e-6
    assert old['boundsMm']['size']==[20,15,10] and current['boundsMm']['size']==[20,15,15]
    call('document.pendingDecision')
    call('document.answerDecision',{'id':'missing','choice':'cancel'},error='no_pending_decision')
    call('view.namedCreate',{'name':'Refused authored view'},error='history_rollback_read_only')
    call('view.zoom',{'factor':1.1})
    call('view.home')
    state=call('app.state');assert state['viewport']['gpu']['hardwareAccelerated']
    tiles=state['ui']['timeline']['features']
    assert len(tiles)==2 and tiles[0]['evaluated'] and not tiles[1]['evaluated']
    assert state['viewport']['gpu']['selectedSupportFace']=={}
    call('capture.screenshot',{'path':str(out/'prefix.png')})
    call('timeline.rollbackBefore',{'featureId':'missing'},error='rollback_unavailable')
    assert call('document.inspect')['evaluation']==prefix['evaluation']
    call('feature.edit',{'featureId':first,'parameters':{'widthMm':24}},error='history_rollback_read_only')
    assert call('document.inspect')['authored']==latest['authored']
    path=str(out/'saved-while-rolled.alloy')
    call('document.save',{'path':path})
    assert call('document.inspect')['rollback']['active']
    call('timeline.returnToLatest')
    assert call('document.inspect')['evaluation']==latest['evaluation']
    call('timeline.rollbackBefore',{'featureId':first})
    assert call('document.inspect')['bodies']==0
    call('history.undo');undone=call('document.inspect')
    assert not undone['rollback']['active'] and undone['features']==1
    call('history.redo');assert call('document.inspect')['authored']==latest['authored']
    call('timeline.rollbackBefore',{'featureId':second})
    call('document.open',{'path':path})
    reopened=call('document.inspect')
    assert not reopened['rollback']['active'] and reopened['bodies']==1
    assert reopened['authored']==latest['authored']
    assert reopened['evaluation']['bodies']==latest['evaluation']['bodies']
    call('capture.screenshot',{'path':str(out/'reopened.png')})
    (out/'transcript.json').write_text(json.dumps(ledger,indent=2))
    (out/'report.json').write_text(json.dumps({'passed':True,'featureIds':[first,second],
        'gpu':state['viewport']['gpu'],'assertions':'prefix/full/empty; authored/revision/history preservation; atomic invalid; edit refusal; save/reopen latest; Undo/Redo'},indent=2))
