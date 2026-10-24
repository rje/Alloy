#!/usr/bin/env python3
"""Matched Box cost after the original 200k-facet retained STL import."""
import argparse
import json
import statistics
import shutil
import subprocess
import threading
import time
from pathlib import Path
from app_harness import application


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--app',required=True);p.add_argument('--output',required=True)
    p.add_argument('--validate',action='store_true')
    p.add_argument('--fixture',default='../Alloy-reference-records/validation/xch015-retained-stl-2026-10-21/capacity-200000.stl')
    args=p.parse_args();out=Path(args.output).resolve()
    with application(args.app,out,command_timeout=120) as (call,transcript):
        pid=call.discovery['pid'];rss=[];stop=threading.Event();origin=time.monotonic()
        def current_rss():
            value=subprocess.run(['ps','-p',str(pid),'-o','rss='],capture_output=True,text=True).stdout.strip()
            return int(value)*1024 if value else 0
        def sample():
            while not stop.is_set():
                rss.append({'elapsedMs':(time.monotonic()-origin)*1000,'bytes':current_rss()});stop.wait(.025)
        monitor=threading.Thread(target=sample);monitor.start()
        metrics=[]
        def measured(name,command,arguments=None):
            before_rss=current_rss();started=time.monotonic();result=call(command,arguments)
            elapsed=(time.monotonic()-started)*1000
            stage={'name':name,'command':command,'wallMs':elapsed,'rssBeforeBytes':before_rss,
                   'rssAfterBytes':current_rss(),'sampledPeakRssBytes':max([s['bytes'] for s in rss if s['elapsedMs']>=(started-origin)*1000] or [before_rss]),'result':result}
            metrics.append(stage);print(name,round(elapsed,1),'ms',flush=True);return result
        try:
            baseline_rss=current_rss()
            for i in range(3):
                call('document.new',{'discard':True})
                state=measured(f'normal-box-{i+1}','solid.box',{'width':4,'depth':5,'height':6,'units':'mm'})
                assert state['features']==1 and state['bodies']==1
                assert abs(state['evaluation']['bodies'][0]['volumeMm3']-120)<1e-9
            call('document.new',{'discard':True})
            source=Path(args.fixture).resolve()
            if args.validate:
                source=out/source.name;shutil.copyfile(Path(args.fixture).resolve(),source)
            measured('capacity-import','document.importStl',{'path':str(source),'units':'mm'})
            imported=call('document.inspect');mesh=imported['meshes'][0];mesh_id=mesh['id']
            assert mesh['triangleCount']==200000 and mesh['vertexCount']==600000
            assert mesh['quality']['boundaryEdges']==600000 and mesh['quality']['connectedComponents']==200000
            page=call('mesh.inspect',{'meshId':mesh_id,'vertexOffset':599000,'triangleOffset':199000})
            assert len(page['verticesMm'])==1000 and len(page['triangles'])==1000
            assert page['verticesMm'][-1]==[498,1599,0] and page['triangles'][-1]==[599997,599998,599999]
            call('selection.set',{'bodyId':mesh_id})
            for i in range(3):
                state=measured(f'mesh-box-{i+1}','solid.box',{'width':4,'depth':5,'height':6,'units':'mm'})
                assert state['features']==1 and state['bodies']==1 and state['meshes']==[mesh]
                assert abs(state['evaluation']['bodies'][0]['volumeMm3']-120)<1e-9
                assert call('app.state')['selection']['meshId']==mesh_id
                if i<2:
                    call('history.undo');assert call('document.inspect')['authored']==imported['authored']
            normal=[s['wallMs'] for s in metrics if s['name'].startswith('normal-box')]
            loaded=[s['wallMs'] for s in metrics if s['name'].startswith('mesh-box')]
            benchmark_peak=max(s['bytes'] for s in rss);validation={}
            if args.validate:
                call('mesh.visibility',{'meshId':mesh_id,'visible':False})
                hidden=call('app.state');assert hidden['selection']['members']==[]
                assert not hidden['viewport']['gpu']['meshes']['bodies'][0]['displayed']
                call('history.undo');assert call('app.state')['viewport']['gpu']['meshes']['bodies'][0]['displayed']
                call('history.redo');assert not call('app.state')['viewport']['gpu']['meshes']['bodies'][0]['displayed']
                call('mesh.visibility',{'meshId':mesh_id,'visible':True});call('selection.set',{'bodyId':mesh_id})
                call('view.fit');call('capture.screenshot',{'path':str(out/'mesh-and-box.png')})
                native=out/'capacity-after-box.alloy';measured('native-save','document.save',{'path':str(native)})
                saved=call('document.inspect');source.unlink();call('document.new',{'discard':True})
                measured('native-open','document.open',{'path':str(native)})
                reopened=call('document.inspect');assert reopened['authored']==saved['authored']
                assert reopened['evaluation']['bodies']==saved['evaluation']['bodies']
                assert call('mesh.inspect',{'meshId':mesh_id,'vertexOffset':599000,'triangleOffset':199000})==page
                call('selection.set',{'bodyId':mesh_id});selected=call('app.state')['selection']
                call('document.importStl',{'path':str(Path(args.fixture).resolve().with_name('capacity-200001.stl')),'units':'mm'},'mesh_import_failed')
                assert call('document.inspect')==reopened and call('app.state')['selection']==selected
                measured('after-reopen-box','solid.box',{'width':4,'depth':5,'height':6,'units':'mm'})
                assert call('document.inspect')['meshes']==saved['meshes']
                validation={'saved':saved,'reopened':reopened,'nativeBytes':native.stat().st_size,
                            'sourceDeletedBeforeReopen':not source.exists(),'selectionAfterRefusal':selected}
            report={'fixture':str(Path(args.fixture).resolve()),'baselineRssBytes':baseline_rss,'metrics':metrics,
                    'normalMedianMs':statistics.median(normal),'meshMedianMs':statistics.median(loaded),
                    'sampledBenchmarkPeakRssBytes':benchmark_peak,'sampledPeakRssBytes':max(s['bytes'] for s in rss),'rssSamples':rss,'mesh':mesh,'finalPage':page,'final':call('document.inspect'),
                    'gpu':call('app.state')['viewport']['gpu'],'validation':validation,'transcript':transcript}
            (out/'report.json').write_text(json.dumps(report,indent=2))
        finally:stop.set();monitor.join()
    print('PASS matched normal/retained-mesh Box samples and exact retained tail geometry')

if __name__=='__main__':main()
