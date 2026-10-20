#!/usr/bin/env python3
"""Independent SRF-028 real-app bilinear sheet geometry/history qualification."""
import argparse
import json
import hashlib
import math
from pathlib import Path
from app_harness import application


def sub(a, b): return [x-y for x, y in zip(a, b)]
def cross(a, b): return [a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]]
def dot(a, b): return sum(x*y for x, y in zip(a, b))
def near(a, b, tol=2e-5): assert math.isclose(a, b, rel_tol=0, abs_tol=tol), (a, b)
def corners(z): return [[0,0,0],[40,0,0],[0,30,0],[0,0,z]]


def area_oracle(z, n=240):
    # Composite midpoint integration of |Su cross Sv|, independently from OCCT.
    a,b,c,d=corners(z)
    total=0
    for i in range(n):
        u=(i+.5)/n
        for j in range(n):
            v=(j+.5)/n
            su=[(1-v)*(b[k]-a[k])+v*(c[k]-d[k]) for k in range(3)]
            sv=[(1-u)*(d[k]-a[k])+u*(c[k]-b[k]) for k in range(3)]
            total += math.sqrt(dot(cross(su,sv),cross(su,sv)))
    return total/n**2


def distance_segment(p, a, b):
    ab=sub(b,a); t=max(0,min(1,dot(sub(p,a),ab)/dot(ab,ab)))
    return math.dist(p,[a[k]+t*ab[k] for k in range(3)])


def sheet(body, z):
    assert body['type']=='surface' and body['valid'] and not body['closed'], body
    assert body['topology']['faces']==1 and body['topology']['solids']==0 and body['topology']['edges']==4, body
    assert body['boundaryLoopCount']==1 and body['closedBoundary'], body
    assert body['volumeMm3'] is None
    for actual,expected in zip(body['boundsMm']['min'],[0,0,0]): near(actual,expected)
    for actual,expected in zip(body['boundsMm']['max'],[40,30,z]): near(actual,expected)
    coarse,fine=area_oracle(z,120),area_oracle(z)
    assert abs(fine-coarse)<.02, (fine,coarse)
    near(body['areaMm2'],fine,.03)
    expected=list(zip(corners(z),corners(z)[1:]+corners(z)[:1]))
    deviations=[]; hull_bounds=[]; isos=set(); sources=set()
    for edge in body['solidEdges']:
        ends=[edge['startMm'],edge['endMm']]
        errors=[min(max(math.dist(x,y) for x,y in zip(ends,order)) for order in (pair,pair[::-1])) for pair in expected]
        idx=min(range(len(errors)),key=errors.__getitem__)
        assert errors[idx]<1e-6, errors
        a,b=expected.pop(idx)
        cert=edge['provenance']['boundaryCertificate']
        assert cert['curveType'] in ('line','bezier','bspline') and cert['degree']==1 and not cert['rational'], cert
        poles=cert['controlPointsMm'];assert len(poles)==2
        # A degree-one nonrational curve is exactly its two-pole convex hull.
        # Distance to a convex finite segment is convex, so the maximum pole
        # distance bounds EVERY boundary point; this is not a sampling claim.
        bound=max(distance_segment(p,a,b) for p in poles)
        assert bound<=.01, bound
        hull_bounds.append(bound)
        for endpoint in (a,b): assert min(math.dist(endpoint,p) for p in poles)<1e-6
        for endpoint in (a,b): assert min(math.dist(endpoint,p) for p in (cert['sourceStartMm'],cert['sourceEndMm']))<1e-6
        isos.add((cert['surfaceIso'],round(cert['isoParameter'])))
        sources.add((cert['sourceSketchId'],cert['sourceEntityId']))
        uv_a,uv_b=cert['pcurveStartUV'],cert['pcurveEndUV']
        fixed_axis=0 if cert['surfaceIso']=='u' else 1
        near(uv_a[fixed_axis],cert['isoParameter']);near(uv_b[fixed_axis],cert['isoParameter'])
        near(min(uv_a[1-fixed_axis],uv_b[1-fixed_axis]),0);near(max(uv_a[1-fixed_axis],uv_b[1-fixed_axis]),1)
        # Re-evaluate each pcurve endpoint against the actual result support poles.
        q=body['surfaceCertificate']['cornersMm']
        for u,v in (uv_a,uv_b):
            on_support=[(1-u)*(1-v)*q[0][k]+u*(1-v)*q[1][k]+u*v*q[2][k]+(1-u)*v*q[3][k] for k in range(3)]
            assert min(math.dist(on_support,p) for p in poles)<1e-6
        deviations.append(errors[idx])
    assert not expected
    assert isos=={('u',0),('u',1),('v',0),('v',1)} and len(sources)==4
    assert sources=={(r['sketchId'],r['entityId']) for r in body['boundaryEdges']}
    support=body['surfaceCertificate']
    assert support['type']=='bezier' and support['uDegree']==support['vDegree']==1 and not support['rational']
    for actual,want in zip(support['cornersMm'],corners(z)):
        assert math.dist(actual,want)<1e-6
    near(body['perimeterMm'],sum(math.dist(a,b) for a,b in zip(corners(z),corners(z)[1:]+corners(z)[:1])))
    face,=body['supportFaces']
    assert not face['planar'], face
    determinant=dot(sub(corners(z)[1],corners(z)[0]),cross(sub(corners(z)[2],corners(z)[0]),sub(corners(z)[3],corners(z)[0])))
    near(determinant,1200*z)
    return {'areaMm2':body['areaMm2'],'independentAreaMm2':fine,'quadratureRefinementMm2':abs(fine-coarse),'cornerDeterminantMm3':determinant,'endpointDeviationMm':max(deviations),'independentWholeBoundaryBoundMm':max(hull_bounds),'proof':'All four actual degree-one nonrational edge control hulls lie on corresponding finite source segments; actual surface degree-(1,1) complete natural isos cover the same four spans.'}


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--app',required=True);p.add_argument('--output',required=True);args=p.parse_args()
    out=Path(args.output).resolve();out.mkdir(parents=True,exist_ok=True)
    with application(args.app,out,command_timeout=90) as (call,transcript):
        try:
            def inspect(): return call('document.inspect')
            def draft(): return call('app.state')['activeSession']['draftSketch']
            def unchanged(before):
                after=inspect()
                for key in ('authored','revision','undo','evaluation'): assert after[key]==before[key],key
            call('window.resize',{'width':1280,'height':900})
            environment=call('app.state');assert environment['viewport']['gpu']['hardwareAccelerated']
            refs=[]; sketches=[]; entities=[]
            for plane,segments in [('XY',[(0,0,40,0),(40,0,0,30)]),('YZ',[(30,0,0,10),(0,10,0,0)])]:
                call('sketch.create',{'plane':plane,'name':'Alloy skew boundary '+plane})
                for coords in segments: call('sketch.line',{**dict(zip(('x1','y1','x2','y2'),coords)),'units':'mm'})
                source=draft();sketches.append(source['id']);entities.append(source['parameters']['entities'])
                refs.extend({'sketchId':source['id'],'entityId':e['id']} for e in entities[-1])
                call('sketch.finish')
            inputs={'boundaryMode':'fourEdges','boundaryEdges':refs,'continuity':'g0'}
            original=inspect()
            yz=next(s for s in original['evaluation']['sketches'] if s['sketchId']==sketches[1])
            for key,expected in [('origin',[0,0,0]),('uAxis',[0,1,0]),('vAxis',[0,0,1])]:
                for actual,want in zip(yz['frame'][key],expected): near(actual,want)
            for invalid in ({**inputs,'boundaryEdges':refs[:3]}, {**inputs,'boundaryEdges':refs[:3]+refs[:1]}, {**inputs,'continuity':'g1'}):
                call('surface.patch',invalid,error='invalid_arguments');unchanged(original)
            session=call('surface.patchDialog',inputs)
            preview=call('command.preview',{'session':session['id']})
            measurements={'preview':sheet(preview['geometry'],10)}
            unchanged(original);call('view.home');call('capture.screenshot',{'path':str(out/'preview.png')})
            call('command.cancel',{'session':session['id']});unchanged(original)
            call('surface.patch',inputs);committed=inspect();feature=committed['authored']['features'][-1];body_id=feature['bodyId']
            def body(state): return next(b for b in state['evaluation']['bodies'] if b['bodyId']==body_id)
            measurements['committed']=sheet(body(committed),10)
            assert committed['evaluation']['sketches']==original['evaluation']['sketches']
            assert len(committed['evaluation']['bodies'])==1
            assert feature['parameters']['boundaryEdges']==refs
            assert feature['parameters']['continuity']=='g0'
            call('capture.screenshot',{'path':str(out/'committed.png')})
            call('history.undo');assert inspect()['authored']==original['authored']
            call('history.redo');assert inspect()['authored']==committed['authored']
            edit=call('feature.editDialog',{'featureId':feature['id']});sheet(call('command.preview',{'session':edit['id']})['geometry'],10);call('command.cancel',{'session':edit['id']})
            call('sketch.edit',{'sketchId':sketches[1]})
            # Lines share the coincident authored corner. Move both IDs only if distinct.
            for point in dict.fromkeys([entities[1][0]['endPointId'],entities[1][1]['startPointId']]):
                call('sketch.movePoint',{'pointId':point,'x':0,'y':15,'units':'mm'})
            call('sketch.finish');edited=inspect();measurements['edited']=sheet(body(edited),15)
            assert edited['authored']['features'][-1]['parameters']==feature['parameters']
            for key,idkey in [('solidEdges','edgeId'),('supportFaces','faceId')]:
                assert [x[idkey] for x in body(edited)[key]]==[x[idkey] for x in body(committed)[key]]
            call('history.undo');assert inspect()['authored']==committed['authored']
            call('history.redo');assert inspect()['authored']==edited['authored']
            model=out/'nonplanar-patch.alloy';call('document.save',{'path':str(model)})
            call('document.new',{'discard':True});call('document.open',{'path':str(model),'discard':True})
            reopened=inspect();assert reopened['authored']==edited['authored'];measurements['reopened']=sheet(body(reopened),15)
            call('view.home');call('capture.screenshot',{'path':str(out/'edited-reopened.png')})
            # Two separated pairs: each pair is connected; the complete selection is not.
            call('sketch.create',{'plane':'XY','name':'Alloy disconnected refusal'})
            for coords in [(100,100,120,100),(120,100,120,120)]:
                call('sketch.line',{**dict(zip(('x1','y1','x2','y2'),coords)),'units':'mm'})
            remote=draft();remote_refs=[{'sketchId':remote['id'],'entityId':e['id']} for e in remote['parameters']['entities']]
            call('sketch.finish');before_invalid=inspect()
            call('surface.patch',{**inputs,'boundaryEdges':refs[:2]+remote_refs},error='evaluation_failed');unchanged(before_invalid)
            # Invalid edit must retain the previously accepted feature and result too.
            call('surface.patch',{'featureId':feature['id'],'boundaryEdges':[refs[0],refs[2],refs[1],refs[3]]},error='evaluation_failed');unchanged(before_invalid)
            report={'passed':True,'sourceFixtureSha256':hashlib.sha256(json.dumps({'cornersMm':corners(10),'orderedReferences':refs},sort_keys=True).encode()).hexdigest(),'kernelVersion':original['evaluation']['sketches'][0].get('version'),'captureEnvironment':call('app.state'),'measurements':measurements,'environment':environment,'inputs':inputs,'final':reopened}
            (out/'result.json').write_text(json.dumps(report,indent=2)+'\n')
        finally: (out/'transcript.json').write_text(json.dumps(transcript,indent=2)+'\n')

if __name__=='__main__': main()
