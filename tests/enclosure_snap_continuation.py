"""Continue the accepted from-blank enclosure through retained split and snaps.

Every modeling step uses Alloy's local control commands. The independently
accepted diagnostic .alloy files are never loaded or injected here.
"""
import math
import uuid
from pathlib import Path
from stl_export_api import inspect_stl


def ref(face):
    return {key: face[key] for key in ('producerFeatureId', 'bodyId', 'faceId')}


def near(value, target, tolerance=1e-5):
    return abs(value-target) <= tolerance


def vector_near(actual, expected, tolerance=1e-5):
    return len(actual)==len(expected) and all(near(a,b,tolerance) for a,b in zip(actual,expected))


def local_point(frame, world):
    delta=[world[i]-frame['origin'][i] for i in range(3)]
    return tuple(sum(delta[i]*frame[axis][i] for i in range(3)) for axis in ('uAxis','vAxis'))


def current_body(state, body_id):
    body=next(b for b in state['evaluation']['bodies'] if b['bodyId']==body_id)
    assert body['valid'] and body['closed'] and body['topology']['solids']==1, body
    return body


def unique(items, label):
    assert len(items)==1, (label, len(items), items)
    return items[0]


def segment(edge, a, b, tolerance=1e-5):
    start,end=edge.get('startMm'),edge.get('endMm')
    return bool(start and end and
        ((vector_near(start,a,tolerance) and vector_near(end,b,tolerance)) or
         (vector_near(start,b,tolerance) and vector_near(end,a,tolerance))))


def current_edge(body, a, b, label):
    return unique([edge for edge in body['solidEdges'] if edge.get('available',True) and segment(edge,a,b)],label)


def current_face(body, axis, coordinate, normal, label):
    return unique([face for face in body['supportFaces'] if face.get('planar',True)
        and near(face['frame']['origin'][axis],coordinate)
        and near(face['frame']['normal'][axis],normal)],label)


def feature_id(state, kind):
    feature=state['authored']['features'][-1]
    assert feature['type']==kind, feature
    return feature['id']


def draft(call):
    return call('app.state')['activeSession']['draftSketch']


def continue_snaps(*, call, stage, inspect, out, case_owner, case_body_id,
                   reference_id, analyses):
    out=Path(out)
    stages=[]
    def mark(text, capture=None):
        stages.append(text);stage(text,capture)
    def case(state= None):
        return current_body(state or inspect(),case_body_id)
    def find_board(name):
        return unique([b for b in inspect()['evaluation']['bodies'] if b['name']==name],name)
    def check_pair(state,bottom_id,top_id):
        bottom=current_body(state,bottom_id);top=current_body(state,top_id)
        assert bottom['boundsMm']['max'][2]<=15.0001 and top['boundsMm']['min'][2]>=14.9999
        return bottom,top

    base=inspect();assert base['bodies']==13 and case(base)['ownerId']==case_owner
    before_volume=case(base)['volumeMm3']
    top_face=current_face(case(base),2,30,+1,'outer top')
    bottom_face=current_face(case(base),2,0,-1,'outer bottom')
    mark('Construct Midplane between exact outer Case top and bottom')
    plane=call('construction.midplane',{'faces':[ref(top_face),ref(bottom_face)],'name':'Case Midplane'})['planeId']
    mark('Split the ported Case at the retained Midplane')
    call('solid.splitBody',{'targetBodyId':case_body_id,'planeId':plane,'name':'Case Split'})
    split=inspect();split_feature=split['authored']['features'][-1]
    assert split_feature['type']=='splitBody' and split_feature['parameters']['planeId']==plane
    top_id=split_feature['bodyId'];bottom_id=case_body_id
    bottom,top=check_pair(split,bottom_id,top_id)
    assert near(bottom['volumeMm3']+top['volumeMm3'],before_volume,1e-4)
    call('body.rename',{'bodyId':bottom_id,'name':'Bottom'})
    call('body.rename',{'bodyId':top_id,'name':'Top'})
    named=inspect();bottom,top=check_pair(named,bottom_id,top_id)
    assert bottom['name']=='Bottom' and top['name']=='Top'
    assert named['authored']['features']==split['authored']['features']
    call('feature.visibility',{'featureId':plane,'visible':False})
    call('feature.visibility',{'featureId':split_feature['id'],'visible':False})
    call('view.home');call('view.orientation',{'name':'left'});call('view.zoom',{'factor':1.3})
    mark('Named Bottom and Top halves; Top hidden for catch sketch','snap-split-bottom.png')

    # The adapted bottom catch lies at x=-32,y=20,z=11. The original seam at
    # z=15 and an actual imported CORE edge locate its 4 mm construction guide.
    bottom=case();outer=current_face(bottom,0,-32,-1,'Bottom outer long wall')
    seam=current_edge(bottom,(-32,41.5,15),(-32,-41.5,15),'current Split seam')
    core=find_board('CORE-001')
    imported=unique([edge for edge in core['solidEdges']
        if edge.get('provenance',{}).get('sourceEdgeIndex')==5
        and segment(edge,(8,.5,7.6),(8,.5,10))], 'retained CORE inner edge')
    mark('Sketch on Bottom long wall; Project Split seam and imported inner-component edge')
    call('sketch.begin',{'support':ref(outer),'name':'Bottom snap catch guide'})
    call('sketch.projectBegin')
    call('sketch.projectSelect',{'bodyId':bottom_id,'edgeId':seam['edgeId']})
    call('sketch.projectSelect',{'bodyId':core['bodyId'],'edgeId':imported['edgeId']})
    call('sketch.projectCommit')
    projected=draft(call)['parameters']['projections'];assert len(projected)==2
    seam_projection=unique([p for p in projected if p['source']['edgeId']==seam['edgeId']],'seam projection')
    core_projection=unique([p for p in projected if p['source']['edgeId']==imported['edgeId']],'imported edge projection')
    sketch_frame=call('app.state')['activeSession']['resolvedFrame']
    start=local_point(sketch_frame,(-32,20,15));end=local_point(sketch_frame,(-32,20,11))
    call('sketch.line',{'x1':start[0],'y1':start[1],'x2':end[0],'y2':end[1],'units':'mm'})
    guide_entity=draft(call)['parameters']['entities'][-1];guide_id=guide_entity['id']
    call('sketch.construction',{'entityIds':[guide_id],'construction':True})
    call('sketch.constraint',{'type':'vertical','entityIds':[guide_id]})
    call('sketch.constraint',{'type':'pointOnCurve','pointIds':[guide_entity['startPointId']],
                              'entityIds':[seam_projection['id']]})
    call('sketch.dimension',{'kind':'linear','entityId':guide_id,'value':4,'units':'mm'})
    call('sketch.dimension',{'kind':'pointLineDistance','pointIds':[guide_entity['endPointId']],
                             'entityId':core_projection['id'],'value':19.5,'units':'mm'})
    resolved=draft(call)['parameters'];guide_end=unique([p for p in resolved['points'] if p['id']==guide_entity['endPointId']],'solved guide end')
    assert near(guide_end['uMm'],end[0],1e-4) and near(guide_end['vMm'],end[1],1e-4)
    call('sketch.centerRectangle',{'centerX':guide_end['uMm'],'centerY':guide_end['vMm'],
        'width':6,'height':3,'units':'mm','centerSnapTarget':{'kind':'point','pointId':guide_entity['endPointId']}})
    catch_sketch_id=draft(call)['id']
    assert len(draft(call)['parameters']['dimensions'])>=4
    mark('4 mm guide and retained imported-edge positioning dimension; 6 × 3 catch','snap-catch-sketch.png')
    call('sketch.finish')
    catch_sketch=unique([s for s in inspect()['evaluation']['sketches'] if s['sketchId']==catch_sketch_id],'catch sketch')
    catch_profile=unique([p for p in catch_sketch['profiles'] if near(p['areaMm2'],18)],'6x3 catch profile')
    inner=current_face(case(),0,-28.5,+1,'Bottom inner wall')
    before_cut=case()['volumeMm3']
    mark('Cut the bottom catch To Object at the actual inner wall')
    call('solid.extrude',{'sketchId':catch_sketch_id,'profileId':catch_profile['profileId'],
         'extent':'toObject','endSupport':ref(inner),'operation':'cut','targetBodyIds':[bottom_id]})
    cut=inspect();cut_id=feature_id(cut,'extrude');bottom=current_body(cut,bottom_id)
    assert near(before_cut-bottom['volumeMm3'],63,1e-4)
    call('view.home');call('view.orientation',{'name':'left'});call('view.zoom',{'factor':1.45})
    mark('Retained 6 × 3 To Object catch in Bottom','snap-bottom-catch-cut.png')
    call('feature.visibility',{'featureId':split_feature['id'],'visible':True})
    width_section=unique([a for a in analyses if a['name']=='Width'],'retained Width section')
    call('analysis.visibility',{'analysisId':width_section['id'],'visible':True})
    mark('Show Top and inspect the catch with the retained Width section',
         'snap-width-section.png')
    call('analysis.visibility',{'analysisId':width_section['id'],'visible':False})
    call('feature.visibility',{'featureId':reference_id,'visible':False})

    # Build the upper catch from the three current sides of that actual Cut.
    # The omitted fourth side remains a normal authored sketch edge, so the
    # resulting regions stay linked to the Cut rather than a copied rectangle.
    side=current_face(bottom,1,17,+1,'current cut side for upper catch')
    projected_segments=[
        ((-32,17,9.5),(-28.5,17,9.5)),
        ((-28.5,17,9.5),(-28.5,17,12.5)),
        ((-32,17,9.5),(-32,17,12.5)),
    ]
    linked=[unique([edge for edge in side['edges'] if segment(edge,*pair)],
                   f'current cut boundary {pair}') for pair in projected_segments]
    mark('Offset a retained cut-side plane by 0.5 mm')
    catch_plane=call('construction.plane',{'source':{'support':ref(side)},
        'offset':.5,'units':'mm','name':'Catch side +0.5'})['planeId']
    mark('Project three current Cut edges on the offset plane')
    call('sketch.begin',{'planeId':catch_plane,'name':'Retained cutout projection'})
    call('sketch.projectBegin')
    for edge in linked:
        call('sketch.projectSelect',{'bodyId':bottom_id,'edgeId':edge['edgeId']})
    call('sketch.projectCommit')
    projected=draft(call)['parameters']['projections']
    assert {p['source']['edgeId'] for p in projected}=={e['edgeId'] for e in linked}
    # Local plane coordinates are derived from the retained cut-side frame:
    # u follows world Z and v follows world X. The stepped segments are the
    # documented 8/2/10/6/2 sequence, closed by the final segment.
    outline=[(3,3.5),(11,3.5),(11,5.5),(1,5.5),(1,-.5),(3,-.5),(3,3.5)]
    lengths=[8,2,10,6,2]
    inner_projection=unique([p for p in projected if p['source']['edgeId']==linked[1]['edgeId']],
                            'projected inner cut edge')
    inner_edge=linked[1]
    at_end=vector_near(inner_edge['endMm'],(-28.5,17,12.5))
    projected_corner=str(uuid.uuid5(uuid.UUID(inner_projection['id']),
                                   'end' if at_end else 'start'))
    for index,(start,end) in enumerate(zip(outline,outline[1:])):
        inputs={'x1':start[0],'y1':start[1],'x2':end[0],'y2':end[1],'units':'mm'}
        if index==0:
            inputs['startSnapTarget']={'kind':'point','pointId':projected_corner}
        call('sketch.line',inputs)
    upper_sketch_id=draft(call)['id']
    entities=draft(call)['parameters']['entities']
    assert len(entities)==6 and len(projected)==3
    assert [math.dist(a,b) for a,b in list(zip(outline,outline[1:]))[:5]]==lengths
    mark('Draw retained stepped 8/2/10/6/2 upper catch and close it','snap-upper-catch-sketch.png')
    call('sketch.finish')
    upper_sketch=unique([s for s in inspect()['evaluation']['sketches']
        if s['sketchId']==upper_sketch_id],'upper catch sketch')
    region_areas=[(p['areaMm2'],p['profileId']) for p in upper_sketch['profiles']]
    assert sorted(round(area,4) for area,_ in region_areas)==[1,3.5,7,20],region_areas
    selected=[profile_id for area,profile_id in region_areas if any(near(area,target) for target in (1,7,20))]
    assert len(selected)==3
    call('feature.visibility',{'featureId':bottom['featureId'],'visible':False})
    call('solid.extrude',{'sketchId':upper_sketch_id,'profileIds':selected,
        'distance':5,'units':'mm','operation':'join','targetBodyIds':[top_id]})
    joined=inspect();join_id=feature_id(joined,'extrude')
    assert joined['authored']['features'][-1]['parameters']['profileIds']==selected
    assert current_body(joined,top_id)['volumeMm3']>top['volumeMm3']
    mark('Join 5 mm from three selected regions into Top','snap-upper-joined.png')

    def join_edge(state, role, start, end, area=None):
        candidates=[]
        for edge in current_body(state,top_id)['solidEdges']:
            provenance=edge.get('provenance',{})
            if not edge.get('available') or provenance.get('generatorFeatureId')!=join_id:
                continue
            if provenance.get('role')!=role or not segment(edge,start,end):
                continue
            if area is not None and provenance.get('sourceProfileId') not in {
                    profile_id for value,profile_id in region_areas if near(value,area)}:
                continue
            candidates.append(edge)
        return unique(candidates,f'current Join {role} area {area}')

    # Each finish resolves its edge on the current shape. Distinct source-cell
    # provenance disambiguates coincident end rims from the selected regions.
    bottom_front=join_edge(joined,'endRim',(-32.5,22.5,10.5),(-26.5,22.5,10.5),7)
    mark('Chamfer lower front catch edge 2 mm')
    call('solid.chamfer',{'targetBodyId':top_id,'edgeIds':[bottom_front['edgeId']],
        'distanceMm':2})
    chamfer_bottom_id=feature_id(inspect(),'chamfer')
    top_front=join_edge(inspect(),'endRim',(-28.5,22.5,20.5),(-26.5,22.5,20.5),20)
    mark('Chamfer upper front catch edge 1 mm')
    call('solid.chamfer',{'targetBodyId':top_id,'edgeIds':[top_front['edgeId']],
        'distanceMm':1})
    chamfer_top_id=feature_id(inspect(),'chamfer')
    root_edge=join_edge(inspect(),'join.root',(-28.5,17.5,15),(-28.5,17.5,20.5),20)
    mark('Chamfer exact retained inner attachment edge 2 mm')
    call('solid.chamfer',{'targetBodyId':top_id,'edgeIds':[root_edge['edgeId']],
        'distanceMm':2})
    chamfer_root_id=feature_id(inspect(),'chamfer')
    inner_snap=join_edge(inspect(),'sideCorner',(-28.5,17.5,12.5),(-28.5,22.5,12.5),7)
    mark('Fillet the retained inner snap edge 0.5 mm')
    call('solid.fillet',{'targetBodyId':top_id,'edgeIds':[inner_snap['edgeId']],
        'radiusMm':.5,'tangentChain':True})
    top_fillet_id=feature_id(inspect(),'fillet')
    upper_group=[join_id,chamfer_bottom_id,chamfer_top_id,chamfer_root_id,top_fillet_id]
    assert [f['id'] for f in inspect()['authored']['features'][-5:]]==upper_group
    call('feature.visibility',{'featureId':bottom['featureId'],'visible':True})
    bottom_inner=current_edge(current_body(inspect(),bottom_id),
        (-28.5,17,12.5),(-28.5,23,12.5),'current Bottom upper inner Cut rim')
    assert bottom_inner['provenance']['role']=='cutEndRim'
    mark('Fillet the original Bottom inner cutout edge 0.5 mm')
    call('solid.fillet',{'targetBodyId':bottom_id,'edgeIds':[bottom_inner['edgeId']],
        'radiusMm':.5,'tangentChain':True})
    bottom_fillet_id=feature_id(inspect(),'fillet')
    mark('Mirror the earlier Bottom Cut alone across the origin plane')
    call('solid.featureMirror',{'targetBodyId':bottom_id,'sourceFeatureIds':[cut_id],
        'mirrorPlane':{'kind':'origin','plane':'YZ'}})
    cut_mirror_id=feature_id(inspect(),'featureMirror')
    mark('Mirror the ordered Top Join, three Chamfers and first Fillet as Features')
    call('solid.featureMirror',{'targetBodyId':top_id,'sourceFeatureIds':upper_group,
        'mirrorPlane':{'kind':'origin','plane':'YZ'}})
    upper_mirror_id=feature_id(inspect(),'featureMirror')
    mirrored=inspect()
    assert mirrored['authored']['features'][-1]['parameters']['sourceFeatureIds']==upper_group

    mark('Move the Bottom inner Fillet after both feature Mirrors')
    earlier_order=[f['id'] for f in mirrored['authored']['features']]
    moved=call('feature.reorder',{'featureId':bottom_fillet_id,
                                  'afterFeatureId':upper_mirror_id})
    assert moved['changed'] and moved['movedFeatureId']==bottom_fillet_id
    reordered=inspect()
    ordered=[f['id'] for f in reordered['authored']['features']]
    assert set(ordered)==set(earlier_order)
    assert ordered.index(bottom_fillet_id)==ordered.index(upper_mirror_id)+1
    assert ordered.index(cut_mirror_id)<ordered.index(upper_mirror_id)
    mirrored_inner=current_edge(current_body(reordered,bottom_id),
        (28.5,17,12.5),(28.5,23,12.5),'mirrored current Bottom upper inner Cut rim')
    assert mirrored_inner['provenance']['role']=='cutEndRim'
    groups=[{'id':str(uuid.uuid4()),'edgeIds':[edge_id],
             'radiusMm':.5,'tangentChain':True}
            for edge_id in (bottom_inner['edgeId'],mirrored_inner['edgeId'])]
    mark('Edit the moved Bottom Fillet to two separate 0.5 mm inner-edge groups')
    call('solid.fillet',{'featureId':bottom_fillet_id,'targetBodyId':bottom_id,
        'groups':groups})
    finished_geometry=inspect()
    final_bottom=current_body(finished_geometry,bottom_id)
    final_top=current_body(finished_geometry,top_id)
    retained=unique([f for f in finished_geometry['authored']['features']
                     if f['id']==bottom_fillet_id],'moved Bottom Fillet')
    assert retained['parameters']['groups']==groups
    assert final_bottom['volumeMm3']>0 and final_top['volumeMm3']>0
    call('history.undo')
    undone=inspect()
    assert next(f for f in undone['authored']['features']
                if f['id']==bottom_fillet_id)['parameters']!=retained['parameters']
    call('history.redo')
    assert inspect()['authored']==finished_geometry['authored']
    call('feature.visibility',{'featureId':catch_plane,'visible':False})
    call('feature.visibility',{'featureId':catch_sketch_id,'visible':False})
    call('feature.visibility',{'featureId':upper_sketch_id,'visible':False})
    call('feature.visibility',{'featureId':reference_id,'visible':True})
    call('view.home');call('view.orientation',{'name':'front'});call('view.zoom',{'factor':2})
    mark('Both retained snap groups on the full Case','snap-final-front.png')
    top_section=unique([a for a in call('analysis.inspect')['analyses']
                        if a['name']=='Top'],'retained Top section')
    if top_section['availability']!='available':
        # The original face-linked analysis can lose that face after Split.
        # A retained origin plane at its documented position is a disclosed
        # inspection adaptation, not a repaired face reference.
        call('inspect.section',{'plane':'XY','name':'Top fit inspection'})
        fallback=unique([a for a in call('analysis.inspect')['analyses']
                         if a['name']=='Top fit inspection'],'origin Top section')
        call('inspect.section',{'analysisId':fallback['id'],'offsetMm':24.5,'flip':True})
        top_section=unique([a for a in call('analysis.inspect')['analyses']
                            if a['id']==fallback['id']],'resolved origin Top section')
        assert top_section['availability']=='available'
        originally_intended=unique([a for a in analyses if a['name']=='Top'],
                                   'pre-Split Top section')['resolvedPlane']
        assert top_section['resolvedPlane']==originally_intended
    call('analysis.visibility',{'analysisId':top_section['id'],'visible':True})
    assert call('app.state')['viewport']['gpu']['sectionClippedActors']>0
    mark('Inspect fitted snaps with available Top-position section and linked board',
         'snap-top-section.png')
    call('analysis.visibility',{'analysisId':top_section['id'],'visible':False})
    finished=inspect()

    mark('Save, reopen and verify both current bodies and authored feature order')
    design_path=out/'enclosure-through-snaps.alloy'
    call('document.save',{'path':str(design_path)})
    call('document.new',{'discard':True})
    call('document.open',{'path':str(design_path),'discard':True})
    reopened=inspect()
    assert reopened['authored']==finished['authored']
    for body_id,expected in ((bottom_id,final_bottom),(top_id,final_top)):
        actual=current_body(reopened,body_id)
        assert near(actual['volumeMm3'],expected['volumeMm3'],1e-4)
    assert {current_body(reopened,body_id)['name'] for body_id in (bottom_id,top_id)}=={'Bottom','Top'}

    mark('Export each retained final half as a separate printable solid')
    # The halves share their split-seam face. A combined STL contains
    # coincident triangles at that seam; each part is a manifold print file.
    stl_exports=[]
    step_exports=[]
    for label,body_id in (('Bottom',bottom_id),('Top',top_id)):
        stl_path=out/f'{label.lower()}-snap-mm.stl'
        stl_export=call('document.exportStl',{'path':str(stl_path),
            'bodyId':body_id,'units':'mm','format':'binary',
            'surfaceDeviationMm':.1,'normalDeviationDeg':10})
        mesh=inspect_stl(stl_path,True,allow_cavities=True)
        assert len(mesh['components'])==1
        stl_exports.append({'name':label,'export':stl_export,'readback':mesh})
        exported=call('document.exportStep',{'path':str(out/f'{label.lower()}-snap-mm.step'),
            'bodyId':body_id,'units':'mm'})
        assert near(exported['volumeMm3'],current_body(reopened,body_id)['volumeMm3'],1e-4)
        step_exports.append(exported)

    return {'stages':stages,'planeId':plane,'splitId':split_feature['id'],'bottomBodyId':bottom_id,
            'topBodyId':top_id,'guideEntityId':guide_id,'catchSketchId':catch_sketch_id,
            'importedPositionEdgeId':imported['edgeId'],'cutFeatureId':cut_id,
            'catchPlaneId':catch_plane,'upperSketchId':upper_sketch_id,'joinFeatureId':join_id,
            'upperGroupFeatureIds':upper_group,'bottomFilletId':bottom_fillet_id,
            'cutMirrorId':cut_mirror_id,'upperMirrorId':upper_mirror_id,
            'oppositeBottomEdgeId':mirrored_inner['edgeId'],
            'finalVolumesMm3':{'Bottom':final_bottom['volumeMm3'],'Top':final_top['volumeMm3']},
            'stlExports':stl_exports,'stepExports':step_exports,
            'finalSection':{'analysisId':top_section['id'],
                            'originFallback':top_section['name']=='Top fit inspection'},
            'source':'from-blank commands through retained source-order snap groups and final two-half exports'}
