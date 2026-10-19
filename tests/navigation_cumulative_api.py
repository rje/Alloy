#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Bounded NAV-005 cumulative real-entrypoint camera/geometry workflow.

API evidence only: physical pointer/key delivery and packaged/platform runtime
qualification remain separate. All authoring is setup; each navigation stage
checks authored data and undo state independently from camera/GPU evidence.
"""
import argparse
import json
import math
from pathlib import Path
from app_harness import application


def near(actual, expected, tolerance=1e-6):
    assert math.isclose(actual, expected, abs_tol=tolerance), (actual, expected)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--app', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    out = Path(args.output).resolve()
    stages = []
    with application(args.app, out) as (call, transcript):
        call('window.resize', {'width': 1360, 'height': 850})
        def state():
            return call('app.state')
        def camera():
            return state()['viewport']['navigation']
        def stable():
            doc = call('document.inspect')
            return {key: doc[key] for key in ('authored', 'undo', 'revision', 'evaluation')}
        def capture(name):
            current = state()
            gpu = current['viewport']['gpu']
            assert gpu['hardwareAccelerated'], gpu
            image = call('capture.screenshot', {'path': str(out / (name + '.png'))})
            stages.append({'name': name, 'viewport': current['viewport'], 'capture': image})
        def unchanged(before):
            assert stable() == before, 'Navigation changed authored data, history or evaluated geometry'
        call('solid.box', {'width': 46, 'depth': 27, 'height': 13, 'units': 'mm', 'name': 'Asymmetric stock'})
        doc = call('document.inspect')
        body = doc['evaluation']['bodies'][0]
        body_id = body['bodyId']
        top = next(face for face in body['supportFaces'] if face['role'] == 'box.z1')
        support = {key: top[key] for key in ('bodyId', 'producerFeatureId', 'faceId')}
        baseline = stable()
        call('view.home')
        home = camera()
        call('view.orbit', {'deltaX': 67, 'deltaY': 31})
        pose = camera()
        call('view.pan', {'x': 50, 'y': -24})
        call('view.zoom', {'factor': 1.3})
        call('view.fit')
        fitted = camera()
        assert fitted['orientation'] == 'custom'
        for key in ('customDirection', 'customUp', 'orbitPivot'):
            assert fitted[key] == pose[key]
        for key, value in (('zoom', 1), ('panX', 0), ('panY', 0)):
            near(fitted[key], value)
        capture('01-fit-custom-orientation')
        call('view.home')
        assert camera()['orientation'] == home['orientation'] == 'isometric'
        unchanged(baseline)

        start = camera()
        call('view.navigationTool', {'mode': 'pan'})
        call('view.pan', {'x': 38, 'y': -20})
        panned = camera()
        assert panned['orientation'] == start['orientation']
        near(panned['zoom'], start['zoom'])
        near(panned['panX'] - start['panX'], 38)
        near(panned['panY'] - start['panY'], -20)
        call('view.navigationTool', {'mode': 'zoom'})
        viewport = state()['viewport']
        call('view.zoom', {'factor': 1.5, 'x': viewport['width'] * .4, 'y': viewport['height'] * .45})
        zoomed = camera()
        near(zoomed['zoom'], panned['zoom'] * 1.5)
        call('view.navigationTool', {'mode': 'zoomWindow'})
        windowed = call('view.zoomWindow', {'x1': viewport['width'] * .25, 'y1': viewport['height'] * .25,
                                          'x2': viewport['width'] * .75, 'y2': viewport['height'] * .75})
        near(windowed['zoom'], zoomed['zoom'] * 2)
        assert windowed['orientation'] == zoomed['orientation']
        refused_camera = camera()
        call('view.zoomWindow', {'x1': 20, 'y1': 20, 'x2': 22, 'y2': 22}, error='invalid_arguments')
        assert camera() == refused_camera
        call('view.navigationTool', {'mode': 'select'})
        capture('02-pan-zoom-window')
        unchanged(baseline)

        call('selection.set', {'bodyId': body_id})
        call('view.orbit', {'deltaX': 30, 'deltaY': 15})
        pose = camera()
        selected = call('view.fitSelection')
        assert selected['customDirection'] == pose['customDirection']
        for key, value in zip(('centerX', 'centerY', 'centerZ'), (23, 13.5, 6.5)):
            near(selected[key], value)
        capture('03-selected-body')
        refused_camera = camera()
        call('view.lookAt', error='unsupported_selection')
        assert camera() == refused_camera
        call('selection.faceSet', {'support': support})
        face = call('view.lookAt')
        near(face['centerZ'], 13)
        for actual, normal in zip(face['customDirection'], top['frame']['normal']):
            near(actual, -normal)
        assert state()['selection']['support'] == support
        capture('04-look-at-planar-face')
        unchanged(baseline)

        call('sketch.begin', {'support': support, 'name': 'Small cap sketch', 'units': 'mm'})
        call('sketch.rectangle', {'x1': 5, 'y1': 5, 'x2': 15, 'y2': 11, 'units': 'mm'})
        draft = state()['activeSession']
        baseline = stable()
        call('view.lookAt')
        call('view.fitSelection')
        after = state()['activeSession']
        assert after['command'] == 'sketch.edit' and after['draftSketch'] == draft['draftSketch']
        unchanged(baseline)
        capture('05-active-small-sketch')
        call('sketch.finish')
        sketch_id = call('document.inspect')['evaluation']['sketches'][-1]['sketchId']
        call('selection.sketchSet', {'sketchId': sketch_id})
        baseline = stable()
        call('view.home')
        whole_scale = camera()['scale']
        small = call('view.fitSelection')
        assert small['scale'] < 20 and small['scale'] < whole_scale
        small = call('view.lookAt')
        for key, value in zip(('centerX', 'centerY', 'centerZ'), (10, 8, 13)):
            near(small[key], value)
        capture('06-finished-small-sketch')
        unchanged(baseline)

        call('solid.cylinder', {'radius': 4, 'height': 20, 'x': 32, 'y': 8,
                                'units': 'mm', 'name': 'Curved selection refusal'})
        cylinder = call('document.inspect')['evaluation']['bodies'][-1]
        curved = next(face for face in cylinder['supportFaces'] if not face['planar'])
        call('selection.faceSet', {'support': {key: curved[key]
             for key in ('bodyId', 'producerFeatureId', 'faceId')}})
        baseline = stable()
        refused_camera = camera()
        call('view.lookAt', error='unsupported_face')
        assert camera() == refused_camera
        unchanged(baseline)

        call('solid.box', {'width': 10, 'depth': 10, 'height': 10, 'x': 10000, 'units': 'mm', 'name': 'Distant hidden stock'})
        distant = call('document.inspect')['authored']['features'][-1]
        call('view.fit')
        distant_scale = camera()['scale']
        call('feature.visibility', {'featureId': distant['id'], 'visible': False})
        baseline = stable()
        call('view.fit')
        visible = camera()
        assert visible['scale'] < 100 and distant_scale > visible['scale'] * 100
        near(visible['centerX'], 23)
        capture('07-hidden-distant-body-excluded')
        unchanged(baseline)

        call('component.createFromBodies', {'bodyIds': [body_id]})
        source = call('document.inspect')['authored']['occurrences'][0]['id']
        placed = call('component.copyOccurrence', {'occurrenceId': source,
            'placement': {'axis': 'Z', 'angleDeg': 90, 'translationMm': [120, 30, 17]}})['rootOccurrenceId']
        baseline = stable()
        call('selection.set', {'bodyId': body_id}, error='ambiguous_instance')
        call('selection.set', {'bodyId': body_id, 'occurrenceId': placed})
        selected = call('view.fitSelection')
        for key, value in zip(('centerX', 'centerY', 'centerZ'), (106.5, 53, 23.5)):
            near(selected[key], value)
        assert state()['selection']['occurrenceId'] == placed
        capture('08-placed-assembly-child')
        placed_support = {**support, 'occurrenceId': placed}
        call('selection.faceSet', {'support': placed_support})
        face = call('view.lookAt')
        near(face['centerZ'], 30)
        near(face['centerX'], 106.5)
        near(face['centerY'], 53)
        capture('09-placed-child-face-look-at')
        unchanged(baseline)
        path = out / 'navigation-cumulative.alloy'
        call('document.save', {'path': str(path)})
        saved = call('document.inspect')['authored']
        call('document.open', {'path': str(path), 'discard': True})
        assert call('document.inspect')['authored'] == saved
        gpu = state()['viewport']['gpu']
        receipt = {'result': 'passed', 'stages': stages, 'gpu': gpu,
                   'hiddenBodyScale': distant_scale, 'visibleScale': visible['scale'],
                   'placedOccurrenceId': placed, 'savedModel': str(path),
                   'limits': ['API command workflow; no physical input claim',
                              'Development macOS runtime; no packaged or cross-platform claim',
                              'No matched public screenshot parity claim'], 'transcript': transcript}
        (out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print('PASS: cumulative Fit/Home, Pan/Zoom/Window, body/face/sketch, hidden bounds, placed child, refusal, no authored mutation and GPU captures')


if __name__ == '__main__':
    main()
