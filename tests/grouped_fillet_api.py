#!/usr/bin/env python3
"""One real Box Fillet with independent vertical and top/bottom radius sets."""
import argparse
import json
import math
import uuid
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--app',required=True)
p.add_argument('--output',required=True)
a=p.parse_args()
out=Path(a.output).resolve()

def group(edges,radius):
    return {'id':str(uuid.uuid4()),'edgeIds':edges,'radiusMm':radius,'tangentChain':True}
def near(actual,expected):
    assert math.isclose(actual,expected,abs_tol=1e-5),(actual,expected)

with application(a.app,out) as (call,transcript):
    assert call('app.state')['viewport']['gpu']['hardwareAccelerated']
    call('solid.box',{'width':50,'depth':80,'height':40,'units':'mm','name':'Enclosure stock'})
    source=call('document.inspect')
    body=source['evaluation']['bodies'][0]
    edges=body['solidEdges']
    vertical=[e['edgeId'] for e in edges if 'z*' in e['provenance']['boxEdge']]
    rims=[e['edgeId'] for e in edges if 'z*' not in e['provenance']['boxEdge']]
    assert len(vertical)==4 and len(rims)==8
    groups=[group(vertical,5),group(rims,2.5)]
    session=call('solid.filletDialog',{'targetBodyId':body['bodyId']})
    assert session['inputs']['radiusMm']==0 and session['inputs']['edgeIds']==[]
    call('command.update',{'session':session['id'],'inputs':{'radiusMm':5}})
    assert call('command.preview',{'session':session['id']},error='edges_required')=={}
    call('command.update',{'session':session['id'],'inputs':{'groups':groups}})
    preview=call('command.preview',{'session':session['id']})
    assert preview['previewed'] and preview['geometry']['radiusGroups'][0]['radiusMm']==5
    assert len(preview['geometry']['radiusGroups'])==2
    call('capture.screenshot',{'path':str(out/'grouped-preview.png')})
    call('command.cancel',{'session':session['id']})
    assert call('document.inspect')['authored']==source['authored']
    call('solid.fillet',{'targetBodyId':body['bodyId'],'groups':groups})
    rounded=call('document.inspect')
    authored=rounded['authored']
    fillet=authored['features'][-1]
    assert fillet['type']=='fillet' and fillet['parameters']['groups']==groups
    shape=rounded['evaluation']['bodies'][0]
    assert shape['valid'] and shape['closed'] and len(shape['radiusGroups'])==2
    assert 150000<shape['volumeMm3']<160000
    near(shape['boundsMm']['max'][0],50)
    call('view.home')
    call('capture.screenshot',{'path':str(out/'grouped-committed.png')})
    call('history.undo')
    assert call('document.inspect')['authored']==source['authored']
    call('history.redo')
    assert call('document.inspect')['authored']==authored
    edited=[dict(groups[0],radiusMm=4),groups[1]]
    call('solid.fillet',{'featureId':fillet['id'],'targetBodyId':body['bodyId'],'groups':edited})
    after_edit=call('document.inspect')
    assert after_edit['authored']['features'][-1]['parameters']['groups']==edited
    box=authored['features'][0]
    call('solid.box',{'featureId':box['id'],'width':60,'depth':80,'height':40,'units':'mm'})
    resized=call('document.inspect')
    assert resized['evaluation']['bodies'][0]['valid']
    near(resized['evaluation']['bodies'][0]['boundsMm']['max'][0],60)
    call('solid.fillet',{'featureId':fillet['id'],'targetBodyId':body['bodyId'],
        'groups':[dict(edited[0],radiusMm=30),edited[1]]},error='evaluation_failed')
    assert call('document.inspect')['authored']==resized['authored']
    path=out/'grouped-fillet.alloy'
    call('document.save',{'path':str(path)})
    call('document.open',{'path':str(path),'discard':True})
    assert call('document.inspect')['authored']==resized['authored']
    stl=out/'grouped-fillet.stl'
    call('document.exportStl',{'path':str(stl),'bodyId':body['bodyId'],
        'units':'mm','format':'binary','surfaceDeviationMm':0.1,'normalDeviationDeg':10})
    mesh=inspect_stl(stl,True)
    assert len(mesh['components'])==1
    near(mesh['bounds'][1][0],60)
    call('capture.screenshot',{'path':str(out/'grouped-reopened.png')})

(out/'receipt.json').write_text(json.dumps({'groups':groups,'resizedGroups':edited,
    'volumeMm3':shape['volumeMm3'],'mesh':mesh,'callCount':len(transcript),
    'transcript':transcript},indent=2)+'\n')
print('PASS: one grouped Fillet, actual radii/solid, Cancel, edit, resize, refusal, history/reopen and STL')
