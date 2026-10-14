#!/usr/bin/env python3
"""Closed inward Shell through the real entrypoint; rounded-collapse gate is separate."""
import argparse,json,math
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,transcript):
    def inspect():return call('document.inspect')
    call('solid.box',{'width':50,'depth':80,'height':40,'x':-25,'y':-40,'units':'mm'})
    source=inspect();body=source['evaluation']['bodies'][0]
    call('selection.set',{'bodyId':body['bodyId']})
    s=call('solid.shellDialog',{'thicknessMm':3.5})
    assert s['geometry']['closedCavity'] and s['geometry']['topology']['shells']==2
    assert inspect()['authored']==source['authored']
    call('command.cancel',{'session':s['id']})
    call('solid.shell',{'targetBodyId':body['bodyId'],'thicknessMm':3.5})
    shelled=inspect();result=shelled['evaluation']['bodies'][0]
    assert result['valid'] and result['closed'] and result['topology']['shells']==2
    assert result['boundsMm']==body['boundsMm']
    assert math.isclose(result['volumeMm3'],50*80*40-43*73*33,abs_tol=1e-4)
    original_inner={f['faceId']:f for f in result['supportFaces'] if f['role']=='shell-inner'}
    assert len(original_inner)==6
    call('history.undo');assert inspect()['authored']==source['authored']
    call('history.redo');assert inspect()['authored']==shelled['authored']
    shell=shelled['authored']['features'][-1]
    call('solid.shell',{'featureId':shell['id'],'targetBodyId':body['bodyId'],'thicknessMm':100},error='evaluation_failed')
    assert inspect()['authored']==shelled['authored']
    call('solid.shell',{'featureId':shell['id'],'targetBodyId':body['bodyId'],'thicknessMm':4})
    edit=call('feature.editDialog',{'featureId':source['authored']['features'][0]['id']})
    call('command.update',{'session':edit['id'],'inputs':{'width':60}})
    call('command.commit',{'session':edit['id']})
    edited=inspect();result=edited['evaluation']['bodies'][0]
    assert math.isclose(result['volumeMm3'],60*80*40-52*72*32,abs_tol=1e-4)
    edited_inner={f['faceId']:f for f in result['supportFaces'] if f['role']=='shell-inner'}
    assert edited_inner.keys()==original_inner.keys()
    assert all(edited_inner[k]['frame']['normal']==original_inner[k]['frame']['normal'] for k in original_inner)
    assert any(edited_inner[k]['frame']['origin']!=original_inner[k]['frame']['origin'] for k in original_inner)
    call('inspect.section',{'plane':'XZ','hatch':True})
    call('view.home');call('selection.set',{'bodyId':''})
    call('capture.screenshot',{'path':str(out/'closed-cavity-section.png')})
    gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated']
    path=out/'closed-shell.alloy';call('document.save',{'path':str(path)})
    final=inspect();call('document.open',{'path':str(path),'discard':True})
    reopened=inspect()
    assert reopened['authored']==final['authored']
    assert reopened['evaluation']['bodies'][0]['supportFaces']==final['evaluation']['bodies'][0]['supportFaces']
    (out/'receipt.json').write_text(json.dumps({'source':source,'shelled':shelled,'edited':edited,
        'gpu':gpu,'transcript':transcript},indent=2)+'\n')
print('PASS: supported closed cavity, section, edit/history/reopen')
