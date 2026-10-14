#!/usr/bin/env python3
"""HIS-007 real-entrypoint Remove trial on an accepted open sheet."""
import argparse
import json
from pathlib import Path
from app_harness import application

parser=argparse.ArgumentParser()
parser.add_argument('--app',required=True)
parser.add_argument('--source',required=True)
parser.add_argument('--output',required=True)
args=parser.parse_args();out=Path(args.output).resolve();out.mkdir(parents=True,exist_ok=True)

with application(args.app,out,command_timeout=90) as (call,ledger):
    call('window.resize',{'width':1400,'height':920})
    call('document.open',{'path':str(Path(args.source).resolve()),'discard':True})
    initial=call('document.inspect');body=initial['evaluation']['bodies'][0]
    assert body['type']=='surface' and body['topology']['solids']==0
    body_id=body['bodyId'];before=initial['authored'];source_feature=before['features'][-1]['id']
    session=call('body.removeDialog',{'bodyId':body_id})
    assert session['command']=='body.remove' and session['previewed']
    assert call('command.preview',{'session':session['id']})['geometry']['remainingBodies']==0
    assert call('document.inspect')['authored']==before
    call('capture.dialog',{'path':str(out/'remove-controls.png')})
    call('capture.screenshot',{'path':str(out/'remove-preview.png')})
    call('command.cancel',{'session':session['id']})
    assert call('document.inspect')['authored']==before
    removed=call('body.remove',{'bodyId':body_id})
    removed_feature=removed['featureId'];after=call('document.inspect')
    assert after['evaluation']['bodies']==[]
    assert after['authored']['features'][-1]['id']==removed_feature
    assert after['authored']['features'][-1]['type']=='removeBody'
    assert any(row['id']==source_feature for row in after['authored']['features'])
    call('capture.screenshot',{'path':str(out/'removed-current.png')})
    call('history.undo')
    restored=call('document.inspect')
    assert restored['authored']==before and restored['evaluation']['bodies'][0]['bodyId']==body_id
    call('history.redo')
    assert call('document.inspect')['authored']==after['authored']
    path=out/'removed-sheet.alloy';call('document.save',{'path':str(path)})
    call('document.new',{'discard':True});call('document.open',{'path':str(path),'discard':True})
    assert call('document.inspect')['authored']==after['authored']
    edit=call('feature.editDialog',{'featureId':removed_feature})
    assert edit['previewed'] and edit['inputs']['bodyId']==body_id
    call('command.cancel',{'session':edit['id']})
    assert call('document.inspect')['authored']==after['authored']
    call('feature.delete',{'featureId':removed_feature})
    restored=call('document.inspect')
    assert restored['evaluation']['bodies'][0]['bodyId']==body_id
    assert restored['evaluation']['bodies'][0]['supportFaces'][0]['faceId']==body['supportFaces'][0]['faceId']
    (out/'result.json').write_text(json.dumps({'passed':True,'bodyId':body_id,
        'removeFeatureId':removed_feature,'sourceFeatureId':source_feature,
        'reopenedRemoved':True,'deletedRemoveRestored':True},indent=2)+'\n')
    (out/'transcript.json').write_text(json.dumps(ledger,indent=2)+'\n')
