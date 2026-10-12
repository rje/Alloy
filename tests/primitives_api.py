#!/usr/bin/env python3
"""One representative native workflow for primitive/history/storage/display."""
import argparse
import hashlib
import json
import math
from pathlib import Path
from app_harness import application

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('app','output','fixture'):parser.add_argument('--'+name,required=True)
    args=parser.parse_args();fixture=json.loads(Path(args.fixture).read_text());output=Path(args.output).resolve()
    captures=[]
    with application(args.app,output) as (call,transcript):
        initial=call('app.state');assert initial['viewport']['gpu']['hardwareAccelerated']
        original=call('document.inspect')
        session=call('command.begin',{'command':'solid.box','inputs':fixture['box']})
        preview=call('command.preview',{'session':session['id']})
        assert preview['geometryEvaluated'] and abs(preview['geometry']['volumeMm3']-6000)<1e-6
        call('command.update',{'session':session['id'],'inputs':{'width':0.000001}})
        draft_state=call('app.state');assert not draft_state['activeSession']['previewed'] and not draft_state['viewport']['gpu']['previewVisible']
        call('command.preview',{'session':session['id']},'command_failed')
        failed_preview=call('app.state');assert not failed_preview['activeSession']['previewed'] and not failed_preview['viewport']['gpu']['previewVisible']
        assert call('document.inspect')==original
        call('command.cancel',{'session':session['id']});assert call('document.inspect')==original
        session=call('command.begin',{'command':'solid.box','inputs':fixture['box']})
        call('capture.dialog',{'path':str(output/'box-dialog.png')})
        call('command.commit',{'session':session['id']})
        box=call('document.inspect');assert box['undo']['count']==1 and box['bodies']==1
        box_feature=box['authored']['features'][0];box_id=box_feature['id'];body_id=box_feature['bodyId']
        assert abs(box['evaluation']['bodies'][0]['volumeMm3']-6000)<1e-6
        call('solid.cylinder',fixture['cylinder'])
        two=call('document.inspect');cylinder=two['authored']['features'][1];cylinder_id=cylinder['id']
        assert two['bodies']==2 and two['features']==2
        summaries=two['evaluation']['bodies']
        assert all(s['valid'] and s['closed'] for s in summaries)
        assert abs(summaries[1]['volumeMm3']-math.pi*250)<1e-6
        assert summaries[0]['boundsMm']['size']==[20,30,10]
        gpu=call('app.state')['viewport']['gpu'];assert gpu['displayedBodies']==2 and gpu['documentRevision']==two['revision']
        point=call('view.projectBody',{'bodyId':body_id})
        picked=call('selection.pick',{'x':point['x'],'y':point['y']});assert picked['bodyId']==body_id,picked
        captures.append(call('capture.screenshot',{'path':str(output/'primitives-isometric.png')}))
        call('view.orientation',{'name':'top'})
        captures.append(call('capture.screenshot',{'path':str(output/'primitives-top.png')}))
        assert captures[0]['viewportSha256']!=captures[1]['viewportSha256']
        call('view.home');call('selection.set',{'bodyId':''})
        call('feature.edit',{'featureId':box_id,'parameters':{'widthMm':24}})
        edited=call('document.inspect');assert edited['authored']['features'][0]['id']==box_id
        assert abs(edited['evaluation']['bodies'][0]['volumeMm3']-7200)<1e-6
        call('history.undo');assert call('document.inspect')['authored']==two['authored']
        call('history.redo');assert call('document.inspect')['authored']==edited['authored']
        before_failure=call('document.inspect')
        call('feature.edit',{'featureId':box_id,'parameters':{'widthMm':0}},'evaluation_failed')
        assert call('document.inspect')==before_failure
        call('feature.delete',{'featureId':cylinder_id});assert call('document.inspect')['bodies']==1
        call('history.undo');assert call('document.inspect')['authored']==edited['authored']
        call('history.redo');assert call('document.inspect')['bodies']==1
        call('history.undo');call('feature.edit',{'featureId':box_id,'parameters':{'depthMm':32}})
        branched=call('document.inspect');assert not branched['undo']['canRedo']
        draft=call('feature.editDialog',{'featureId':box_id})
        call('command.update',{'session':draft['id'],'inputs':{'width':50}})
        assert call('app.state')['ui']['activeDialog']['inputs']['width']==50
        call('command.preview',{'session':draft['id']});call('command.cancel',{'session':draft['id']})
        assert call('document.inspect')==branched
        call('document.units',{'units':'in'})
        call('solid.box',{'width':1,'depth':1,'height':1,'x':100,'units':'in','name':'Inch cube'})
        assert abs(call('document.inspect')['evaluation']['bodies'][-1]['volumeMm3']-25.4**3)<1e-6
        call('history.undo');call('history.undo')
        native=output/'gauge-block.alloy';call('document.save',{'path':str(native)})
        saved=call('document.inspect');assert not saved['dirty'];bytes_before=native.read_bytes()
        failure_path=output/'directory.alloy';failure_path.mkdir()
        call('document.save',{'path':str(failure_path)},'save_failed')
        assert native.read_bytes()==bytes_before and call('document.inspect')==saved
        bad=output/'malformed.alloy';bad.write_text('{')
        call('document.open',{'path':str(bad)},'open_failed');assert call('document.inspect')==saved
        newer=json.loads(bytes_before);newer['schema']='alloy-document/999';bad.write_text(json.dumps(newer))
        call('document.open',{'path':str(bad)},'open_failed');assert call('document.inspect')==saved
        call('document.new',{'discard':False});call('document.open',{'path':str(native)})
        reopened=call('document.inspect')
        assert reopened['authored']==saved['authored'] and not reopened['dirty'] and reopened['undo']['count']==0
        assert reopened['evaluation']['bodies']==saved['evaluation']['bodies']
        call('feature.edit',{'featureId':box_id,'parameters':{'heightMm':12}})
        assert abs(call('document.inspect')['evaluation']['bodies'][0]['volumeMm3']-24*32*12)<1e-6
        call('document.open',{'path':str(native)},'unsaved_changes')
        call('history.undo');assert not call('document.inspect')['dirty']
        call('view.home');captures.append(call('capture.screenshot',{'path':str(output/'primitives-final.png')}))
    caps=['document.transactions','document.atomic_native_storage','viewport.document_brep',
        'primitives.editable','geometry.trustworthy_summaries']
    result={'result':'passed','fixtureSha256':hashlib.sha256(Path(args.fixture).read_bytes()).hexdigest(),
        'verifiedCapabilities':caps,'initialState':initial,'transcript':transcript,'captures':captures,
        'evidenceDimensions':{'api':{'status':'passed'},'hardware':{'status':'passed','diagnostics':gpu},
            'geometry':{'status':'passed'},'save_reopen':{'status':'passed'},'visual':{'status':'captured',
                'reason':'Actual GPU/model/dialog captures; familiarity/physical acceptance remains separate.'}}}
    (output/'primitives-api.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS: native primitive/history/storage/GPU workflow; physical and product gates separate')
if __name__=='__main__':main()
