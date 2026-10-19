#!/usr/bin/env python3
"""DOC-042: real-entrypoint Measure ambiguity, identity and current references."""
import argparse
import json
import math
from pathlib import Path
from app_harness import application

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--app', required=True)
p.add_argument('--output', required=True)
a = p.parse_args()
out = Path(a.output).resolve()

def near(actual, expected):
    assert math.isclose(actual, expected, rel_tol=1e-8, abs_tol=1e-6), (actual, expected)

with application(a.app, out) as (call, transcript):
    def measure(sources, name='Length', expected=None, error=None):
        before = call('document.inspect')
        result = call('inspect.measureResult', {'sources': sources}, error=error)
        assert call('document.inspect') == before
        if not error:
            assert result['sources'] == sources and not result['authoredChange']
            value = next(m['canonicalValue'] for m in result['measurements'] if m['name'] == name)
            if expected is not None:
                near(value, expected)
        return result

    def line_sketch(name, overlapping=False):
        call('sketch.begin', {'plane': 'XY', 'name': name})
        call('sketch.line', {'x1': 0, 'y1': 10, 'x2': 10, 'y2': 10, 'units': 'mm'})
        draft = call('app.state')['activeSession']['draftSketch']
        line = draft['parameters']['entities'][-1]
        if overlapping:
            call('sketch.line', {'x1': 2, 'y1': 10, 'x2': 8, 'y2': 10, 'units': 'mm'})
        call('sketch.finish')
        return draft['id'], line

    sid, line = line_sketch('Retained measured line')
    source = {'kind': 'sketchEdge', 'sketchId': sid, 'entityId': line['id']}
    call('sketch.begin', {'plane': 'XY', 'name': 'Placed-edge distance datum'})
    call('sketch.addPoint', {'x': 0, 'y': -6, 'units': 'mm'})
    point_draft = call('app.state')['activeSession']['draftSketch']
    datum = {'kind': 'sketchPoint', 'sketchId': point_draft['id'],
             'pointId': point_draft['parameters']['entities'][-1]['pointId']}
    call('sketch.finish')
    other_sid, other_line = line_sketch('Overlapping visible line', overlapping=True)
    measure([source], expected=10)
    measure([{**source, 'entityId': 'missing'}], error='measure_unavailable')
    measure([source, {**source, 'positionMm': [999, 999, 999]}], error='measure_duplicate')
    call('inspect.measure')
    call('view.orientation', {'name': 'top'})
    call('view.fit')
    pixel = call('view.projectSketchPoint', {'sketchId': sid, 'uMm': 5, 'vMm': 10})
    before = call('document.inspect')
    call('selection.pick', {'x': pixel['x'], 'y': pixel['y']}, error='measure_ambiguous')
    assert call('document.inspect') == before
    call('inspect.measureResult', error='measure_selection_count')
    # Removing one overlapping source makes the remaining nearest hit deterministic.
    call('feature.visibility', {'featureId': other_sid, 'visible': False})
    picked = call('selection.pick', {'x': pixel['x'], 'y': pixel['y']})
    assert picked['sources'] == [source]
    call('selection.pick', {'x': pixel['x'], 'y': pixel['y']}, error='measure_duplicate')
    assert call('inspect.measureResult')['sources'] == [source]
    measure([{'kind': 'sketchEdge', 'sketchId': other_sid, 'entityId': other_line['id']}], error='measure_unavailable')
    call('capture.screenshot', {'path': str(out/'resolved-line.png')})
    # Move the authored endpoint; the same source IDs must evaluate current geometry.
    call('sketch.edit', {'sketchId': sid})
    call('sketch.movePoint', {'pointId': line['endPointId'], 'x': 20, 'y': 10, 'units': 'mm'})
    call('sketch.finish')
    measure([source], expected=20)
    call('history.undo'); measure([source], expected=10)
    call('history.redo'); measure([source], expected=20)
    call('feature.delete', {'featureId': sid})
    measure([source], error='measure_unavailable')
    call('history.undo'); measure([source], expected=20)
    call('history.redo'); measure([source], error='measure_unavailable')
    call('history.undo'); measure([source], expected=20)
    # Stable face ID follows an evaluated parameter edit and placed instances.
    call('document.parameterAdd', {'name': 'Width', 'value': 20, 'units': 'mm'})
    parameter = call('document.inspect')['userParameters'][0]['id']
    call('solid.box', {'widthParameter': 'Width', 'depth': 10, 'height': 5, 'units': 'mm'})
    body = call('document.inspect')['evaluation']['bodies'][0]
    face = next(f for f in body['supportFaces'] if f['role'] == 'box.z1')
    planar = {'kind': 'face', 'bodyId': body['bodyId'], 'faceId': face['faceId']}
    measure([planar], 'Area', 200)
    measure([planar, {**planar, 'producerFeatureId': face['producerFeatureId']}], error='measure_duplicate')
    call('document.parameterUpdate', {'parameterId': parameter, 'name': 'Width', 'value': 30})
    measure([planar], 'Area', 300)
    call('history.undo'); measure([planar], 'Area', 200)
    call('history.redo'); measure([planar], 'Area', 300)
    call('component.createFromBodies', {'bodyIds': [body['bodyId']]})
    first = call('document.inspect')['authored']['occurrences'][0]['id']
    measure([planar, {**planar, 'occurrenceId': first}], error='measure_duplicate')
    copied = call('component.copyOccurrence', {'occurrenceId': first,
        'placement': {'axis': 'Z', 'angleDeg': 0, 'translationMm': [100, 0, 0]}})
    second = copied['rootOccurrenceId']
    placed = {**planar, 'occurrenceId': second}
    vertical = next(e for f in body['supportFaces'] for e in f['edges'] if e.get('boxEdge') == 'x0y0z*')
    placed_edge = {'kind': 'edge', 'bodyId': body['bodyId'], 'edgeId': vertical['edgeId'], 'occurrenceId': second}
    measure([datum, {**placed_edge, 'occurrenceId': first}], 'Distance', 6)
    measure([datum, placed_edge], 'Distance', math.hypot(100, 6))
    measure([planar], error='measure_ambiguous')
    measure([placed], 'Area', 300)
    call('history.undo'); measure([placed], error='measure_unavailable')
    call('history.redo'); measure([placed], 'Area', 300)
    measure([datum, placed_edge], 'Distance', math.hypot(100, 6))
    saved = out/'measure-references.alloy'
    call('document.save', {'path': str(saved)})
    authored = call('document.inspect')['authored']
    measure([source], expected=20); measure([placed], 'Area', 300)
    call('document.save', {'path': str(out/'measure-references-after.alloy')})
    assert saved.read_bytes() == (out/'measure-references-after.alloy').read_bytes()
    call('document.new', {'discard': False})
    call('document.open', {'path': str(saved)})
    assert call('document.inspect')['authored'] == authored
    measure([source], expected=20); measure([placed], 'Area', 300)
    measure([planar], error='measure_ambiguous')
    measure([datum, placed_edge], 'Distance', math.hypot(100, 6))
    gpu = call('app.state')['viewport']['gpu']
    (out/'receipt.json').write_text(json.dumps({'status': 'passed', 'calls': len(transcript),
        'sources': {'line': source, 'placedFace': placed, 'placedEdge': placed_edge, 'datum': datum}, 'gpu': gpu,
        'transcript': transcript}, indent=2)+'\n')
print('PASS Measure source ambiguity and retained reference lifecycle')
