#!/usr/bin/env python3
"""Generated support-face annulus Delete through the real shared commands."""
import argparse,json,hashlib
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
with application(a.app,out) as (call,ledger):
    inspect=lambda:call('document.inspect')
    session=lambda:call('app.state')['activeSession']
    draft=lambda:session()['draftSketch']
    call('document.open',{'path':str(Path(__file__).parent.resolve()/'fixtures/rounded-rectangle-source.alloy')})
    initial=inspect();source=initial['evaluation']['sketches'][0]
    call('solid.extrude',{'sketchId':source['sketchId'],'profileId':source['profiles'][0]['profileId'],'distance':2,'units':'mm'})
    body=inspect()['evaluation']['bodies'][0];cap=next(f for f in body['supportFaces'] if f['role']=='extrude.endCap');support={k:cap[k] for k in ('producerFeatureId','bodyId','faceId')}
    for name in ('Selected loop','Independent loop'):
        call('sketch.begin',{'support':support,'name':name,'units':'mm'});call('sketch.tool',{'tool':'offset'})
        edge=next(e for e in body['solidEdges'] if e['provenance']['role']=='endRim');pt=call('view.projectEdge',{'bodyId':body['bodyId'],'edgeId':edge['edgeId']})
        call('sketch.placePoint',{'x':pt['x'],'y':pt['y']});call('sketch.loopOffsetAccept');call('sketch.finish')
    other_id=inspect()['authored']['features'][-1]['id'];call('feature.visibility',{'featureId':other_id,'visible':False})
    base=inspect();target=base['authored']['features'][-2];other=base['authored']['features'][-1]
    profile=next(s for s in base['evaluation']['sketches'] if s['sketchId']==target['id'])['profiles'][0]['profileId']
    other_profile=next(s for s in base['evaluation']['sketches'] if s['sketchId']==other['id'])['profiles'][0]['profileId']
    # Retained support edit reevaluates the same identity before deletion.
    src=base['authored']['features'][0];radius=next(d for d in src['parameters']['dimensions'] if d['kind']=='radius')
    call('sketch.edit',{'sketchId':src['id']});call('sketch.dimensionUpdate',{'dimensionId':radius['id'],'value':5.5,'units':'mm'});call('sketch.finish')
    base=inspect();assert next(s for s in base['evaluation']['sketches'] if s['sketchId']==target['id'])['profiles'][0]['profileId']==profile
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
    call('solid.extrude',{'sketchId':target['id'],'profileId':profile,'distance':1,'units':'mm'})
    consumed=inspect();consumer=consumed['authored']['features'][-1];call('sketch.edit',{'sketchId':target['id']});call('sketch.tool',{'tool':'select'});call('sketch.selectionSet',{'profileIds':[profile]});before=session()
    call('sketch.deleteSelection',error='downstream_profile_in_use');assert session()==before;assert inspect()['authored']==consumed['authored'] and inspect()['evaluation']==consumed['evaluation']
    refusal=next(x['response'] for x in reversed(ledger) if x['command']=='sketch.deleteSelection')
    assert refusal['error']['diagnostics']['featureId']==consumer['id'] and refusal['error']['diagnostics']['profileId']==profile
    gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated'];call('sketch.cancel')
    (out/'receipt.json').write_text(json.dumps({'status':'pass','appSha256':hashlib.sha256(Path(a.app).read_bytes()).hexdigest(),'variant':'classic annulus','target':target['id'],'profileId':profile,'support':support,'gpu':gpu,'downstreamRefusal':refusal},indent=2)+'\n')
    (out/'transcript.json').write_text(json.dumps(ledger,indent=2)+'\n')
print('PASS: loop profile owner Delete, support edit, exact history/reopen, independent sibling and consumed refusal')
