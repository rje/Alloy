#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Bounded real-app strict OBJ/API verification, independent source geometry checks."""
import argparse, hashlib, json
from pathlib import Path
from app_harness import application

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--app',required=True);p.add_argument('--output',required=True)
    a=p.parse_args();out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True)
    vertices=[(0,0,0),(5,0,0),(0,5,0),(0,0,5),(10,0,0),(15,0,0),(10,5,0),(10,0,5)]
    faces=[(0,2,1),(0,1,3),(0,3,2),(1,2,3),(4,6,5),(4,5,7),(4,7,6),(5,6,7)]
    grouped='o first\ng shell one\n'+''.join('v %s %s %s\n'%v for v in vertices)+'vt 0 0\nvn 0 0 1\nmtllib /definitely/nonexistent/sidecar.mtl\nusemtl absent\ns 1\nl 1/1 2/1\np -1\n'
    grouped+='f 1 3 2\nf 1/1 2/1 4/1\nf 1//1 4//1 3//1\nf -7/1/1 -6/1/1 -5/1/1\no second\ng second\n'+''.join('f %d %d %d\n'%tuple(i+1 for i in f) for f in faces[4:])
    quad='v 1 2 3\nv 5 2 3\nv 5 6 3\nv 1 6 3\nf 1 2 3 4\n'
    concave='v 0 0 0\nv 4 0 0\nv 4 4 0\nv 2 1 0\nv 0 4 0\nf 1 2 3 4 5\n'
    fixtures=[('groups',grouped,vertices,faces),('quad',quad,[(1,2,3),(5,2,3),(5,6,3),(1,6,3)],[(3,0,1),(1,2,3)]),('concave',concave,[(0,0,0),(4,0,0),(4,4,0),(2,1,0),(0,4,0)],[(1,2,3),(0,1,3),(0,3,4)])]
    manifest=[]
    with application(a.app,out) as (call,transcript):
        for name,text,points,tris in fixtures:
            for unit,scale in [('mm',1),('in',25.4)]:
                call('document.new',{'discard':True});source=out/(name+'.obj');source.write_text(text)
                call('document.importObj',{'path':str(source),'units':unit})
                state=call('document.inspect');assert len(state['meshes'])==1
                mid=state['meshes'][0]['id'];geo=call('mesh.inspect',{'meshId':mid})
                assert geo['verticesMm']==[[x*scale for x in v] for v in points]
                assert geo['triangles']==[list(t) for t in tris]
                diag=geo['mesh']['diagnostics'];assert diag['sidecarsRead'] is False
                if name=='groups':
                    assert len(diag['sourceLabels'])==4
                    assert diag['ignoredRecords']=={'vt':1,'vn':1,'mtllib':1,'usemtl':1,'s':1,'l':1,'p':1}
                source.unlink();call('history.undo');assert not call('document.inspect')['meshes'];call('history.redo');assert call('mesh.inspect',{'meshId':mid})==geo
                saved=out/(name+'-'+unit+'.alloy');call('document.save',{'path':str(saved)})
                call('document.new',{'discard':True});call('document.open',{'path':str(saved),'discard':True});assert call('mesh.inspect',{'meshId':mid})==geo
                manifest.append({'fixture':name,'units':unit,'sourceSha256':hashlib.sha256(text.encode()).hexdigest(),'triangles':len(tris),'savedSha256':hashlib.sha256(saved.read_bytes()).hexdigest()})
        saved=out/'atomic-before.alloy';call('document.save',{'path':str(saved)})
        before=call('document.inspect');selection=call('app.state')['selection'];saved_bytes=saved.read_bytes()
        bads={'late-index':quad+'f 1 2 99\n','texture-index':quad.replace('f 1 2 3 4','f 1/1 2/1 3/1'),'normal-index':quad.replace('f 1 2 3 4','f 1//1 2//1 3//1'),'bowtie':quad.replace('f 1 2 3 4','f 1 3 2 4'),'nonplanar':quad.replace('v 5 6 3','v 5 6 4'),'repeated':quad.replace('f 1 2 3 4','f 1 2 2 4'),'collinear':'v 0 0 0\nv 1 0 0\nv 2 0 0\nf 1 2 3\n','empty':'','comments':'# empty\n','vertices':'v 0 0 0\n','ignored-only':'v 0 0 0\np 1\n'}
        for name,text in bads.items():
            source=out/(name+'.obj');source.write_text(text);call('document.importObj',{'path':str(source),'units':'mm'},'mesh_import_failed')
            error=transcript[-1]['response']['error'];assert error['filename']==source.name and 'line' in error and 'face' in error and error['remedy']
            assert call('document.inspect')==before and call('app.state')['selection']==selection and saved.read_bytes()==saved_bytes
        (out/'transcript.json').write_text(json.dumps(transcript,indent=2))
        (out/'summary.json').write_text(json.dumps({'passed':True,'responses':len(transcript),'fixtures':manifest,'invalidAtomicCases':list(bads)},indent=2))
if __name__=='__main__':main()
