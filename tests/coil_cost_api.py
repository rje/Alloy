#!/usr/bin/env python3
"""Matched retained Coil Pitch/Move/history cost sample through the real app."""
import argparse
import json
import math
import time
from pathlib import Path
from app_harness import application
from alloyctl import request
from stl_export_api import inspect_stl


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app',required=True)
    parser.add_argument('--output',required=True)
    parser.add_argument('--expect-reuse',action='store_true')
    parser.add_argument('--validate',action='store_true')
    parser.add_argument('--fixture',default='../Alloy-reference-records/validation/coil-2026-10-24/api-responsive/spring.alloy')
    args=parser.parse_args()
    output=Path(args.output).resolve()
    with application(args.app,output,command_timeout=60) as (call,transcript):
        call('document.open',{'path':str(Path(args.fixture).resolve())})
        before=call('document.inspect')
        authored=before['authored'];coil=authored['features'][0]
        assert coil['type']=='coil' and coil['parameters']['revolutions']==4
        assert authored['features'][1]['type']=='move' and len(authored['features'])==2
        parameter=next(p for p in before['userParameters'] if p['id']==coil['parameters']['pitchParameterId'])
        assert parameter['valueMm']==7
        call('view.home');call('view.fit');call('view.zoom',{'factor':.7})
        initial_gpu=call('app.state')['viewport']['gpu'];camera=initial_gpu['camera']
        assert initial_gpu['hardwareAccelerated']
        call('capture.screenshot',{'path':str(output/'pitch7-before.png')})
        expected_features=json.loads(json.dumps(authored['features']));expected_features[0]['parameters']['pitchMm']=9
        samples=[]
        def measure(name,command,args=None):
            started=time.monotonic();response=request(call.endpoint,command,args,timeout=60,transport=call.transport)
            dispatch_ms=(time.monotonic()-started)*1000
            transcript.append({'command':command,'args':args or {},'response':response,'elapsedMs':dispatch_ms})
            initial=response.get('result',{});pending_inspect_ms=None
            if response.get('completion')=='pending':
                inspected=time.monotonic();call('app.state');pending_inspect_ms=(time.monotonic()-inspected)*1000
            while response.get('completion')=='pending':
                assert time.monotonic()-started<60
                time.sleep(.025);response=request(call.endpoint,'command.status',{'session':initial['id'],'generation':initial['previewGeneration']},timeout=60,transport=call.transport)
            assert response['ok'],response
            sample={'name':name,'command':command,'wallMs':(time.monotonic()-started)*1000,'dispatchMs':dispatch_ms,
                    'pendingInspectMs':pending_inspect_ms,'completedOperation':response.get('result',{}),
                    'document':call('document.inspect'), 'gpu':call('app.state')['viewport']['gpu']}
            samples.append(sample);print(name,round(sample['wallMs'],1),'ms',flush=True)
            return sample
        for repeat in range(2):
            assert call('document.inspect')['authored']==authored
            edited=measure(f'pitch9-{repeat+1}','document.parameterUpdate',{'parameterId':parameter['id'],'value':9})
            assert edited['document']['authored']['features']==expected_features
            if args.expect_reuse:
                assert edited['completedOperation']['authored']==edited['document']['authored']
                assert edited['completedOperation']['id']==edited['document']['id']
                assert edited['completedOperation']['userParameters']==edited['document']['userParameters']
            assert edited['document']['evaluation']['bodies'][0]['bodyId']==coil['bodyId']
            if repeat==0:
                call('capture.screenshot',{'path':str(output/'pitch9.png')})
            undone=measure(f'undo7-{repeat+1}','history.undo')
            assert undone['document']['authored']==authored
            if args.expect_reuse:
                assert undone['gpu']['displayMeshes'][0]['identitySignature']==initial_gpu['displayMeshes'][0]['identitySignature']
            redone=measure(f'redo9-{repeat+1}','history.redo')
            assert redone['document']['authored']==edited['document']['authored']
            if args.expect_reuse:
                assert redone['gpu']['displayMeshes'][0]['identitySignature']==edited['gpu']['displayMeshes'][0]['identitySignature']
                assert edited['dispatchMs']<500 and edited['pendingInspectMs']<500
                assert edited['completedOperation']['evaluationHeartbeat']['maxGapMs']<500
            if repeat==0:
                reset=measure('reset7','history.undo');assert reset['document']['authored']==authored
        validation={}
        if args.validate:
            committed=call('document.inspect');body=committed['evaluation']['bodies'][0]
            expected_volume=math.pi*4*math.hypot(20*math.pi,9)
            assert body['valid'] and body['closed'] and math.isclose(body['volumeMm3'],expected_volume,rel_tol=1e-5)
            # A modal Parameters flow exposes pending Cancel without native input.
            call('document.parametersDialog')
            started=time.monotonic();pending=request(call.endpoint,'document.parameterUpdate',
                {'parameterId':parameter['id'],'name':parameter['name'],'units':'mm','value':11},timeout=2,transport=call.transport)
            validation['cancelledUpdateDispatchMs']=(time.monotonic()-started)*1000
            assert pending['ok'] and pending['completion']=='pending'
            call('capture.dialog',{'path':str(output/'parameters-pending.png')})
            started=time.monotonic();call('command.cancel',{'session':pending['result']['id']})
            validation['cancelDispatchMs']=(time.monotonic()-started)*1000
            assert validation['cancelledUpdateDispatchMs']<500 and validation['cancelDispatchMs']<500
            assert call('document.inspect')==committed
            call('capture.dialog',{'path':str(output/'parameters-cancelled.png')})
            deadline=time.monotonic()+60
            while call('app.state')['geometryWork']['active']:
                assert time.monotonic()<deadline;time.sleep(.025)
            assert call('document.inspect')==committed
            call('capture.dialog',{'path':str(output/'parameters-cancelled-settled.png')})
            call('document.parametersDialog',{'visible':False})
            call('document.parameterUpdate',{'parameterId':parameter['id'],'value':1},'parameter_evaluation_failed')
            assert call('document.inspect')==committed
            native=output/'source-edited.alloy';call('document.save',{'path':str(native)})
            saved=call('document.inspect');call('document.open',{'path':str(native)})
            reopened=call('document.inspect');assert reopened['authored']==saved['authored']
            assert reopened['evaluation']['bodies']==saved['evaluation']['bodies']
            mesh_path=output/'source-edited.stl'
            call('document.exportStl',{'path':str(mesh_path),'bodyId':coil['bodyId'],'units':'mm','format':'binary',
                'surfaceDeviationMm':.03,'normalDeviationDeg':8})
            mesh=inspect_stl(mesh_path,True)
            assert len(mesh['components'])==1 and mesh['eulerCharacteristic']==2
            assert math.isclose(mesh['signedVolume'],expected_volume,rel_tol=.02)
            assert all(math.isclose(mesh['bounds'][1][i]-mesh['bounds'][0][i],body['boundsMm']['size'][i],abs_tol=.09) for i in range(3))
            validation.update({'reopened':reopened,'mesh':mesh})
        (output/'report.json').write_text(json.dumps({'fixture':str(Path(args.fixture).resolve()),
            'authoredBefore':authored,'initialGpu':initial_gpu,'camera':camera,'samples':samples,'validation':validation,'transcript':transcript},indent=2))
    print('PASS: matched Coil source/history sample, retained IDs and named binding')

if __name__=='__main__':main()
