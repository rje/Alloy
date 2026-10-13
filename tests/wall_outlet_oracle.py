"""Independent final geometry oracle for source wall-outlet-sketch-20261013.

Preparation for TUT-011, not a completed workflow receipt. The caller supplies
actual authored sketch and solver diagnostics from the running application.
Source derivation/provenance: external wall-outlet-audit/independent-contract.md.
"""
import math


def near(actual, expected, tolerance=1e-5):
    assert math.isclose(actual, expected, abs_tol=tolerance), (actual, expected)


def verify(sketch, diagnostics):
    """Check source dimensions, analytic arcs, retained symmetry and zero DOF."""
    p = sketch['parameters']
    points = {v['id']: v for v in p['points']}
    entities = {v['id']: v for v in p['entities']}
    assert len(entities) == 20, 'Expected outer six, axis, circle and two six-curve cutouts'
    assert diagnostics['dof'] == 0, diagnostics
    assert len(p['mirrorGroups']) == 1
    group = p['mirrorGroups'][0]
    assert len(group['entityPairs']) == 6 and len(group['pointPairs']) == 6
    source_ids = {v['sourceEntityId'] for v in group['entityPairs']}
    result_ids = {v['resultEntityId'] for v in group['entityPairs']}
    assert len(source_ids) == len(result_ids) == 6 and source_ids.isdisjoint(result_ids)

    def xy(point_id):
        q = points[point_id]
        return q['uMm'], q['vMm']

    axis = entities[group['axisEntityId']]
    assert axis['type'] == 'line' and axis['construction']
    for point_id in (axis['startPointId'], axis['endPointId']):
        x, y = xy(point_id)
        near(abs(x), 35)
        near(y, 0)
    circles = [v for v in entities.values() if v['type'] == 'circle']
    assert len(circles) == 1
    circle = circles[0]
    near(circle['radiusMm'], 6)
    for coordinate in xy(circle['centerPointId']):
        near(coordinate, 0)

    excluded = source_ids | result_ids | {axis['id'], circle['id']}
    outer = [e for eid, e in entities.items() if eid not in excluded]
    assert len(outer) == 6 and sum(bool(e.get('construction')) for e in outer) == 2
    for edge in outer:
        assert edge['type'] == 'line'
        for point_id in (edge['startPointId'], edge['endPointId']):
            x, y = xy(point_id)
            near(abs(x), 35)
            near(abs(y), 57)

    chord = math.sqrt(17**2 - 14**2)
    sweep = math.degrees(2 * math.asin(14 / 17))
    for ids, center_y in ((source_ids, 25), (result_ids, -25)):
        edges = [entities[i] for i in ids]
        arcs = [e for e in edges if e['type'] == 'arc']
        lines = [e for e in edges if e['type'] == 'line']
        assert len(arcs) == 2 and len(lines) == 4
        assert sum(bool(e.get('construction')) for e in lines) == 2
        for arc in arcs:
            near(arc['radiusMm'], 17)
            x, y = xy(arc['centerPointId'])
            near(x, 0)
            near(y, center_y)
            near(abs(arc['sweepAngleDeg']), sweep)
        for edge in edges:
            for point_id in (edge['startPointId'], edge['endPointId']):
                x, y = xy(point_id)
                near(abs(x), chord)
                near(abs(y - center_y), 14)
        for line in lines:
            a, b = xy(line['startPointId']), xy(line['endPointId'])
            near(a[0] if line.get('construction') else a[1],
                 b[0] if line.get('construction') else b[1])

    for pair in group['pointPairs']:
        a, b = xy(pair['sourcePointId']), xy(pair['resultPointId'])
        near(a[0], b[0])
        near(a[1], -b[1])
    for pair in group['entityPairs']:
        a, b = entities[pair['sourceEntityId']], entities[pair['resultEntityId']]
        assert a['type'] == b['type'] and bool(a.get('construction')) == bool(b.get('construction'))
        if a['type'] == 'arc':
            near(a['sweepAngleDeg'], -b['sweepAngleDeg'])
    dimensions = p['dimensions']
    assert len(dimensions) == 6 and all(d['driving'] for d in dimensions)
    assert sorted(round(d['valueMm'], 6) for d in dimensions) == [11, 12, 17, 28, 70, 114]
    assert sum(d['kind'] == 'parallelLineDistance' for d in dimensions) == 2
    outer_side_ids = {e['id'] for e in outer if not e.get('construction')}
    for endpoint in (axis['startPointId'], axis['endPointId']):
        assert any(c['type'] == 'midpoint' and c.get('pointIds') == [endpoint]
                   and len(c.get('entityIds', [])) == 1 and c['entityIds'][0] in outer_side_ids
                   for c in p['constraints']), 'Axis endpoint lost its outer-edge midpoint relation'
    source_arc_ids = {i for i in source_ids if entities[i]['type'] == 'arc'}
    for relation_type in ('concentric', 'equal'):
        assert any(c['type'] == relation_type and set(c.get('entityIds', [])) == source_arc_ids
                   for c in p['constraints']), 'Missing retained source-arc ' + relation_type
    arc_centers = {entities[i]['centerPointId'] for i in source_ids if entities[i]['type'] == 'arc'}
    assert any(c['type'] == 'vertical' and len(c.get('pointIds', [])) == 2
               and circle['centerPointId'] in c['pointIds']
               and bool(arc_centers.intersection(c['pointIds']))
               for c in p['constraints']), 'Missing retained circle-to-arc center Vertical relation'
    area = 2 * (14 * chord + 289 * math.asin(14 / 17))
    return {'outerBoundsMm': [-35, -57, 35, 57], 'cutoutAreaMm2': area,
            'circleAreaMm2': 36 * math.pi,
            'remainingOuterAreaMm2': 70 * 114 - 2 * area - 36 * math.pi,
            'entityCount': len(entities), 'dof': diagnostics['dof'],
            'mirrorGroupId': group['id']}
