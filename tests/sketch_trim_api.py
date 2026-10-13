#!/usr/bin/env python3
"""Real-app bounded Trim preview, draft Undo and authored persistence; native input remains separate."""
import argparse,json,math
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);args=p.parse_args();out=Path(args.output).resolve()
with application(args.app,out) as (call,log):
    assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
    session=call('sketch.begin',{'plane':'XY','name':'Trim interval fixture'})
    def draft(): return call('command.preview',{'session':session['id']})['authoredDraft']
    def entity_at(u,v):
        d=draft()['parameters'];pts={p['id']:(p['uMm'],p['vMm']) for p in d['points']}
        for e in d['entities']:
            if e['type']!='line': continue
            a,b=pts[e['startPointId']],pts[e['endPointId']];dx,dy=b[0]-a[0],b[1]-a[1];t=((u-a[0])*dx+(v-a[1])*dy)/(dx*dx+dy*dy)
            if 0<=t<=1 and math.hypot(u-a[0]-t*dx,v-a[1]-t*dy)<1e-5:return e['id']
        raise AssertionError(('missing line',u,v))
    call('sketch.rectangle',{'x1':0,'y1':0,'x2':32,'y2':-10,'units':'mm'})
    call('sketch.line',{'x1':0,'y1':-5,'x2':7.5,'y2':-5,'units':'mm'})
    call('sketch.addCircle',{'centerX':7.5,'centerY':-5,'diameter':15,'units':'mm'})
    circle=draft()['parameters']['entities'][-1]
    for u,v,kind,value in [(16,0,'horizontal',32),(0,-2,'vertical',10),(4,-5,'horizontal',7.5)]:call('sketch.dimension',{'kind':kind,'entityId':entity_at(u,v),'value':value,'units':'mm'})
    call('sketch.dimension',{'kind':'diameter','entityId':circle['id'],'value':15,'units':'mm'})
    before=draft();before_history=call('app.state')['activeSession']['draftHistory']
    # Far-right coincident strips are covered by the core source fixture; this
    # single app receipt isolates the command/preview/history boundary.
    call('sketch.tool',{'tool':'trim'})
    point=call('view.projectSketchPoint',{'plane':'XY','uMm':7.5,'vMm':0})
    preview=call('sketch.pointerMove',{'x':point['x'],'y':point['y']})
    assert preview['authoredChange'] is False and draft()==before
    assert call('app.state')['activeSession']['draftHistory']==before_history
    call('capture.screenshot',{'path':str(out/'trim-preview.png')})
    call('sketch.placePoint',{'x':point['x'],'y':point['y']});after=draft();assert after!=before
    call('history.undo') # Exits the active Trim tool, following existing sketch Undo policy.
    call('history.undo');assert draft()==before
    call('history.redo');assert draft()==after
    for u,v in [(0,-2.5),(0,-7.5),(4,-5),(7.5,-10)]:call('sketch.trim',{'entityId':entity_at(u,v),'uMm':u,'vMm':v})
    final=draft();assert len(final['parameters']['dimensions'])==2
    assert next(e for e in final['parameters']['entities'] if e['id']==circle['id'])==circle
    call('sketch.finish');committed=call('document.inspect');assert committed['features']==1
    call('history.undo');assert call('document.inspect')['features']==0
    call('history.redo');assert call('document.inspect')['authored']==committed['authored']
    path=out/'trimmed-sketch.alloy';call('document.save',{'path':str(path)});call('document.open',{'path':str(path)})
    assert call('document.inspect')['authored']==committed['authored']
    call('capture.screenshot',{'path':str(out/'trimmed-sketch.png')})
    (out/'report.json').write_text(json.dumps({'passed':True,'before':before,'afterFirstTrim':after,'final':committed},indent=2));(out/'transcript.json').write_text(json.dumps(log,indent=2))
