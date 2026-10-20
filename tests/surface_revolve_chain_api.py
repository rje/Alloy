#!/usr/bin/env python3
"""SRF-037 connected Line-chain fixture through the real application."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application

parser = argparse.ArgumentParser()
parser.add_argument('--app', required=True)
parser.add_argument('--output', required=True)
args = parser.parse_args()
out = Path(args.output).resolve()
out.mkdir(parents=True, exist_ok=True)


def near(a, b):
    assert math.isclose(a, b, abs_tol=2e-5), (a, b)


with application(args.app, out, command_timeout=90) as (call, transcript):
    try:
        def inspect():
            return call('document.inspect')

        def unchanged(before):
            after = inspect()
            for key in ('authored', 'revision', 'undo', 'evaluation'):
                assert after[key] == before[key], key

        def sheet(state, angle=90, reverse=False, end_height=10, axis_x=0):
            b = state if 'areaMm2' in state else state['evaluation']['bodies'][0]
            assert b['valid'] and b['type'] == 'surface' and not b['closed']
            assert b['topology']['faces'] == 2 and b['topology']['edges'] == 7
            assert b['topology']['shells'] == 1 and b['topology']['solids'] == 0
            assert b['volumeMm3'] is None
            near(b['areaMm2'], math.radians(angle)*(25-axis_x)*(math.sqrt(200)+math.sqrt(100+end_height**2)))
            theta = math.radians(angle)
            expected_min = [axis_x+(20-axis_x)*math.cos(theta), -(30-axis_x)*math.sin(theta) if reverse else 0, -10]
            expected_max = [30, 0 if reverse else (30-axis_x)*math.sin(theta), end_height]
            for a, e in zip(b['boundsMm']['min'], expected_min): near(a, e)
            for a, e in zip(b['boundsMm']['max'], expected_max): near(a, e)
            return b

        call('window.resize', {'width': 1280, 'height': 900})
        environment = call('app.state')
        call('sketch.create', {'plane': 'XZ', 'name': 'Connected conical section'})
        # The extra Lines permit atomic disconnected, branched, crossing and self-crossing refusals.
        for coords in [(20,-10,30,0),(30,0,20,10),(0,-15,0,15),(40,0,50,5),
                       (30,0,40,0),(-5,0,5,0),(20,-10,30,10),(30,10,20,10),(20,10,30,-10)]:
            call('sketch.line', dict(zip(('x1','y1','x2','y2'),coords)) | {'units':'mm'})
        draft = call('app.state')['activeSession']['draftSketch']
        entities = draft['parameters']['entities']
        ids = [e['id'] for e in entities]
        sid = draft['id']
        call('sketch.construction', {'entityIds':[ids[2]], 'construction':True})
        call('sketch.finish')
        inputs = {'sketchId':sid, 'entityIds':ids[:2], 'axisEntityId':ids[2]}
        before = inspect()
        session = call('command.begin', {'command':'surface.revolve','inputs':inputs})
        sheet(call('command.preview', {'session':session['id']})['geometry'])
        panel = call('app.state')['ui']['activeDialog']
        assert panel['visible'] and panel['inputs']['entityIds'] == ids[:2]
        call('capture.screenshot', {'path':str(out/'chain-dock-preview.png')})
        call('command.cancel', {'session':session['id']})
        unchanged(before)
        for invalid in ([ids[0],ids[3]], [ids[0],ids[1],ids[4]], [ids[0],ids[5]], ids[6:9]):
            call('surface.revolve', inputs | {'entityIds':invalid}, error='evaluation_failed')
            unchanged(before)
        session = call('command.begin', {'command':'surface.revolve','inputs':inputs})
        call('command.update', {'session':session['id'],'inputs':{'entityIds':[ids[0],ids[3]]}}, error='evaluation_failed')
        unchanged(before)
        call('command.update', {'session':session['id'],'inputs':{'entityIds':ids[:2]}})
        call('command.commit', {'session':session['id']})
        initial = inspect()
        b = sheet(initial)
        feature = initial['authored']['features'][-1]
        fid = feature['id']
        assert feature['parameters']['sectionVersion'] == 2
        assert feature['parameters']['entityIds'] == ids[:2]
        assert b['sourceEntityIds'] == ids[:2]
        assert [x['entityId'] for x in b['orientedSources']] == ids[:2]
        face_ids = [f['faceId'] for f in b['supportFaces']]
        edge_ids = [e['edgeId'] for e in b['solidEdges']]
        assert len(set(edge_ids)) == 7
        call('view.home')
        call('capture.screenshot', {'path':str(out/'chain-quarter-sheet.png')})
        edit = call('feature.editDialog', {'featureId':fid})
        assert edit['inputs']['entityIds'] == ids[:2]
        call('command.update', {'session':edit['id'],'inputs':{'angleDeg':45,'reverseDirection':True}})
        sheet(call('command.preview', {'session':edit['id']})['geometry'],45,True)
        call('command.commit', {'session':edit['id']})
        edited = inspect()
        sheet(edited,45,True)
        call('history.undo'); assert inspect()['authored'] == initial['authored']
        call('history.redo'); assert inspect()['authored'] == edited['authored']
        call('sketch.edit', {'sketchId':sid})
        call('sketch.movePoint', {'pointId':entities[1]['endPointId'],'x':20,'y':20,'units':'mm'})
        call('sketch.finish')
        final = inspect()
        sheet(final,45,True,20)
        call('history.undo'); assert inspect()['authored'] == edited['authored']
        call('history.redo'); assert inspect()['authored'] == final['authored']
        call('sketch.edit', {'sketchId':sid})
        for point, height in ((entities[2]['startPointId'],-15),(entities[2]['endPointId'],15)):
            call('sketch.movePoint', {'pointId':point,'x':5,'y':height,'units':'mm'})
        call('sketch.finish')
        shifted = inspect()
        sheet(shifted,45,True,20,5)
        call('history.undo'); assert inspect()['authored'] == final['authored']
        call('history.redo'); assert inspect()['authored'] == shifted['authored']
        call('feature.visibility', {'featureId':sid,'visible':False})
        final = inspect()
        model = out/'connected-surface-revolve.alloy'
        call('document.save', {'path':str(model)})
        call('document.new', {'discard':True})
        call('document.open', {'path':str(model),'discard':True})
        reopened = inspect()
        assert reopened['authored'] == final['authored']
        b = sheet(reopened,45,True,20,5)
        assert [f['faceId'] for f in b['supportFaces']] == face_ids
        assert [e['edgeId'] for e in b['solidEdges']] == edge_ids
        call('view.home')
        capture = call('capture.screenshot', {'path':str(out/'edited-reopened-chain.png')})
        (out/'result.json').write_text(json.dumps({'passed':True,'environment':environment,'final':reopened,'capture':capture,
            'limits':'Line chains only; explicit dock selection inspected through API. Physical canvas picks/angle drag and matched public visual comparison remain separate.'},indent=2)+'\n')
    finally:
        (out/'transcript.json').write_text(json.dumps(transcript,indent=2)+'\n')
