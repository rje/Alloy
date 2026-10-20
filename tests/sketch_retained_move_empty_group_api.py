#!/usr/bin/env python3
"""Empty retained-Move group IDs must follow creation guards without crashing."""
import argparse,json
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,log):
    session=call('sketch.begin',{'plane':'XY'});call('sketch.rectangle',{'x1':10,'y1':8,'x2':30,'y2':20,'units':'mm'})
    def state():return call('app.state')['activeSession']
    original=state();draft=original['draftSketch'];ids=[e['id'] for e in draft['parameters']['entities']];authored=call('document.inspect')['authored'];history=original['draftHistory']
    cases=[{'groupId':''},{'groupId':'','entityIds':[]},{'groupId':'','entityIds':ids[:1]},{'groupId':'','entityIds':ids,'x':0,'y':0},{'retain':True},{'retain':True,'entityIds':[]}]
    for case in cases:
        call('sketch.movePreview',dict({'x':5,'y':-3,'units':'mm'},**case),error='sketch_move_failed')
        current=state();assert current['draftSketch']==draft and current['draftHistory']==history and 'pendingSketchMove' not in current
        assert call('document.inspect')['authored']==authored
    preview=call('sketch.movePreview',{'groupId':'','entityIds':ids,'x':5,'y':-3,'units':'mm'});assert preview['retained'] and preview['pending'];assert state()['draftSketch']==draft
    call('sketch.moveAccept');accepted=state();assert accepted['draftSketch']['parameters']['moveGroups'][0]['deltaUMm']==5
    call('sketch.movePreview',{'groupId':'','entityIds':ids,'x':0,'y':0,'units':'mm'},error='sketch_move_failed');assert state()['draftSketch']==accepted['draftSketch'] and state()['draftHistory']==accepted['draftHistory']
    call('sketch.cancel');assert call('document.inspect')['authored']==authored
    (out/'report.json').write_text(json.dumps({'passed':True,'refusalCases':cases,'accepted':accepted['draftSketch'],'authoredUnchanged':True},indent=2));(out/'transcript.json').write_text(json.dumps(log,indent=2))
