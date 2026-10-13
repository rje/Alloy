"""Focused owned parameter-dialog capture/recording through the actual app."""
import argparse
import json
import time
from pathlib import Path
from app_harness import application

p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args()
out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
with application(a.app,out) as (call,transcript):
    call('window.resize',{'width':1400,'height':1000})
    call('recording.start',{'path':str(out/'parameters.mp4'),'fps':10})
    call('recording.step',{'text':'API | Change Parameters: named lengths'})
    call('document.parametersDialog')
    for name,value in [('Length',75),('Width',50),('Thickness',10)]:
        call('document.parameterAdd',{'name':name,'value':value,'units':'mm'})
        time.sleep(.35)
    before=call('document.inspect')['authored']
    assert len(before['userParameters'])==3
    time.sleep(.5)
    dialog=call('capture.dialog',{'path':str(out/'parameters-dialog.png')})
    call('capture.screenshot',{'path':str(out/'parameters-main.png')})
    call('document.parametersDialog',{'visible':False})
    assert call('document.inspect')['authored']==before
    time.sleep(.25)
    recording=call('recording.stop')
    report={'result':'passed','scope':'Parameter dialog lifecycle and actual capture only',
            'authored':before,'dialog':dialog,'recording':recording}
    (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    (out/'transcript.json').write_text(json.dumps(transcript,indent=2)+'\n')
