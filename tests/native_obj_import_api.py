#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Prepared OBJ native command path, exact geometry and atomic history via real app."""
import argparse, json, time
from pathlib import Path
from app_harness import application


def fixtures():
    points=[(0,0,0),(5,0,0),(0,5,0),(0,0,5),(10,0,0),(15,0,0),(10,5,0),(10,0,5)]
    tris=[(0,2,1),(0,1,3),(0,3,2),(1,2,3),(4,6,5),(4,5,7),(4,7,6),(5,6,7)]
    groups='o first\ng shell one\n'+''.join('v %s %s %s\n'%v for v in points)+'vt 0 0\nvn 0 0 1\nmtllib /definitely/nonexistent/sidecar.mtl\nusemtl absent\ns 1\nl 1/1 2/1\np -1\n'
    groups+='f 1 3 2\nf 1/1 2/1 4/1\nf 1//1 4//1 3//1\nf -7/1/1 -6/1/1 -5/1/1\no second\ng second\n'+''.join('f %d %d %d\n'%tuple(i+1 for i in f) for f in tris[4:])
    return [('groups',groups,points,tris),('quad','v 1 2 3\nv 5 2 3\nv 5 6 3\nv 1 6 3\nf 1 2 3 4\n',[(1,2,3),(5,2,3),(5,6,3),(1,6,3)],[(3,0,1),(1,2,3)]),('concave','v 0 0 0\nv 4 0 0\nv 4 4 0\nv 2 1 0\nv 0 4 0\nf 1 2 3 4 5\n',[(0,0,0),(4,0,0),(4,4,0),(2,1,0),(0,4,0)],[(1,2,3),(0,1,3),(0,3,4)])]


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--app',required=True);p.add_argument('--output',required=True);a=p.parse_args()
    out=Path(a.output).resolve();out.mkdir(parents=True,exist_ok=True);manifest=[]
    for name,text,_,_ in fixtures(): (out/(name+'.obj')).write_text(text)
    with application(a.app,out) as (call,transcript):
        def ready(expected='ready'):
            deadline=time.monotonic()+30
            while True:
                state=call('app.state')['meshImport']
                if state['status']!='pending': assert state['status']==expected,state;return state
                assert time.monotonic()<deadline;time.sleep(.02)
        for name,text,points,tris in fixtures():
            for units,scale in [('mm',1),('in',25.4)]:
                call('document.new',{'discard':True});call('solid.box',{'width':8,'depth':9,'height':10,'x':30,'units':'mm'})
                saved=out/'before.alloy';call('document.save',{'path':str(saved)});before=call('document.inspect');selection=call('app.state')['selection'];saved_bytes=saved.read_bytes()
                source=out/('selected-'+name+('.mesh' if name=='concave' else '.OBJ'));source.write_text(text)
                call('mesh.importPreview',{'path':str(source)});state=ready();assert state['preview']['source']['format']=='obj'
                # Repeated unit updates use the immutable mm original, never cumulative scale.
                for unit in ['in','mm',units]:call('mesh.importUnits',{'units':unit});state=ready()
                preview=state['preview'];assert preview['triangleCount']==len(tris)
                assert call('document.inspect')==before and call('app.state')['selection']==selection and saved.read_bytes()==saved_bytes
                call('mesh.importCancel');assert call('document.inspect')==before and call('app.state')['selection']==selection
                call('mesh.importPreview',{'path':str(source)});ready();call('mesh.importUnits',{'units':units});state=ready();preview=state['preview']
                source.unlink() # OK must commit the exact owned ready candidate without rereading.
                call('mesh.importAccept');mid=preview['id'];geo=call('mesh.inspect',{'meshId':mid})
                assert geo['mesh']==preview and geo['verticesMm']==[[x*scale for x in v] for v in points] and geo['triangles']==[list(t) for t in tris]
                assert call('app.state')['selection']['meshId']==mid
                call('history.undo');assert not call('document.inspect')['meshes'];call('history.redo');assert call('mesh.inspect',{'meshId':mid})==geo
                path=out/(name+'-'+units+'.alloy');call('document.save',{'path':str(path)});call('document.new',{'discard':True});call('document.open',{'path':str(path),'discard':True});assert call('mesh.inspect',{'meshId':mid})==geo
                manifest.append({'name':name,'units':units,'preview':preview,'geometry':geo})
        # Shared canonical-mm polygon thresholds must agree with direct import.
        # Keep the source snapshot even when default mm fails; switching units
        # and switching back must not touch a deleted source path.
        threshold_cases=[('planarity','v 0 0 0\nv 0.001 0 0\nv 0.001 0.001 0\nv 0 0.001 0.0000000005\nf 1 2 3 4\n',True,False),('tiny','v 0 0 0\nv 1e-13 0 0\nv 0 1e-13 0\nf 1 2 3\n',False,True)]
        for name,text,mm_ok,in_ok in threshold_cases:
            direct={}
            for unit,ok in [('mm',mm_ok),('in',in_ok)]:
                call('document.new',{'discard':True});source=out/(name+'.obj');source.write_text(text)
                call('document.importObj',{'path':str(source),'units':unit},None if ok else 'mesh_import_failed')
                if ok:
                    mid=call('document.inspect')['meshes'][0]['id'];direct[unit]=call('mesh.inspect',{'meshId':mid})
            call('document.new',{'discard':True});call('mesh.importPreview',{'path':str(source)});ready('ready' if mm_ok else 'failed');source.unlink()
            for unit,ok in [('in',in_ok),('mm',mm_ok),('in',in_ok)]:
                call('mesh.importUnits',{'units':unit});state=ready('ready' if ok else 'failed')
                if ok:
                    assert state['preview']['boundsMm']==direct[unit]['mesh']['boundsMm']
            call('mesh.importCancel');assert not call('document.inspect')['meshes']
        saved=out/'invalid-before.alloy';call('document.save',{'path':str(saved)});before=call('document.inspect');selection=call('app.state')['selection'];saved_bytes=saved.read_bytes()
        invalid=out/'invalid.obj';invalid.write_text(fixtures()[1][1]+'f 1 2 99\n');call('mesh.importPreview',{'path':str(invalid)});bad=ready('failed');call('capture.screenshot',{'path':str(out/'invalid.png')});assert bad['error']['line']==6 and bad['error']['face']==2 and bad['error']['remedy'];call('mesh.importCancel')
        assert call('document.inspect')==before and call('app.state')['selection']==selection and saved.read_bytes()==saved_bytes
        call('capture.screenshot',{'path':str(out/'reopened.png')})
        (out/'summary.json').write_text(json.dumps({'passed':True,'responses':len(transcript),'fixtures':manifest,'invalid':bad},indent=2));(out/'transcript.json').write_text(json.dumps(transcript,indent=2))
if __name__=='__main__':main()
