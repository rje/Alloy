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
        camera=gpu['camera']
        assert abs(camera['aspect']-initial['viewport']['width']/initial['viewport']['height'])<1e-9
        assert all(abs(a-b)<1e-9 for a,b in zip(camera['center'],[22.5,15,5])), camera
        point=call('view.projectBody',{'bodyId':body_id})
        assert 0<=point['x']<initial['viewport']['width'] and 0<=point['y']<initial['viewport']['height'],point
        unselected=call('capture.screenshot',{'path':str(output/'bodies-unselected.png')})
        call('selection.set',{'bodyId':body_id})
        selected=call('capture.screenshot',{'path':str(output/'body-selected.png')})
        assert selected['viewportSha256']!=unselected['viewportSha256'], 'Selection must visibly change the shaded body'
        call('selection.set',{'bodyId':''})
        cleared=call('capture.screenshot',{'path':str(output/'selection-cleared.png')})
        assert cleared['viewportSha256']==unselected['viewportSha256'], 'Clearing selection must restore the body appearance'
        assert call('document.inspect')==two, 'Selection must not edit the document'
        picked=call('selection.pick',{'x':point['x'],'y':point['y']});assert picked['bodyId']==body_id,picked
        fractional=call('selection.pick',{'x':point['x']+0.5,'y':point['y']+0.5})
        assert fractional['bodyId']==body_id, 'Retina fractional mouse coordinates must not become zero'
        assert call('app.state')['viewport']['gpu']['camera']==camera, 'Picking must not change the camera'
        call('view.zoom',{'factor':0.5})
        zoomed=call('app.state')['viewport']['gpu']['camera']
        assert zoomed['direction']==camera['direction'] and zoomed['center']==camera['center']
        assert abs(zoomed['scale']-camera['scale']*2)<1e-9
        point=call('view.projectBody',{'bodyId':body_id})
        assert call('selection.pick',{'x':point['x'],'y':point['y']})['bodyId']==body_id
        assert call('app.state')['viewport']['gpu']['camera']==zoomed
        call('view.home')
        call('view.pan',{'x':24,'y':-12})
        panned=call('app.state')['viewport']['gpu']['camera']
        assert panned['direction']==camera['direction'] and panned['up']==camera['up']
        assert panned['center']!=camera['center']
        assert call('app.state')['viewport']['gpu']['camera']==panned, 'Inspection must not accumulate pan'
        call('view.home')
        captures.append(call('capture.screenshot',{'path':str(output/'primitives-isometric.png')}))
        call('view.orientation',{'name':'top'})
        top_camera=call('app.state')['viewport']['gpu']['camera']
        assert top_camera['direction']==[0,0,-1], 'Named Top must preserve its orientation for offset bodies'
        assert all(abs(a-b)<1e-9 for a,b in zip(top_camera['center'],[22.5,15,5])), top_camera
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
        panel_state=call('app.state');panel=panel_state['ui']['activeDialog'];viewport=panel_state['viewport']
        assert panel['presentation']=='docked-panel' and panel['visible']
        assert viewport['x']+viewport['width']<=panel['bounds']['x'], 'Editor must reserve space outside the model viewport'
        call('command.panelDock',{'docked':False})
        assert call('app.state')['ui']['activeDialog']['presentation']=='floating-panel'
        call('command.panelDock',{'docked':True})
        assert call('app.state')['ui']['activeDialog']['presentation']=='docked-panel'
        call('command.update',{'session':draft['id'],'inputs':{'width':12}})
        assert call('app.state')['ui']['activeDialog']['inputs']['width']==12
        call('command.preview',{'session':draft['id']})
        assert call('app.state')['viewport']['gpu']['previewReplacedBodyId']==body_id
        captures.append(call('capture.screenshot',{'path':str(output/'edit-panel-preview.png')}))
        call('command.update',{'session':draft['id'],'inputs':{'width':0.000001}})
        call('command.preview',{'session':draft['id']},'command_failed')
        assert call('app.state')['viewport']['gpu']['previewReplacedBodyId']==''
        call('command.cancel',{'session':draft['id']})
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
