#!/usr/bin/env python3
"""One real-app sketch drawing/history/storage workflow; solver work is separate."""
import argparse
import hashlib
import json
from pathlib import Path
from app_harness import application


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('app','output','fixture'):parser.add_argument('--'+name,required=True)
    args=parser.parse_args();output=Path(args.output).resolve();fixture=json.loads(Path(args.fixture).read_text())
    captures=[]
    with application(args.app,output) as (call,transcript):
        initial=call('app.state');assert initial['viewport']['gpu']['hardwareAccelerated']
        original=call('document.inspect')
        call('sketch.tool',{'tool':'rectangle'},'no_active_sketch')
        assert not call('app.state')['activeSession'], 'An idle rejection must not create a ghost session'
        cancelled=call('sketch.begin',{'plane':'XY'})
        call('sketch.line',{'x1':0,'y1':0,'x2':10,'y2':10,'units':'mm'})
        call('command.cancel',{'session':cancelled['id']})
        assert call('document.inspect')==original, 'Cancelled creation must preserve the document and journal'
        # Drawing uses the same two-click command path as native mouse input.
        session=call('command.begin',{'command':'sketch.create','inputs':{'plane':fixture['plane'],'name':fixture['name'],'units':'mm'}})
        call('sketch.tool',{'tool':'rectangle'})
        for u,v in [(fixture['rectangle']['x1'],fixture['rectangle']['y1']),(fixture['rectangle']['x2'],fixture['rectangle']['y2'])]:
            point=call('view.projectSketchPoint',{'plane':fixture['plane'],'uMm':u,'vMm':v})
            call('sketch.placePoint',{'x':point['x'],'y':point['y']})
        preview=call('command.preview',{'session':session['id']})
        draft=preview['authoredDraft'];assert draft['type']=='sketch' and 'bodyId' not in draft
        assert len(draft['parameters']['points'])==4 and len(draft['parameters']['entities'])==4
        assert preview['geometryEvaluated'] and call('document.inspect')==original
        captures.append(call('capture.screenshot',{'path':str(output/'sketch-draft.png')}))
        call('command.commit',{'session':session['id']})
        sketch=call('document.inspect');assert sketch['bodies']==0 and sketch['sketches']==1
        feature=sketch['authored']['features'][0];sketch_id=feature['id']
        assert len(feature['parameters']['entities'])==4 and sketch['undo']['count']==1
        gpu=call('app.state')['viewport']['gpu'];assert gpu['displayedSketches']==1
        call('history.undo');assert call('document.inspect')['authored']==original['authored']
        call('history.redo');assert call('document.inspect')['authored']==sketch['authored']
        # Re-enter, append a line, then cancel: shared draft does not edit history.
        before_edit=call('document.inspect')
        edit=call('sketch.edit',{'sketchId':sketch_id})
        call('sketch.line',{'x1':0,'y1':0,'x2':10,'y2':10,'units':'mm'})
        assert len(call('command.preview',{'session':edit['id']})['authoredDraft']['parameters']['entities'])==5
        call('command.cancel',{'session':edit['id']})
        assert call('document.inspect')==before_edit
        # Other origin-plane frames receive real authored/evaluated line geometry.
        for plane in ('XZ','YZ'):
            draft_session=call('command.begin',{'command':'sketch.create','inputs':{'plane':plane,'name':plane+' line','units':'mm'}})
            call('sketch.line',{'x1':0,'y1':0,'x2':10,'y2':20,'units':'mm'})
            if plane=='XZ':
                call('sketch.line',{'x1':10,'y1':20,'x2':30,'y2':20,'units':'mm'})
            evaluated=call('command.preview',{'session':draft_session['id']})
            assert evaluated['geometryEvaluated'] and evaluated['authoredDraft']['parameters']['plane']==plane
            if plane=='XZ':
                polyline=evaluated['authoredDraft']['parameters'];assert len(polyline['points'])==3 and len(polyline['entities'])==2
                assert polyline['entities'][0]['endPointId']==polyline['entities'][1]['startPointId']
            call('command.commit',{'session':draft_session['id']})
        authored=call('document.inspect');assert authored['sketches']==3 and authored['bodies']==0
        assert authored['undo']['count']==3
        native=output/'origin-sketches.alloy';call('document.save',{'path':str(native)})
        saved=call('document.inspect');assert not saved['dirty']
        call('document.new',{'discard':False});call('document.open',{'path':str(native)})
        reopened=call('document.inspect');assert reopened['authored']==saved['authored']
        assert reopened['sketches']==3 and reopened['bodies']==0 and reopened['undo']['count']==0
        assert len(reopened['evaluation']['sketches'])==3
        assert call('app.state')['viewport']['gpu']['displayedSketches']==3
        call('view.home');captures.append(call('capture.screenshot',{'path':str(output/'sketches-reopened.png')}))
    result={'result':'passed','fixtureSha256':hashlib.sha256(Path(args.fixture).read_bytes()).hexdigest(),
        'verifiedCapabilities':['sketch.origin_planes','sketch.authored_entities','sketch.transaction_lifecycle','sketch.native_storage','sketch.gpu_wires','sketch.drawing_commands'],
        'initialState':initial,'transcript':transcript,'captures':captures,
        'evidenceDimensions':{'api':{'status':'passed'},'hardware':{'status':'passed','diagnostics':gpu},
            'geometry':{'status':'passed'},'save_reopen':{'status':'passed'},'visual':{'status':'captured','reason':'GPU wire captures; physical mouse/keyboard and compatibility remain separate.'}}}
    (output/'sketch-api.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS: real origin-plane sketch drawing/history/storage workflow; constraints/dimensions separate')


if __name__=='__main__':main()
