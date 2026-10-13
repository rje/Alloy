#!/usr/bin/env python3
"""Shared multi-body export API with independent triangle readback."""
import argparse
import json
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl

p = argparse.ArgumentParser()
p.add_argument('--app', required=True)
p.add_argument('--output', required=True)
a = p.parse_args()
out = Path(a.output).resolve()
records = []
with application(a.app, out) as (call, transcript):
    call('solid.box', {'width':20,'depth':10,'height':15,'x':-5,'y':2,'z':3,'units':'mm'})
    call('solid.box', {'width':12,'depth':12,'height':12,'x':40,'y':-5,'z':7,'units':'mm'})
    baseline = call('document.inspect')
    ids = [b['bodyId'] for b in baseline['evaluation']['bodies']]
    settings = {'units':'mm','format':'binary','surfaceDeviationMm':0.1,'normalDeviationDeg':10}
    def export(name, selection, expected):
        before = call('document.inspect')
        path = out/name
        result = call('document.exportStl', dict(settings,path=str(path),**selection))
        mesh = inspect_stl(path, True)
        assert result['bodyIds'] == expected
        assert len(mesh['components']) == len(expected)
        assert mesh['triangles'] == result['triangles']
        after = call('document.inspect')
        assert before['authored'] == after['authored'] and before['undo'] == after['undo']
        records.append({'file':name,'result':result,'mesh':mesh})
    export('selected.stl', {'bodyIds':ids}, ids)
    export('visible.stl', {'allVisible':True}, ids)
    call('feature.visibility', {'featureId':baseline['evaluation']['bodies'][1]['featureId'],'visible':False})
    export('one-visible.stl', {'allVisible':True}, ids[:1])
    export('explicit-hidden.stl', {'bodyIds':ids}, ids)
    export('legacy-single.stl', {'bodyId':ids[0]}, ids[:1])
    path = out/'preserved.stl';path.write_bytes(b'preserve')
    call('document.exportStl', dict(settings,path=str(path),bodyIds=[ids[0],'missing']), error='stale_body')
    assert path.read_bytes() == b'preserve'
    call('view.home')
    call('capture.screenshot', {'path':str(out/'source-bodies.png')})
(out/'multibody-stl-api.json').write_text(json.dumps({'exports':records,'transcript':transcript},indent=2))
print('PASS: explicit bodies, all visible, hidden selection, legacy selection, no authored changes and stale-body refusal')
