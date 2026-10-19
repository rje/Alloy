#!/usr/bin/env python3
"""Bounded New Body cone/torus lifecycle through the real GPU application."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application
from stl_export_api import inspect_stl


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    output = Path(args.output).resolve()
    meshes = []
    with application(args.app, output) as (call, transcript):
        gpu = call('app.state')['viewport']['gpu']
        assert gpu['hardwareAccelerated']
        baseline = call('document.inspect')
        cone_inputs = {'baseDiameter':20, 'topDiameter':6, 'height':24,
                       'x':-28, 'y':0, 'z':2, 'units':'mm'}
        session = call('command.begin', {'command':'solid.cone', 'inputs':cone_inputs})
        preview = call('command.preview', {'session':session['id']})
        assert math.isclose(preview['geometry']['volumeMm3'],1112*math.pi,rel_tol=1e-10)
        call('capture.dialog', {'path':str(output/'cone-panel.png')})
        call('capture.screenshot', {'path':str(output/'cone-preview.png')})
        call('command.cancel', {'session':session['id']})
        assert call('document.inspect') == baseline
        call('solid.cone', cone_inputs)
        first = call('document.inspect')
        cone = first['authored']['features'][0]
        session = call('primitive.torus')
        call('command.preview', {'session':session['id']})
        call('capture.dialog', {'path':str(output/'torus-panel.png')})
        call('command.update', {'session':session['id'], 'inputs':{'x':24,'z':8}})
        call('command.commit', {'session':session['id']})
        created = call('document.inspect')
        torus = created['authored']['features'][1]
        assert created['bodies']==2 and created['undo']['count']==2
        for body, volume, size in zip(created['evaluation']['bodies'],
                                     [1112*math.pi,448*math.pi**2], [[20,20,24],[36,36,8]]):
            assert body['valid'] and body['closed']
            assert math.isclose(body['volumeMm3'],volume,rel_tol=1e-10)
            assert all(math.isclose(a,b,abs_tol=1e-6) for a,b in zip(body['boundsMm']['size'],size))
        # A retained downstream Move must recompute after a primitive edit.
        call('solid.move', {'targetBodyId':cone['bodyId'], 'steps':[
            {'xMm':0,'yMm':0,'zMm':3,'xDeg':0,'yDeg':0,'zDeg':0}]})
        session = call('feature.editDialog', {'featureId':cone['id']})
        call('command.update', {'session':session['id'], 'inputs':{'height':30}})
        call('command.preview', {'session':session['id']})
        call('command.commit', {'session':session['id']})
        edited_cone = call('document.inspect')
        assert edited_cone['authored']['features'][0]['bodyId']==cone['bodyId']
        assert math.isclose(edited_cone['evaluation']['bodies'][0]['volumeMm3'],1390*math.pi,rel_tol=1e-10)
        assert math.isclose(edited_cone['evaluation']['bodies'][0]['boundsMm']['min'][2],5,abs_tol=1e-6)
        call('solid.torus', {'featureId':torus['id'],'majorRadius':14,'minorRadius':3,
                             'x':24,'z':8,'units':'mm'})
        edited = call('document.inspect')
        assert edited['authored']['features'][1]['id']==torus['id']
        assert edited['authored']['features'][1]['bodyId']==torus['bodyId']
        call('history.undo')
        assert call('document.inspect')['authored']==edited_cone['authored']
        call('history.redo')
        assert call('document.inspect')['authored']==edited['authored']
        edited = call('document.inspect')
        call('feature.edit', {'featureId':torus['id'],'parameters':{'minorRadiusMm':14}}, 'evaluation_failed')
        assert call('document.inspect')==edited
        call('feature.edit', {'featureId':cone['id'],'parameters':{'heightMm':0}}, 'evaluation_failed')
        assert call('document.inspect')==edited
        session = call('feature.editDialog', {'featureId':torus['id']})
        call('command.update', {'session':session['id'],'inputs':{'minorRadius':14}})
        call('command.preview', {'session':session['id']}, 'command_failed')
        assert not call('app.state')['viewport']['gpu']['previewVisible']
        call('command.update', {'session':session['id'],'inputs':{'minorRadius':2}})
        call('command.preview', {'session':session['id']})
        call('command.cancel', {'session':session['id']})
        assert call('document.inspect')==edited
        # Active inch units convert all dimensions and placement once.
        call('solid.cone', {'baseDiameter':1,'topDiameter':0,'height':1,'x':3,'units':'in'})
        inch = call('document.inspect')['authored']['features'][-1]['parameters']
        assert inch['baseDiameterMm']==25.4 and math.isclose(inch['xMm'],76.2)
        call('history.undo')
        assert call('document.inspect')['authored']==edited['authored']
        native = output/'cone-torus.alloy'
        call('document.save', {'path':str(native)})
        saved = call('document.inspect')
        call('document.new', {'discard':False})
        call('document.open', {'path':str(native)})
        reopened = call('document.inspect')
        assert reopened['authored']==saved['authored']
        assert reopened['evaluation']['bodies']==saved['evaluation']['bodies']
        assert not reopened['dirty'] and reopened['undo']['count']==0
        for body, name, volume, expected_euler in zip(reopened['evaluation']['bodies'],
                ['cone','torus'], [1390*math.pi,252*math.pi**2], [2,0]):
            path = output/(name+'.stl')
            call('document.exportStl', {'path':str(path),'bodyId':body['bodyId'],
                'units':'mm','format':'binary','surfaceDeviationMm':0.03,'normalDeviationDeg':8})
            mesh = inspect_stl(path, True)
            assert len(mesh['components'])==1 and mesh['eulerCharacteristic']==expected_euler
            assert math.isclose(mesh['signedVolume'],volume,rel_tol=0.015)
            size=[mesh['bounds'][1][i]-mesh['bounds'][0][i] for i in range(3)]
            assert all(math.isclose(a,b,abs_tol=0.08) for a,b in zip(size,body['boundsMm']['size']))
            meshes.append({'name':name,'mesh':mesh})
        call('view.home')
        call('capture.screenshot', {'path':str(output/'cone-torus-final.png')})
        (output/'report.json').write_text(json.dumps({'passed':True,'gpu':gpu,
            'final':reopened,'meshes':meshes,'transcript':transcript},indent=2))
    print('PASS: cone/torus preview, retained edit, downstream Move, refusal, history, reopen and independent STL')


if __name__=='__main__':
    main()
