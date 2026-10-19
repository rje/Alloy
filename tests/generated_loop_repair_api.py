"""SKT-162 real-entrypoint lifecycle on the named external policy fixtures."""
import copy,json,sys,hashlib
from pathlib import Path
from app_harness import application
app,fixtures,output=map(Path,sys.argv[1:4]);output.mkdir(parents=True,exist_ok=True)
with application(app,output) as (call,ledger):
 try:
  inspect=lambda:call('document.inspect')
  session=lambda:call('app.state')['activeSession']
  call('document.open',{'path':str(fixtures/'multi-face-one-inset.alloy')});base=inspect()
  features=base['authored']['features'];sketch=next(f for f in features if f['parameters'].get('loopOffsets'));sid=sketch['id'];old=sketch['parameters']['loopOffsets'][0]
  combine=next(f for f in features if f['type']=='boolean');delete=next(f for f in features if f['name']=='Delete face tool');split=next(f for f in features if f['name']=='Split tool');extrude=next(f for f in features if f['type']=='extrude')
  target=json.loads((fixtures/'replacement-boundary-new-owner.alloy').read_text());support=next(f for f in target['features'] if f['id']==sid)['parameters']['support'];choice={'support':support,'distanceMm':-1}
  args={'featureId':combine['id'],'targetBodyId':sketch['parameters']['support']['bodyId'],'toolBodyIds':[delete['bodyId']],'operation':'cut','keepTools':True}
  profile=lambda d:next(s for s in d['evaluation']['sketches'] if s['sketchId']==sid)['profiles'][0]
  oldp=profile(base)
  call('solid.combine',args,error='evaluation_failed');cached=session();assert inspect()==base and cached['failedEditRepair']['diagnostics']['code']=='support_unavailable'
  call('capture.screenshot',{'path':str(output/'cached.png')})
  call('sketch.redefinePlaneBegin',{'sketchId':sid});choosing=session();assert choosing['planeRedefinition']['mode']=='recreateSingleInsetDistinctFace' and all('support' in c['replacement'] for c in choosing['planeRedefinition']['choices'])
  for replacement,code in [({'support':support},'offset_confirmation_required'),({'plane':'XY','distanceMm':-1},'offset_confirmation_required'),({'support':sketch['parameters']['support'],'distanceMm':-1},'distinct_support_required')]:
   call('sketch.redefinePlanePreview',{'replacement':replacement},error=code);assert session()==choosing and inspect()==base
  preview=call('sketch.redefinePlanePreview',{'replacement':choice});assert preview['previewed'] and preview['confirmedDistanceMm']==-1 and len(preview['boundaryEdgeIds'])==4 and len(set(preview['boundaryEdgeIds']))==4
  newowner=preview['recreatedOwner'];assert newowner['id']!=old['id'] and newowner['outerLoopId']!=old['outerLoopId'];assert inspect()==base
  for replacement,code in [({'support':support},'offset_confirmation_required'),({'support':support,'distanceMm':1},'invalid_offset_distance'),({'support':support,'distanceMm':-1000},'inset_disconnected')]:
   previous=session();expectedInvalid=copy.deepcopy(previous);state=expectedInvalid['planeRedefinition']
   for key in ('replacement','referenceLabel','frame','geometry','documentDraft','recreatedOwner','boundaryEdgeIds','confirmedDistanceMm'):state.pop(key,None)
   state.update(previewed=False,status='confirmationRequired')
   call('sketch.redefinePlanePreview',{'replacement':replacement},error=code);invalid=session();assert invalid==expectedInvalid and inspect()==base
   gpu=call('app.state')['viewport']['gpu'];assert not gpu['bodyStageKey'] and gpu['sketchProfiles'][0]['profileId']==oldp['profileId'] and gpu['sketchProfiles'][0]['frame']==oldp['frame']
   call('sketch.redefinePlaneCommit',error='replacement_required');assert session()==invalid and inspect()==base
   call('capture.screenshot',{'path':str(output/'invalidated.png')})
   preview=call('sketch.redefinePlanePreview',{'replacement':choice});assert preview['previewed'] and inspect()==base
  call('view.orientation',{'name':'isometric'});call('view.fit');call('capture.screenshot',{'path':str(output/'preview.png')})
  call('sketch.redefinePlaneCancel');assert session()==cached and inspect()==base
  call('sketch.redefinePlaneBegin',{'sketchId':sid});preview=call('sketch.redefinePlanePreview',{'replacement':choice});call('sketch.redefinePlaneCommit');after=inspect();assert session()=={} and call('history.failedEditInspect')=={}
  newsketch=next(f for f in after['authored']['features'] if f['id']==sid);expected=copy.deepcopy(sketch);expected['parameters']['support']=support;expected['parameters']['loopOffsets']=[preview['recreatedOwner']];assert newsketch==expected
  newp=profile(after);assert newp['profileId']!=oldp['profileId'] and abs(newp['areaMm2']-874)<1e-6 and newp['sourceLoopId']==preview['recreatedOwner']['outerLoopId']
  assert not {e['edgeId'] for e in newp['boundaryEdges']} & {e['edgeId'] for e in oldp['boundaryEdges']}
  assert after['undo']['count']==base['undo']['count']+1
  call('capture.screenshot',{'path':str(output/'committed.png')});call('history.undo');assert inspect()['authored']==base['authored'];call('capture.screenshot',{'path':str(output/'undo.png')});call('history.redo');assert inspect()['authored']==after['authored']
  call('document.save',{'path':str(output/'repaired.alloy')});call('document.open',{'path':str(output/'repaired.alloy')});assert inspect()['authored']==after['authored'] and profile(inspect())==newp
  call('document.open',{'path':str(fixtures/'replacement-boundary-new-owner.alloy')});independent=profile(inspect());assert all(newp[k]==independent[k] for k in ('areaMm2','sourceAreaMm2','edgeCount','frame','sourceLoopId','sourceFaceId','signedDistanceMm')) and newp['profileId']!=independent['profileId']
  call('document.open',{'path':str(fixtures/'multi-face-one-inset-consumer.alloy')});consumer=inspect();call('solid.combine',args,error='evaluation_failed');consumerCached=session();call('sketch.redefinePlaneBegin',{'sketchId':sid});beforeRefusal=session();call('sketch.redefinePlanePreview',{'replacement':choice},error='generated_profile_consumer');assert session()==beforeRefusal and inspect()==consumer;call('sketch.redefinePlaneCancel');assert session()==consumerCached;call('command.cancel',{'session':consumerCached['id']});assert inspect()==consumer
  call('document.open',{'path':str(fixtures/'multi-face-one-inset.alloy')});base2=inspect();args['toolBodyIds']=[split['bodyId']];call('solid.combine',args,error='evaluation_failed');incomplete=session();assert incomplete['failedEditRepair']['diagnostics']['code']=='ambiguous_inset_lineage';call('sketch.redefinePlaneBegin',{'sketchId':sid},error='repair_unavailable');assert session()==incomplete and inspect()==base2;call('command.cancel',{'session':incomplete['id']});assert inspect()==base2
  call('feature.edit',{'featureId':extrude['id'],'parameters':{'distanceMm':6}});same=inspect();assert next(f for f in same['authored']['features'] if f['id']==sid)==sketch and profile(same)['profileId']==oldp['profileId'] and profile(same)['frame']['origin'][2]==6
  gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated']
  (output/'receipt.json').write_text(json.dumps({'status':'pass','appSha256':hashlib.sha256(app.read_bytes()).hexdigest(),'preview':preview,'before':base,'after':after,'gpu':gpu,'limits':['Qt input separate','physical input and public parity unknown','Windows/Linux runtime unqualified']},indent=2)+'\n')
 finally:(output/'transcript.json').write_text(json.dumps(ledger,indent=2)+'\n')
print('PASS generated replacement repair API, Cancel, atomic history/reopen, consumer refusal, incomplete and same-boundary regressions')
