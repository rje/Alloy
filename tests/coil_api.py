#!/usr/bin/env python3
"""Bounded retained circular Coil through the real Alloy application."""
import argparse
import json
import math
import time
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl
from alloyctl import request


def volume(pitch, turns=3, diameter=20, section=2):
    return math.pi*(section/2)**2*turns*math.hypot(math.pi*diameter,pitch)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app',required=True)
    parser.add_argument('--output',required=True)
    args=parser.parse_args()
    output=Path(args.output).resolve()
    with application(args.app,output) as (call,transcript):
        timings={}
        def timed(label,command,args):
            started=time.monotonic();result=call(command,args);timings[label]=time.monotonic()-started;print(f'{label}: {timings[label]:.3f}',flush=True);return result
        inspect=lambda:call('document.inspect')
        gpu=call('app.state')['viewport']['gpu']
        assert gpu['hardwareAccelerated']
        baseline=inspect()
        # Inspect the initial async dispatch separately from completed geometry.
        pending=call('primitive.coil')
        started=time.monotonic()
        response=request(call.endpoint,'command.preview',{'session':pending['id']},timeout=2,transport=call.transport)
        timings['previewDispatchSeconds']=time.monotonic()-started
        assert response['ok'] and response['completion']=='pending' and timings['previewDispatchSeconds']<.5
        started=time.monotonic();state=call('app.state');timings['pendingInspectSeconds']=time.monotonic()-started
        assert timings['pendingInspectSeconds']<.5 and state['activeSession']['evaluationStatus']=='pending'
        started=time.monotonic();call('command.cancel',{'session':pending['id']});timings['pendingCancelSeconds']=time.monotonic()-started
        assert timings['pendingCancelSeconds']<.5 and inspect()==baseline
        deadline=time.monotonic()+20
        while call('app.state')['geometryWork']['active']:
            assert time.monotonic()<deadline
            time.sleep(.025)
        session=call('primitive.coil')
        assert session['inputs']['diameter']==20 and session['inputs']['pitch']==5
        preview=timed('defaultPreviewSeconds','command.preview',{'session':session['id']})
        assert math.isclose(preview['geometry']['volumeMm3'],volume(5),rel_tol=1e-5)
        timings['defaultWorkerPhasesMs']=preview['evaluationTimingMs']
        timings['defaultHeartbeat']=preview['evaluationHeartbeat']
        assert preview['evaluationTimingMs']['publication']<500
        assert preview['evaluationHeartbeat']['ticks']>2 and preview['evaluationHeartbeat']['maxGapMs']<500
        call('capture.dialog',{'path':str(output/'coil-panel.png')})
        call('capture.screenshot',{'path':str(output/'coil-preview.png')})
        assert inspect()==baseline
        call('command.cancel',{'session':session['id']})
        assert inspect()==baseline
        call('document.parameterAdd',{'name':'SpringPitch','value':5,'units':'mm'})
        parameter=inspect()['userParameters'][0]
        timed('defaultCommitSeconds','solid.coil',{'diameter':20,'pitchParameter':'SpringPitch','revolutions':3,
                          'sectionSize':2,'x':4,'y':-3,'units':'mm'})
        feature=inspect()['authored']['features'][0]
        assert feature['parameters']['pitchParameterId']==parameter['id']
        assert math.isclose(inspect()['evaluation']['bodies'][0]['volumeMm3'],volume(5),rel_tol=1e-5)
        call('solid.move',{'targetBodyId':feature['bodyId'],'steps':[
            {'xMm':0,'yMm':0,'zMm':3,'xDeg':0,'yDeg':0,'zDeg':0}]})
        timed('pitch7UpdateSeconds','document.parameterUpdate',{'parameterId':parameter['id'],'value':7})
        body=inspect()['evaluation']['bodies'][0]
        assert body['bodyId']==feature['bodyId'] and math.isclose(body['volumeMm3'],volume(7),rel_tol=1e-5)
        assert math.isclose(body['boundsMm']['min'][2],3-10/math.hypot(10,7/(2*math.pi)),abs_tol=2e-3)
        before=inspect()
        call('document.parameterUpdate',{'parameterId':parameter['id'],'value':1},'parameter_evaluation_failed')
        assert inspect()==before
        session=call('feature.editDialog',{'featureId':feature['id']})
        assert session['inputs']['pitchParameter']=='SpringPitch'
        call('command.update',{'session':session['id'],'inputs':{'revolutions':4}})
        call('command.preview',{'session':session['id']})
        timed('retainedPreparedCommitSeconds','command.commit',{'session':session['id']})
        assert timings['retainedPreparedCommitSeconds']<.5
        edited=inspect()
        assert edited['authored']['features'][0]['id']==feature['id']
        assert math.isclose(edited['evaluation']['bodies'][0]['volumeMm3'],volume(7,4),rel_tol=1e-5)
        call('history.undo');assert inspect()['authored']==before['authored']
        call('history.redo');assert inspect()['authored']==edited['authored'];edited=inspect()
        session=call('feature.editDialog',{'featureId':feature['id']})
        call('command.update',{'session':session['id'],'inputs':{'sectionSize':8}},'evaluation_failed')
        assert not call('app.state')['viewport']['gpu']['previewVisible']
        call('command.commit',{'session':session['id']},'evaluation_failed')
        assert inspect()==edited
        call('command.update',{'session':session['id'],'inputs':{'sectionSize':2}})
        call('command.preview',{'session':session['id']})
        call('command.cancel',{'session':session['id']});assert inspect()==edited
        path=output/'spring.alloy';call('document.save',{'path':str(path)})
        saved=inspect();call('document.open',{'path':str(path)})
        reopened=inspect();assert reopened['authored']==saved['authored']
        assert reopened['evaluation']['bodies']==saved['evaluation']['bodies']
        meshes=[]
        for pitch in (7,9):
            if pitch==9:call('document.parameterUpdate',{'parameterId':parameter['id'],'value':pitch})
            body=inspect()['evaluation']['bodies'][0]
            path=output/f'spring-pitch-{pitch}.stl'
            call('document.exportStl',{'path':str(path),'bodyId':body['bodyId'],'units':'mm',
                'format':'binary','surfaceDeviationMm':.03,'normalDeviationDeg':8})
            mesh=inspect_stl(path,True)
            assert len(mesh['components'])==1 and mesh['eulerCharacteristic']==2
            assert math.isclose(mesh['signedVolume'],volume(pitch,4),rel_tol=.02)
            assert all(math.isclose(mesh['bounds'][1][i]-mesh['bounds'][0][i],body['boundsMm']['size'][i],abs_tol=.09) for i in range(3))
            meshes.append({'pitchMm':pitch,'mesh':mesh})
        call('view.home');call('view.fit')
        call('capture.screenshot',{'path':str(output/'coil-final.png')})
        (output/'report.json').write_text(json.dumps({'passed':True,'gpu':gpu,'final':inspect(),
            'meshes':meshes,'timings':timings,'transcript':transcript},indent=2))
    print('PASS: circular Coil preview, named pitch/source edit, retained turns, refusal, history/reopen and independently read STL')

if __name__=='__main__':main()
