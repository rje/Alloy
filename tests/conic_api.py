#!/usr/bin/env python3
"""Real-app conic authoring and disposable input; physical input is a separate gate."""
import argparse
import hashlib
import json
from pathlib import Path
from app_harness import application


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--app',required=True)
    parser.add_argument('--output',required=True)
    args=parser.parse_args()
    out=Path(args.output).resolve();out.mkdir(parents=True,exist_ok=True)
    report={'status':'incomplete','scope':'Shared conic commands, not native source acceptance',
            'appSha256':hashlib.sha256(Path(args.app).read_bytes()).hexdigest()}
    transcript=[]
    try:
        with application(args.app,out) as (call,transcript):
            def session():return call('app.state')['activeSession']
            def authored():return call('document.inspect')['authored']
            call('sketch.begin',{'plane':'XY'})
            before=session()['draftSketch']
            call('sketch.tool',{'tool':'conic'})
            for u,v in [(-30,-10),(30,-10),(0,40)]:
                p=call('view.projectSketchPoint',{'plane':'XY','uMm':u,'vMm':v})
                call('sketch.placePoint',{'x':p['x'],'y':p['y']})
                report.setdefault('placementStates',[]).append(session())
            assert session()['draftSketch']==before
            assert session()['pendingConic']['valid']
            call('sketch.conicPreview',{'text':'1.5'})
            assert not session()['pendingConic']['valid']
            call('sketch.conicAccept',error='invalid_conic_rho')
            assert session()['draftSketch']==before
            call('sketch.conicPreview',{'text':'0.683'})
            assert session()['pendingConic']['valid']
            call('capture.screenshot',{'path':str(out/'pending-conic.png')})
            call('sketch.conicAccept')
            draft=session()['draftSketch'];entity=draft['parameters']['entities'][0]
            assert entity['type']=='conic' and entity['rho']==.683
            assert session()['solverDiagnostics']['conicRhoDof']==1
            call('sketch.finish')
            saved=authored();path=out/'conic.alloy';call('document.save',{'path':str(path)})
            call('sketch.edit',{'sketchId':draft['id']})
            call('sketch.conicRho',{'entityId':entity['id'],'rho':.5})
            call('sketch.finish');changed=authored();assert changed!=saved
            call('history.undo');assert authored()==saved
            call('history.redo');assert authored()==changed
            call('history.undo');call('document.open',{'path':str(path)});assert authored()==saved
            report.update(status='api-pass',sourceAuthored=saved)
    finally:
        report['callCount']=len(transcript)
        (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
        (out/'transcript.json').write_text(json.dumps(transcript,indent=2)+'\n')
    print('PASS: conic preview, invalid input refusal, retained edit/history/reopen')


if __name__=='__main__':main()
