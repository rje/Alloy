#!/usr/bin/env python3
"""Origin section lifecycle through the actual application entrypoint."""
import argparse,json,time
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);args=p.parse_args();out=Path(args.output).resolve()
with application(args.app,out) as (call,transcript):
    def inspect():return call('document.inspect')
    def evaluated(state):return {k:v for k,v in state['evaluation'].items() if k!='documentRevision'}
    def section():return call('app.state')['viewport']['gpu']['sectionAnalysis']
    def capture(name):
        time.sleep(.15);return call('capture.screenshot',{'path':str(out/(name+'.png'))})['viewportSha256']
    call('sketch.begin',{'plane':'XY'});call('sketch.addCircle',{'centerX':0,'centerY':0,'diameter':50,'units':'mm'});sketch=call('app.state')['activeSession']['draftSketch'];call('sketch.finish');profile=inspect()['evaluation']['sketches'][0]['profiles'][0]['profileId']
    call('solid.extrude',{'sketchId':sketch['id'],'profileId':profile,'distance':60,'units':'mm'});body=inspect()['evaluation']['bodies'][0];cap=next(f for f in body['supportFaces'] if f['role']=='extrude.endCap');call('solid.shell',{'targetBodyId':body['bodyId'],'removedFace':{k:cap[k] for k in ('producerFeatureId','bodyId','faceId')},'thicknessMm':5});call('view.home');call('view.grid',{'visible':False});call('view.origin',{'visible':False})
    before=inspect();geometry=evaluated(before);features=before['authored']['features'];uncut=capture('unclipped')
    s=call('inspect.sectionDialog');assert not s['previewed'];call('command.update',{'session':s['id'],'inputs':{'plane':'XZ'}});assert section()['plane']=='XZ';hatched=capture('preview-hatched');call('command.cancel',{'session':s['id']});assert inspect()==before;assert section()=={}
    s=call('inspect.sectionDialog',{'plane':'XZ'});reserved=s['newAnalysisId'];call('command.commit',{'session':s['id']});first=inspect();assert first['authored']['analyses'][0]['id']==reserved;assert first['authored']['features']==features and evaluated(first)==geometry;assert section()['id']==reserved
    s=call('inspect.sectionDialog',{'analysisId':reserved});call('command.update',{'session':s['id'],'inputs':{'hatch':False}});plain=capture('plain-cap');call('command.cancel',{'session':s['id']});assert inspect()==first and section()['hatch']
    s=call('inspect.sectionDialog',{'plane':'YZ'});assert section()['plane']=='YZ';call('command.cancel',{'session':s['id']});assert inspect()==first and section()['plane']=='XZ'
    call('inspect.section',{'plane':'YZ'});second=inspect();assert not second['authored']['analyses'][0]['visible'];call('history.undo');assert inspect()['authored']==first['authored'];assert section()['plane']=='XZ';call('history.redo');assert inspect()['authored']==second['authored'];assert section()['plane']=='YZ'
    call('analysis.visibility',{'analysisId':reserved,'visible':True});assert section()['plane']=='XZ';capture('retained-hatched');call('analysis.visibility',{'analysisId':reserved,'visible':False});hidden=capture('hidden');assert section()=={};call('analysis.visibility',{'analysisId':reserved,'visible':True});assert evaluated(inspect())==geometry and inspect()['authored']['features']==features
    for plane in ('XY','YZ','XZ'):
        call('inspect.section',{'analysisId':reserved,'plane':plane,'hatch':True});assert section()['plane']==plane;capture('plane-'+plane.lower())
    saved=inspect()['authored'];path=out/'section.alloy';call('document.save',{'path':str(path)});tab=call('document.tabs')['activeTabId'];other=call('document.tabNew')['activeTabId'];assert section()=={};call('solid.box',{'width':20,'depth':22,'height':24,'units':'mm'});capture('other-tab');call('document.tabActivate',{'tabId':tab});assert inspect()['authored']==saved and section()['plane']=='XZ';capture('restored-tab');call('document.open',{'path':str(path)});assert inspect()['authored']==saved and section()['plane']=='XZ';capture('reopened')
    assert len({hatched,plain,uncut})==3 and hidden==uncut,'distinct cap/hatch/hidden views'
    gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated'] and gpu['sectionClippedActors']==1
    call('inspect.section',{'plane':'bad'},error='invalid_arguments');call('analysis.visibility',{'analysisId':'missing','visible':True},error='invalid_arguments')
    (out/'report.json').write_text(json.dumps({'passed':True,'calls':len(transcript),'gpu':gpu,'unchangedFeatures':True,'unchangedGeometry':True,'distinctViews':True},indent=2));(out/'transcript.json').write_text(json.dumps(transcript,indent=2))
