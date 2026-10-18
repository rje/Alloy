#!/usr/bin/env python3
"""Retained split-cap planar inset Delete through the real shared commands."""
import argparse,json,hashlib
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--source',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out,command_timeout=180) as (call,ledger):
    inspect=lambda:call('document.inspect')
    session=lambda:call('app.state')['activeSession']
    draft=lambda:session()['draftSketch']
    source_path=Path(a.source).resolve()
    call('document.open',{'path':str(source_path)})
    call('window.resize',{'width':1500,'height':950})
    initial=inspect()
    body=next(b for b in initial['evaluation']['bodies'] if any(f['role']=='split.negative.cap' for f in b['supportFaces']))
    cap=next(f for f in body['supportFaces'] if f['role']=='split.negative.cap')
    assert len(cap['edges'])==18
    support={k:cap[k] for k in ('producerFeatureId','bodyId','faceId')}
    owner=next(b['ownerId'] for b in call('component.inspect')['bodies'] if b['bodyId']==body['bodyId'])
    call('component.activate',{'definitionId':owner})
    for feature in initial['authored']['features']:
        if feature['type'] in ('stepAssemblyImport','splitBody'):
            call('feature.visibility',{'featureId':feature['id'],'visible':False})
    for analysis in initial['authored'].get('analyses',[]):
        if analysis.get('visible'): call('analysis.visibility',{'analysisId':analysis['id'],'visible':False})
    for name in ('Selected loop','Independent loop'):
        call('sketch.begin',{'support':support,'name':name,'units':'mm'});call('sketch.tool',{'tool':'offset'})
        call('sketch.loopOffsetPreview',{'kind':'planarInsetV1','distance':-1.25,'chain':True,'units':'mm'})
        call('sketch.loopOffsetAccept');call('sketch.finish')
    other_id=inspect()['authored']['features'][-1]['id'];call('feature.visibility',{'featureId':other_id,'visible':False})
    base=inspect();target=base['authored']['features'][-2];other=base['authored']['features'][-1]
    profile=next(s for s in base['evaluation']['sketches'] if s['sketchId']==target['id'])['profiles'][0]['profileId']
    other_profile=next(s for s in base['evaluation']['sketches'] if s['sketchId']==other['id'])['profiles'][0]['profileId']
    assert target['parameters']['loopOffsets'][0]['kind']=='planarInsetV1'
    original_profile=next(s for s in base['evaluation']['sketches'] if s['sketchId']==target['id'])['profiles'][0]
    # Retained support edit reevaluates the same identity before deletion.
    src=next(f for f in base['authored']['features'] if f['name']=='Centered case stock');radius=next(d for d in src['parameters']['dimensions'] if d['kind']=='horizontal' and d['valueMm']==64)
    call('sketch.edit',{'sketchId':src['id']});call('sketch.dimensionUpdate',{'dimensionId':radius['id'],'value':65,'units':'mm'});call('sketch.finish')
    base=inspect();assert next(s for s in base['evaluation']['sketches'] if s['sketchId']==target['id'])['profiles'][0]['profileId']==profile
    edited_profile=next(s for s in base['evaluation']['sketches'] if s['sketchId']==target['id'])['profiles'][0]
    assert abs(edited_profile['areaMm2']-original_profile['areaMm2'])>1
    assert target==next(f for f in base['authored']['features'] if f['id']==target['id'])
    call('document.save',{'path':str(out/'native-before.alloy')})
    call('sketch.edit',{'sketchId':target['id']});call('sketch.lookAt');call('view.fit');call('sketch.tool',{'tool':'select'})
    original=draft()
    for args,code in [({'profileIds':[profile,profile]},'invalid_profile_target'),({'profileIds':[other_profile]},'stale_profile'),({'profileIds':['00000000-0000-0000-0000-000000000001']},'stale_profile'),({'profileIds':[profile],'entityIds':['stale']},'ambiguous_delete_target')]:
        before=session();call('sketch.deleteSelection',args,error=code);after=session();now=inspect();assert after==before, (code,before,after);assert now['authored']==base['authored'] and now['evaluation']==base['evaluation'],code
    call('capture.screenshot',{'path':str(out/'before.png')});call('sketch.selectionSet',{'profileIds':[profile]});call('capture.screenshot',{'path':str(out/'selected.png')})
    before=session();call('sketch.deleteSelection');changed=draft();assert changed['parameters']['support']==support and 'loopOffsets' not in changed['parameters']
    assert session()['draftHistory']['index']==before['draftHistory']['index']+1
    call('capture.screenshot',{'path':str(out/'deleted.png')});call('history.undo');assert draft()==original;call('capture.screenshot',{'path':str(out/'undone.png')});call('history.redo');assert draft()==changed
    call('sketch.finish');final=inspect();assert final['authored']['features'][:-2]==base['authored']['features'][:-2] and final['authored']['features'][-1]==other
    assert final['evaluation']['bodies']==base['evaluation']['bodies']
    path=out/'deleted.alloy';call('document.save',{'path':str(path)});call('document.open',{'path':str(path),'discard':True});assert inspect()['authored']==final['authored']
    call('capture.screenshot',{'path':str(out/'reopened.png')})
    # Consumed generated profiles must refuse before a draft/history publication.
    call('document.open',{'path':str(out/'native-before.alloy'),'discard':True})
    call('component.activate',{'definitionId':owner})
    call('solid.extrude',{'sketchId':target['id'],'profileId':profile,'distance':1,'units':'mm'})
    call('document.save',{'path':str(out/'consumed-before.alloy')})
    consumed=inspect();consumer=consumed['authored']['features'][-1];call('sketch.edit',{'sketchId':target['id']});call('sketch.tool',{'tool':'select'});call('sketch.selectionSet',{'profileIds':[profile]});before=session()
    call('sketch.deleteSelection',error='downstream_profile_in_use');assert session()==before;assert inspect()['authored']==consumed['authored'] and inspect()['evaluation']==consumed['evaluation']
    refusal=next(x['response'] for x in reversed(ledger) if x['command']=='sketch.deleteSelection')
    assert refusal['error']['diagnostics']['featureId']==consumer['id'] and refusal['error']['diagnostics']['profileId']==profile
    gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated'];call('sketch.cancel')
    gpu={k:gpu[k] for k in ('vendor','renderer','version','hardwareAccelerated','details') if k in gpu}
    (out/'receipt.json').write_text(json.dumps({'status':'pass','appSha256':hashlib.sha256(Path(a.app).read_bytes()).hexdigest(),'source':str(source_path),'sourceSha256':hashlib.sha256(source_path.read_bytes()).hexdigest(),'variant':'planarInsetV1','areaBeforeMm2':original_profile['areaMm2'],'areaAfterMm2':edited_profile['areaMm2'],'target':target['id'],'profileId':profile,'support':support,'gpu':gpu,'downstreamRefusal':refusal},indent=2)+'\n')
    (out/'transcript.json').write_text(json.dumps(ledger,indent=2)+'\n')
print('PASS: planar inset loop profile owner Delete, support edit, exact history/reopen, independent sibling and consumed refusal')
