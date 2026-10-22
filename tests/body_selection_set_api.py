#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""DOC-043 real-entrypoint transient body/occurrence set contract."""
import argparse
import hashlib
import json
from pathlib import Path
from app_harness import application

parser=argparse.ArgumentParser()
parser.add_argument('--app',required=True)
parser.add_argument('--output',required=True)
args=parser.parse_args()
out=Path(args.output).resolve()
with application(args.app,out) as (call,ledger):
    call('window.resize',{'width':1440,'height':900})
    for x,name in ((-60,'Root A'),(0,'Root B'),(60,'Linked C')):
        call('solid.box',{'width':30,'depth':25,'height':20,'x':x,'y':0,'z':0,'units':'mm','name':name})
    bodies=call('document.inspect')['evaluation']['bodies']
    a,b,c=[body['bodyId'] for body in bodies]
    call('component.createFromBodies',{'bodyIds':[c]})
    occurrence=call('document.inspect')['authored']['occurrences'][0]['id']
    copied=call('component.copyOccurrence',{'occurrenceId':occurrence,'placement':{'axis':'Z','angleDeg':0,'translationMm':[0,50,0]}})['rootOccurrenceId']
    call('view.home');call('view.orientation',{'name':'top'})
    saved=out/'fixture.alloy'
    call('document.save',{'path':str(saved)})
    saved_bytes=saved.read_bytes()
    before=call('document.inspect')
    baseline_state=call('app.state')
    history=baseline_state['document']
    selection_revision=baseline_state['revision']
    def members():
        return call('app.state')['selection']['members']
    def select(body=None,operation='replace',occ=None):
        p={'operation':operation}
        if body is not None:p['bodyId']=body
        if occ is not None:p['occurrenceId']=occ
        result=call('selection.set',p)
        assert result['members']==members()
    def expected(*rows):
        assert members()==[{'bodyId':r[0],'occurrenceId':r[1] if len(r)>1 else ''} for r in rows],members()
    select(a);expected((a,))
    select(b,'add');expected((a,),(b,))
    call('capture.screenshot',{'path':str(out/'multi-selected.png')})
    select(b,'add');expected((a,),(b,))
    select(a,'toggle');expected((b,))
    select(b,'toggle');expected()
    select(c,occ=occurrence);select(c,'add',copied);expected((c,occurrence),(c,copied))
    prior=members()
    for params,code in (({'bodyId':c,'operation':'add'},'ambiguous_instance'),({'bodyId':'missing','operation':'toggle'},'stale_entity'),({'bodyId':c,'occurrenceId':'missing','operation':'replace'},'stale_entity')):
        call('selection.set',params,error=code)
        assert members()==prior
    call('capture.screenshot',{'path':str(out/'linked-selected.png')})
    select(operation='clear');expected()
    assert call('app.state')['revision']==selection_revision
    call('selection.priority',{'mode':'body'})
    for body,add,toggle in ((a,False,False),(b,True,False),(b,True,False),(a,False,True),(b,False,True)):
        at=call('view.projectBody',{'bodyId':body})
        call('selection.pick',{'x':at['x'],'y':at['y'],'additive':add,'toggle':toggle})
    expected()
    select(a);call('selection.pick',{'x':0,'y':0});expected()
    assert call('document.inspect')==before
    assert call('app.state')['document']==history
    call('document.save',{'path':str(saved)})
    assert saved.read_bytes()==saved_bytes
    select(a);select(b,'add')
    tab=call('document.tabs')['activeTabId']
    call('document.tabNew');expected()
    call('document.tabActivate',{'tabId':tab});expected((a,),(b,))
    select(c,occ=occurrence);select(c,'add',copied)
    call('history.undo');expected((c,occurrence))
    assert len(call('document.inspect')['authored']['occurrences'])==1
    call('history.redo');expected((c,occurrence))
    assert call('document.inspect')['authored']==before['authored']
    call('document.new',{'discard':True});expected()
    call('document.open',{'path':str(saved)});expected()
    assert call('document.inspect')['authored']==before['authored']
    # Undo is still the last authored action, never a selection-only action.
    call('history.undo')
    assert call('document.inspect')['authored']==before['authored']
    state=call('app.state')
    assert state['viewport']['gpu']['hardwareAccelerated']
    # Auto modifiers must still reach ordinary finished-profile selection.
    fixture_tab=call('document.tabs')['activeTabId']
    call('document.tabNew')
    call('selection.priority',{'mode':'body'})  # active Body toggles back to Auto
    call('sketch.begin',{'plane':'XY','name':'Profile modifier regression'})
    call('sketch.rectangle',{'x1':0,'y1':0,'x2':20,'y2':20,'units':'mm'})
    call('sketch.finish')
    sketch=call('document.inspect')['evaluation']['sketches'][0]
    call('view.home');call('view.orientation',{'name':'top'})
    point=call('view.projectSketchPoint',{'sketchId':sketch['featureId'],'uMm':10,'vMm':10})
    result=call('selection.pick',{'x':point['x'],'y':point['y'],'additive':True})
    assert result['kind']=='profile',result
    call('document.tabActivate',{'tabId':fixture_tab})
    (out/'report.json').write_text(json.dumps({'passed':True,'fixtureSha256':hashlib.sha256(saved_bytes).hexdigest(),'state':state,'bodyIds':[a,b,c],'occurrenceIds':[occurrence,copied]},indent=2))
    (out/'transcript.json').write_text(json.dumps(ledger,indent=2))
