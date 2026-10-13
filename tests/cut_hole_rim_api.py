#!/usr/bin/env python3
"""Focused real-app Cut-hole edge lineage, R1 Fillet and floor-support receipt."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--app',required=True)
parser.add_argument('--output',required=True)
args=parser.parse_args()
out=Path(args.output).resolve();out.mkdir(parents=True,exist_ok=True)
with application(args.app,out) as (call,transcript):
    def inspect():return call('document.inspect')
    def active():return call('app.state')['activeSession']
    def body():
        bodies=inspect()['evaluation']['bodies']
        assert len(bodies)==1 and bodies[0]['valid'] and bodies[0]['closed']
        return bodies[0]
    def support(face):return {k:face[k] for k in ('producerFeatureId','bodyId','faceId')}
    def floor_face(identity):
        matches=[f for f in body()['supportFaces'] if support(f)==identity and
                 f['frame']['normal']==[0,0,1] and math.isclose(f['frame']['origin'][2],2,abs_tol=1e-6)]
        assert len(matches)==1,'Original broad floor support lost'
        return matches[0]
    def available_rim(sketch_id):
        found=[e for e in body()['solidEdges'] if e['available'] and
               e['provenance']['sourceSketchId']==sketch_id and
               e['provenance']['role']=='baseRim' and
               math.isclose(e['midpointMm'][2],2,abs_tol=1e-6)]
        assert len(found)==1,'Upper Cut circle rim has no unique live identity'
        return found[0]
    try:
        call('window.resize',{'width':1400,'height':1000})
        call('sketch.begin',{'plane':'XY','units':'mm','name':'Keyring floor'})
        call('sketch.rectangle',{'x1':0,'y1':-30,'x2':60,'y2':0,'units':'mm'})
        floor_sketch=active()['draftSketch']['id']
        call('sketch.finish')
        floor_profile=inspect()['evaluation']['sketches'][0]['profiles'][0]['profileId']
        call('solid.extrude',{'sketchId':floor_sketch,'profileId':floor_profile,'distance':2,'units':'mm'})
        base=body();body_id=base['bodyId']
        top=next(f for f in base['supportFaces'] if f['role']=='extrude.endCap')
        identity=support(top);floor_face(identity)
        call('sketch.twoPointCircle')
        call('sketch.begin',{'support':identity,'units':'mm','name':'Keyring bore'})
        sketch_id=active()['draftSketch']['id']
        def pixel(u,v):
            projected=call('view.projectSketchPoint',{'sketchId':sketch_id,'uMm':u,'vMm':v})
            return {key:projected[key] for key in ('x','y')}
        call('sketch.placePoint',pixel(5,-15))
        call('sketch.pointerMove',pixel(9,-15))
        call('sketch.circleDiameterPreview',{'text':'4'})
        call('sketch.circleDiameterAccept')
        source=active()['draftSketch']
        circle=source['parameters']['entities'][0]
        assert circle['diameterStartPointId'] and math.isclose(circle['radiusMm'],2)
        diameter=source['parameters']['dimensions'][0]['id']
        call('sketch.finish')
        circle_profile=inspect()['evaluation']['sketches'][-1]['profiles'][0]['profileId']
        call('solid.extrude',{'sketchId':sketch_id,'profileId':circle_profile,'distance':-10,
                              'operation':'cut','targetBodyIds':[body_id],'units':'mm'})
        cut=body();floor_face(identity)
        assert math.isclose(cut['volumeMm3'],3600-8*math.pi,abs_tol=1e-4)
        rim=available_rim(sketch_id)
        call('view.home');call('view.orientation',{'name':'top'})
        call('capture.screenshot',{'path':str(out/'cut-before-rim-selection.png')})
        session=call('solid.filletDialog',{'targetBodyId':body_id})
        point=call('view.projectEdge',{'bodyId':body_id,'edgeId':rim['edgeId']})
        picked=call('selection.pick',{'x':point['x'],'y':point['y']})
        assert picked['edgeId']==rim['edgeId']
        call('command.update',{'session':session['id'],'inputs':{'radiusMm':1}})
        preview=call('command.preview',{'session':session['id']})
        assert preview['geometry']['valid']
        call('capture.screenshot',{'path':str(out/'selected-upper-rim-preview.png')})
        call('command.commit',{'session':session['id']})
        filleted=inspect();floor_face(identity)
        assert body()['volumeMm3']<cut['volumeMm3']
        call('capture.screenshot',{'path':str(out/'filleted-hole.png')})
        call('sketch.text',{'support':identity,'name':'Later text support','units':'mm'})
        call('sketch.textDialog')
        call('sketch.textPreview',{'text':'DSB','heightMm':10,'uMm':17,'vMm':-18})
        call('capture.screenshot',{'path':str(out/'floor-text-support-preview.png')})
        call('sketch.textCancel');call('sketch.cancel')
        assert inspect()['authored']==filleted['authored']
        call('sketch.edit',{'sketchId':sketch_id})
        call('sketch.dimensionUpdate',{'dimensionId':diameter,'value':4.5,'units':'mm'})
        call('sketch.finish')
        edited=inspect()
        assert edited['evaluation']['bodies'][0]['bodyId']==body_id
        floor_face(identity)
        retained=[e for e in body()['solidEdges'] if e['edgeId']==rim['edgeId']]
        assert len(retained)==1 and retained[0]['provenance']['sourceSketchId']==sketch_id
        call('history.undo')
        assert inspect()['authored']==filleted['authored']
        call('history.redo')
        assert inspect()['authored']==edited['authored']
        call('history.undo')
        path=out/'hole-rim-r1.alloy'
        call('document.save',{'path':str(path)})
        call('document.open',{'path':str(path)})
        assert inspect()['authored']==filleted['authored']
        floor_face(identity)
        report={'passed':True,'bodyId':body_id,'floorSupport':identity,'holeSketchId':sketch_id,
            'rimEdgeId':rim['edgeId'],'cutVolumeMm3':cut['volumeMm3'],
            'filletedVolumeMm3':filleted['evaluation']['bodies'][0]['volumeMm3'],
            'gpu':call('app.state')['viewport']['gpu']}
        (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    finally:
        (out/'transcript.json').write_text(json.dumps(transcript,indent=2)+'\n')
