#!/usr/bin/env python3
"""Delete local linked line/circle/point records through the real application."""
import argparse,hashlib,json
from pathlib import Path
from app_harness import application
p=argparse.ArgumentParser();p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args();out=Path(a.output).resolve()
def ref(f):return {k:f[k] for k in ('producerFeatureId','bodyId','faceId')}
with application(a.app,out) as (call,ledger):
 def session():return call('app.state')['activeSession']
 def draft():return session()['draftSketch']
 def params():return draft()['parameters']
 def unchanged(before):
  after=session()
  for key in ('draftSketch','draftHistory','selection','projectionContext'):assert after.get(key)==before.get(key),key
 # Reuse the accepted linked circular Project source fixture.
 fixture=Path(__file__).resolve().parents[2]/'Alloy-reference-records/deliverables/enclosure-adapted-input-v2-2026-10-13/adapted-board-envelope.step'
 call('reference.insert',{'path':str(fixture),'name':'Circle source'})
 call('component.create',{'name':'Delete fixture','activate':True})
 call('solid.box',{'width':20,'depth':15,'height':5,'x':80,'units':'mm','name':'Support'})
 call('solid.box',{'width':6,'depth':6,'height':5,'x':110,'y':8,'units':'mm','name':'Point source'})
 baseline=call('document.inspect');bodies=baseline['evaluation']['bodies'];support=next(b for b in bodies if b['name']=='Support');point_body=next(b for b in bodies if b['name']=='Point source');cylinder=next(b for b in bodies if b['name']=='PCB-001')
 top=next(f for f in support['supportFaces'] if f['role']=='box.z1')
 point_edge=next(e for e in point_body['solidEdges'] if e['provenance'].get('boxEdge')=='x0y0z*')
 rim=next(e for e in cylinder['solidEdges'] if e['provenance'].get('sourceEdgeIndex')==12)
 call('sketch.begin',{'support':ref(top),'name':'Linked Delete'})
 call('sketch.projectBegin');face_projection=call('sketch.projectFace',ref(top))
 preview=call('sketch.projectPreview');duplicate=call('sketch.projectFace',ref(top));assert duplicate['faceProjectionGroupId']==face_projection['faceProjectionGroupId'];assert duplicate['projectionIds']==face_projection['projectionIds'];assert call('sketch.projectPreview')==preview
 sibling_face=next(f for f in point_body['supportFaces'] if f['role']=='box.z1');sibling_projection=call('sketch.projectFace',ref(sibling_face))
 for b,e in [(point_body,point_edge),(cylinder,rim)]:call('sketch.projectSelect',{'bodyId':b['bodyId'],'edgeId':e['edgeId']})
 call('sketch.projectCommit');call('sketch.lookAt');call('view.fit')
 line=params()['projections'][0]['id'];circle=params()['circleProjections'][0]['id'];point=params()['pointProjections'][0]['id'];sketch_id=draft()['id']
 call('sketch.addCircle',{'centerX':35,'centerY':-12,'diameter':4,'units':'mm'});unrelated=params()['entities'][-1]
 call('sketch.finish');call('document.save',{'path':str(out/'native-before.alloy')});call('sketch.edit',{'sketchId':sketch_id});call('sketch.lookAt');call('view.fit')
 original=draft();context=session()['projectionContext']
 group=original['parameters']['faceProjectionGroups'][0];assert group['source']==ref(top) and group['id']==face_projection['faceProjectionGroupId'];assert group['projectionIds']==face_projection['projectionIds'];assert len(group['projectionIds'])==4
 sibling_group=original['parameters']['faceProjectionGroups'][1];assert sibling_group['id']==sibling_projection['faceProjectionGroupId'] and sibling_group['source']==ref(sibling_face)
 # Removing every result of one action retires only that now-empty membership.
 call('sketch.deleteSelection',{'entityIds':group['projectionIds']});assert params()['faceProjectionGroups']==[sibling_group];call('history.undo');assert draft()==original
 # A retained rectangle Offset consumes this face's boundary identities.
 call('sketch.tool',{'tool':'offset'});call('sketch.offsetPreview',{'sourceEntityIds':group['projectionIds'],'distance':1,'units':'mm'});call('sketch.offsetAccept')
 before=session();doc=call('document.inspect');call('sketch.deleteSelection',{'entityIds':[line]},error='sketch_solve_failed');face_dependency_refusal=ledger[-1]['response'];unchanged(before);assert call('document.inspect')==doc
 call('history.undo');assert draft()==original
 # Request validation must leave draft, selection/history and document exact.
 for args in [{'entityIds':[line,'stale']},{'entityIds':[line,line]},{'pointIds':[point,point]},{'entityIds':[point]},{'pointIds':[line]}]:
  before=session();doc=call('document.inspect');call('sketch.deleteSelection',args,error='sketch_solve_failed');unchanged(before);assert call('document.inspect')==doc
 # A retained Circle Offset consumes the projection as geometry input.
 call('sketch.circleOffset',{'sourceEntityId':circle,'distance':2,'units':'mm'})
 before=session();call('sketch.deleteSelection',{'entityIds':[circle]},error='sketch_solve_failed');unchanged(before)
 call('history.undo');assert draft()==original
 # A read-only circle measurement is removable annotation; intrinsic diameter
 # is evaluated presentation and never an authored cascade target.
 call('sketch.dimension',{'kind':'diameter','entityId':circle,'driving':False});measured=draft()
 for kind,target,key,field in [('line',line,'projections','entityIds'),('circle',circle,'circleProjections','entityIds'),('point',point,'pointProjections','pointIds')]:
  before=draft();state=session();call('sketch.selectionSet',{field:[target]});call('capture.screenshot',{'path':str(out/(kind+'-selected.png'))});call('sketch.deleteSelection')
  changed=draft();expected=dict(before['parameters']);rows=[r for r in expected[key] if r['id']!=target]
  if rows:expected[key]=rows
  else:expected.pop(key)
  if kind=='line':expected['faceProjectionGroups']=[{**g,'projectionIds':[i for i in g['projectionIds'] if i!=target]} for g in expected['faceProjectionGroups'] if any(i!=target for i in g['projectionIds'])]
  if kind=='circle':expected['dimensions']=[d for d in expected['dimensions'] if d.get('entityId')!=circle]
  assert changed['parameters']==expected,(kind,changed['parameters'],expected)
  assert session()['draftHistory']['index']==state['draftHistory']['index']+1
  call('capture.screenshot',{'path':str(out/(kind+'-deleted.png'))});call('history.undo');assert draft()==before;assert session()['projectionContext']==state['projectionContext']
  call('capture.screenshot',{'path':str(out/(kind+'-undone.png'))});call('history.redo');assert draft()==changed;call('history.undo');assert draft()==before;assert session()['projectionContext']==state['projectionContext']
 # Keep the projected rectangle profile consumed; delete only unrelated point.
 call('sketch.finish');state=call('document.inspect');sketch=next(s for s in state['evaluation']['sketches'] if s['featureId']==sketch_id)
 profile=next(p for p in sketch['profiles'] if abs(p['areaMm2']-300)<1e-6)
 call('solid.extrude',{'sketchId':sketch_id,'profileId':profile['profileId'],'distance':3,'units':'mm','name':'Projected consumer'})
 committed=call('document.inspect');call('sketch.edit',{'sketchId':sketch_id});call('sketch.selectionSet',{'entityIds':[line]});before=session()
 call('sketch.deleteSelection',error='downstream_profile_in_use');refusal=ledger[-1]['response'];unchanged(before);assert call('document.inspect')==committed
 call('capture.screenshot',{'path':str(out/'downstream-refused.png')})
 call('sketch.deleteSelection',{'pointIds':[point]});call('sketch.finish');final=call('document.inspect');assert final['evaluation']['bodies']==committed['evaluation']['bodies']
 path=out/'projection-deleted.alloy';call('document.save',{'path':str(path)});call('document.open',{'path':str(path),'discard':True});assert call('document.inspect')['authored']==final['authored'];assert call('document.inspect')['evaluation']['bodies']==final['evaluation']['bodies']
 call('capture.screenshot',{'path':str(out/'reopened.png')});gpu=call('app.state')['viewport']['gpu'];assert gpu['hardwareAccelerated']
 (out/'receipt.json').write_text(json.dumps({'status':'pass','gpu':gpu,'appSha256':hashlib.sha256(Path(a.app).read_bytes()).hexdigest(),'sketchId':sketch_id,'lineId':line,'circleId':circle,'pointId':point,'faceProjectionGroup':group,'siblingFaceProjectionGroup':sibling_group,'selectedEdgePolicy':True,'duplicateFaceStable':True,'emptyGroupRetired':True,'faceDependencyRefusal':face_dependency_refusal,'sourceBodies':baseline['evaluation']['bodies'],'projectionContext':context,'refusal':refusal,'exactUndoRedoReopen':True,'nativeFixture':str(out/'native-before.alloy')},indent=2)+'\n')
 (out/'transcript.json').write_text(json.dumps(ledger,indent=2)+'\n')
print('PASS: linked line/circle/point Delete, atomic refusals, annotation cleanup, downstream preflight, exact history/reopen and GPU')
