#!/usr/bin/env python3
"""Bounded accumulated body Move: real GPU app, rigid geometry, history and Cut."""
import argparse,json,math
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);args=p.parse_args();out=Path(args.output).resolve()
with application(args.app,out) as (call,log):
    gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated']
    call('solid.box',{'width':30,'depth':30,'height':11,'x':-15,'y':-15,'z':0,'units':'mm'})
    call('solid.cylinder',{'radius':3,'height':40,'x':0,'y':0,'z':11,'units':'mm'})
    source=call('document.inspect');target,tool=[f['bodyId'] for f in source['authored']['features']]
    call('view.home')
    session=call('solid.moveDialog',{'targetBodyId':tool});sid=session['id']
    assert session['inputs']['pivotZMm']==11
    call('command.update',{'session':sid,'inputs':{'yDeg':10}})
    call('command.preview',{'session':sid})
    accepted=call('solid.moveApplyGesture');assert accepted['inputs']['yDeg']==0
    call('command.update',{'session':sid,'inputs':{'zMm':-20}})
    preview=call('command.preview',{'session':sid});assert math.isclose(preview['geometry']['volumeMm3'],math.pi*9*40,rel_tol=1e-9)
    call('capture.screenshot',{'path':str(out/'move-preview.png')});call('capture.dialog',{'path':str(out/'move-panel.png')})
    handles=call('viewport.moveHandles');assert len(handles['handles'])==3
    assert call('document.inspect')['authored']==source['authored']
    call('command.cancel',{'session':sid});assert call('document.inspect')['authored']==source['authored']
    session=call('solid.moveDialog',{'targetBodyId':tool,'steps':[{'xMm':0,'yMm':0,'zMm':0,'xDeg':0,'yDeg':10,'zDeg':0},{'xMm':0,'yMm':0,'zMm':-20,'xDeg':0,'yDeg':0,'zDeg':0}]})
    call('command.commit',{'session':session['id']});moved=call('document.inspect');assert moved['undo']['count']==source['undo']['count']+1
    moveid=moved['authored']['features'][-1]['id'];assert moved['bodies']==2
    call('history.undo');assert call('document.inspect')['authored']==source['authored'];call('history.redo');assert call('document.inspect')['authored']==moved['authored']
    call('solid.combine',{'targetBodyId':target,'toolBodyIds':[tool],'operation':'cut','keepTools':False})
    cut=call('document.inspect');assert cut['bodies']==1;assert cut['evaluation']['bodies'][0]['volumeMm3']<9900
    edit=call('feature.editDialog',{'featureId':moveid});call('command.update',{'session':edit['id'],'inputs':{'xMm':1}});call('command.preview',{'session':edit['id']});call('command.cancel',{'session':edit['id']});assert call('document.inspect')['authored']==cut['authored']
    edit=call('feature.editDialog',{'featureId':moveid});call('command.update',{'session':edit['id'],'inputs':{'xMm':1}});call('command.commit',{'session':edit['id']});final=call('document.inspect');assert final['bodies']==1
    path=out/'moved-cut.alloy';call('document.save',{'path':str(path)});call('document.open',{'path':str(path)});assert call('document.inspect')['authored']==final['authored']
    call('capture.screenshot',{'path':str(out/'moved-cut.png')})
    (out/'transcript.json').write_text(json.dumps(log,indent=2));(out/'report.json').write_text(json.dumps({'passed':True,'gpu':gpu,'source':source,'moved':moved,'final':final,'handles':handles},indent=2))
