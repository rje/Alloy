#!/usr/bin/env python3
import argparse
import hashlib
import json
from pathlib import Path
from app_harness import application

def main():
    parser=argparse.ArgumentParser(description='Real app kernel boundary integration')
    for name in ('app','output','fixture'): parser.add_argument('--'+name,required=True)
    args=parser.parse_args(); fixture=json.loads(Path(args.fixture).read_text())
    with application(args.app,args.output) as (call,transcript):
        initial=call('app.state'); before=call('document.inspect')
        shape=call('kernel.probe',fixture['box'])
        assert shape['valid'] and shape['closed'] and shape['topology']['solids']==1
        assert abs(shape['volumeMm3']-fixture['volumeMm3'])<=1e-6
        assert shape['boundsMm']['size']==list(fixture['box'].values())
        assert call('document.inspect')==before, 'Diagnostic geometry must not mutate intent'
    result={'result':'passed','fixtureSha256':hashlib.sha256(Path(args.fixture).read_bytes()).hexdigest(),
        'verifiedCapabilities':['kernel.real_solid'],'initialState':initial,'transcript':transcript,
        'shape':shape,'evidenceDimensions':{'geometry':{'status':'passed','reason':'Real diagnostic solid; document modeling follows separately.'}}}
    (Path(args.output)/'kernel-api.json').write_text(json.dumps(result,indent=2)+'\n')
    print('PASS: real app OCCT boundary, solid/volume/bounds and no diagnostic mutation')
if __name__=='__main__':main()
