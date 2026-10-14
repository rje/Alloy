#!/usr/bin/env python3
"""Independent native four-solid shared-hinge fixture and component hierarchy."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application

parser = argparse.ArgumentParser()
parser.add_argument('--app', required=True)
parser.add_argument('--output', required=True)
args = parser.parse_args()
out = Path(args.output).resolve()
out.mkdir(parents=True, exist_ok=True)

with application(args.app, out, command_timeout=90) as (call, ledger):
    def inspect(): return call('document.inspect')
    def draft(): return call('app.state')['activeSession']['draftSketch']
    def sketch_profile(name, polygon, holes):
        call('sketch.begin', {'plane':'XZ', 'name':name})
        for a,b in zip(polygon, polygon[1:]+polygon[:1]):
            call('sketch.line', {'x1':a[0], 'y1':a[1], 'x2':b[0], 'y2':b[1], 'units':'mm'})
        for x,z,r in holes:
            call('sketch.addCircle', {'centerX':x, 'centerY':z, 'diameter':2*r, 'units':'mm'})
        source = draft()
        call('sketch.finish')
        evaluated = next(s for s in inspect()['evaluation']['sketches'] if s['sketchId']==source['id'])
        profile = max(evaluated['profiles'], key=lambda p:p['areaMm2'])
        return source['id'], profile['profileId'], evaluated
    def bodies(): return inspect()['evaluation']['bodies']
    lower = [(-105,-8),(105,-8),(105,8),(10,8),(10,92),(-10,92),(-10,8),(-105,8)]
    lower_sketch,lower_profile,lower_eval = sketch_profile('Lower shared-hinge arm', lower,
        [(-90,0,4.5),(90,0,4.5),(0,80,4.5)])
    print('lower profiles', [(p['areaMm2'],p['innerLoopCount']) for p in lower_eval['profiles']],flush=True)
    call('solid.extrude', {'sketchId':lower_sketch,'profileId':lower_profile,'start':'offset','startOffset':2,
        'distance':8,'units':'mm','name':'Lower arm'})
    print('lower body',[(b['name'],b['boundsMm'],b['volumeMm3']) for b in bodies()],flush=True)
    lower_id = bodies()[0]['bodyId']
    upper = [(-105,172),(-10,172),(-10,72),(10,72),(10,172),(105,172),(105,188),(-105,188)]
    upper_sketch,upper_profile,upper_eval = sketch_profile('Upper shared-hinge arm', upper,
        [(-90,180,4.5),(90,180,4.5),(0,80,4.5)])
    call('solid.extrude', {'sketchId':upper_sketch,'profileId':upper_profile,'start':'offset','startOffset':-15,
        'distance':8,'units':'mm','name':'Upper arm'})
    upper_id = next(b['bodyId'] for b in bodies() if b['name']=='Upper arm')
    call('solid.cylinder',{'radius':4,'height':102,'x':90,'y':0,'z':8,'units':'mm','name':'Inner piston'})
    inner_id = next(b['bodyId'] for b in bodies() if b['name']=='Inner piston')
    def eye_sketch(name,z):
        call('sketch.begin',{'plane':'XZ','name':name})
        call('sketch.addCircle',{'centerX':90,'centerY':z,'diameter':20,'units':'mm'})
        call('sketch.addCircle',{'centerX':90,'centerY':z,'diameter':9,'units':'mm'})
        source=draft();call('sketch.finish')
        evaluated=next(s for s in inspect()['evaluation']['sketches'] if s['sketchId']==source['id'])
        ring=next(p for p in evaluated['profiles'] if p['innerLoopCount']==1)
        return source['id'],ring['profileId']
    eye,ring=eye_sketch('Inner piston lower eye',0)
    call('solid.extrude',{'sketchId':eye,'profileId':ring,'start':'offset','startOffset':-1,
        'distance':2,'units':'mm','operation':'join','targetBodyIds':[inner_id]})
    call('sketch.begin',{'plane':'XY','name':'Outer piston sleeve annulus'})
    call('sketch.addCircle',{'centerX':90,'centerY':0,'diameter':12,'units':'mm'})
    call('sketch.addCircle',{'centerX':90,'centerY':0,'diameter':10,'units':'mm'})
    tube_source=draft();call('sketch.finish')
    tube_eval=next(s for s in inspect()['evaluation']['sketches'] if s['sketchId']==tube_source['id'])
    tube_ring=next(p for p in tube_eval['profiles'] if p['innerLoopCount']==1)
    call('solid.extrude',{'sketchId':tube_source['id'],'profileId':tube_ring['profileId'],
        'start':'offset','startOffset':80,'distance':94,'units':'mm','name':'Outer piston'})
    outer_id=next(b['bodyId'] for b in bodies() if b['name']=='Outer piston')
    eye,ring=eye_sketch('Outer piston upper eye',180)
    call('solid.extrude',{'sketchId':eye,'profileId':ring,'start':'offset','startOffset':-5,
        'distance':6,'units':'mm','operation':'join','targetBodyIds':[outer_id]})
    before=inspect();original=before['evaluation']['bodies']
    assert len(original)==4 and all(b['valid'] and b['closed'] and b['volumeMm3']>0 for b in original)
    ids=[lower_id,upper_id,inner_id,outer_id]
    assert [b['bodyId'] for b in original]==ids
    call('view.home');call('capture.screenshot',{'path':str(out/'four-native-solids.png')})
    call('document.save',{'path':str(out/'four-native-solids.alloy')})
    print('four bodies',[(b['name'],b['boundsMm'],b['volumeMm3']) for b in original],flush=True)
    call('component.createFromBodies',{'bodyIds':ids})
    converted=inspect();components=converted['authored']['components'];occurrences=converted['authored']['occurrences']
    assert len(components)==len(occurrences)==4
    assert converted['authored']['features']==before['authored']['features']
    assert all(a['bodyId']==b['bodyId'] and a['boundsMm']==b['boundsMm'] and a['volumeMm3']==b['volumeMm3']
        for a,b in zip(original,converted['evaluation']['bodies']))
    parent=call('component.create',{'name':'Piston subassembly','activate':False})
    parent_id=parent['definitionId'];parent_occurrence=parent['occurrenceId']
    call('component.reparentOccurrence',{'occurrenceIds':[occurrences[2]['id'],occurrences[3]['id']],
        'destinationId':parent_id})
    grouped=inspect();tree=grouped['authored']['occurrences']
    assert [o['parentId'] for o in tree]==[grouped['authored']['documentId']]*2+[parent_id]*2+[grouped['authored']['documentId']]
    assert grouped['authored']['features']==before['authored']['features']
    assert all(a['boundsMm']==b['boundsMm'] for a,b in zip(original,grouped['evaluation']['bodies']))
    call('capture.screenshot',{'path':str(out/'nested-piston-browser.png')})
    call('component.reparentOccurrence',{'occurrenceIds':[parent_occurrence],
        'destinationId':components[2]['id']},error='invalid_component_hierarchy')
    assert inspect()['authored']==grouped['authored']
    call('history.undo');call('history.redo');assert inspect()['authored']==grouped['authored']
    rod_feature=next(f for f in grouped['authored']['features'] if f.get('bodyId')==inner_id and f['type']=='cylinder')
    call('feature.edit',{'featureId':rod_feature['id'],'parameters':{**rod_feature['parameters'],'radiusMm':4.2}})
    edited=inspect();assert edited['authored']['components']==grouped['authored']['components']
    assert edited['authored']['occurrences']==grouped['authored']['occurrences']
    assert edited['evaluation']['bodies'][2]['bodyId']==inner_id
    assert edited['evaluation']['bodies'][2]['volumeMm3']>original[2]['volumeMm3']
    call('history.undo');assert inspect()['authored']==grouped['authored']
    call('history.redo');assert inspect()['authored']==edited['authored']
    saved=out/'piston-components.alloy';call('document.save',{'path':str(saved)})
    call('document.open',{'path':str(saved),'discard':True})
    reopened=inspect();assert reopened['authored']==edited['authored']
    assert [b['bodyId'] for b in reopened['evaluation']['bodies']]==ids
    exported=call('document.exportStl',{'occurrenceId':parent_occurrence,'path':str(out/'piston-subassembly.stl'),
        'units':'mm','format':'binary','surfaceDeviationMm':0.1,'normalDeviationDeg':10})
    assert exported['bodyCount']==2 and set(exported['bodyIds'])=={inner_id,outer_id}
    call('view.home');call('capture.screenshot',{'path':str(out/'piston-reopened.png')})
    (out/'receipt.json').write_text(json.dumps({'passed':True,'scope':'independent native shared-hinge piston fixture; identity component hierarchy',
        'bodyIds':ids,'parentId':parent_id,'initial':before['evaluation']['bodies'],
        'final':reopened,'ledger':ledger},indent=2)+'\n')
    print('PASS native piston geometry, conversion/nesting/edit/history/reopen',flush=True)
