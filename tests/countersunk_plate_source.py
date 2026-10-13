"""Source-order stock and Point preparation for the pending #249 full gate.

No standalone pass: the cumulative runner must add Hole/Pattern, capture and
independently validate the final deliverables. Uses real shared commands only.
"""
import math


def create_stock_and_point(call, stage, capture):
    def inspect():
        return call('document.inspect')

    def session():
        return call('app.state')['activeSession']

    def draft():
        return session()['draftSketch']

    assert not inspect()['authored']['features'], 'Creation must start blank'
    stage('Change Parameters: Length75, Width50, Thickness10 mm')
    for name, value in [('Length', 75), ('Width', 50), ('Thickness', 10)]:
        call('document.parameterAdd', {'name': name, 'value': value, 'units': 'mm'})
    parameters = {p['name']: p for p in inspect()['userParameters']}

    stage('XY Center Rectangle at origin; named driving dimensions')
    call('sketch.begin', {'plane': 'XY', 'name': 'Plate outline'})
    call('sketch.lookAt')
    call('sketch.tool', {'tool': 'centerRectangle'})
    for u, v in [(0, 0), (33, 22)]:
        pixel = call('view.projectSketchPoint', {'sketchId': draft()['id'], 'uMm': u, 'vMm': v})
        call('sketch.placePoint', {'x': pixel['x'], 'y': pixel['y']})
    edges = draft()['parameters']['entities']
    assert len(edges) == 4 and not draft()['parameters']['dimensions']
    for kind, edge, name, label in [
            ('horizontal', edges[0], 'Length', (0, -32)),
            ('vertical', edges[1], 'Width', (45, 0))]:
        call('sketch.dimension', {'kind': kind, 'entityId': edge['id'],
             'parameterName': name, 'annotation': {'uMm': label[0], 'vMm': label[1]}})
    base = draft()
    bindings = {d['parameterId'] for d in base['parameters']['dimensions']}
    assert bindings == {parameters['Length']['id'], parameters['Width']['id']}
    assert session()['solverDiagnostics']['dof'] == 0
    capture('01-named-rectangle')
    call('sketch.finish')
    profile = inspect()['evaluation']['sketches'][0]['profiles'][0]['profileId']

    stage('Extrude Thickness; four vertical R5 Fillets')
    call('solid.extrude', {'sketchId': base['id'], 'profileId': profile,
                          'distanceParameter': 'Thickness', 'units': 'mm'})
    extrusion = inspect()['authored']['features'][-1]
    assert extrusion['parameters']['distanceParameterId'] == parameters['Thickness']['id']
    body = inspect()['evaluation']['bodies'][0]
    assert math.isclose(body['volumeMm3'], 37500, abs_tol=1e-5)
    corners = [e['edgeId'] for e in body['solidEdges'] if e['provenance']['role'] == 'sideCorner']
    assert len(corners) == 4
    call('solid.fillet', {'targetBodyId': body['bodyId'], 'edgeIds': corners, 'radiusMm': 5})
    body = inspect()['evaluation']['bodies'][0]
    assert math.isclose(body['volumeMm3'], 36500 + 250*math.pi, abs_tol=1e-5)
    call('view.home')
    capture('02-rounded-plate')

    stage('Top-face Point; driving10 mm from left and upper rims')
    top = next(f for f in body['supportFaces'] if f['role'] == 'extrude.endCap')
    call('sketch.begin', {'support': {k: top[k] for k in ('bodyId', 'producerFeatureId', 'faceId')},
                          'name': 'Hole placement'})
    call('sketch.lookAt')
    call('sketch.addPoint', {'x': -26, 'y': 14, 'units': 'mm'})
    entity = draft()['parameters']['entities'][0]
    assert entity['type'] == 'point'
    lines = session()['projectionContext']['lines']
    left = next(e for e in lines if abs(e['u1Mm']+37.5)<1e-6 and abs(e['u2Mm']+37.5)<1e-6)
    upper = next(e for e in lines if abs(e['v1Mm']-25)<1e-6 and abs(e['v2Mm']-25)<1e-6)
    for edge, label in [(left, (-32, 9)), (upper, (-20, 20))]:
        call('sketch.dimension', {'kind': 'pointLineDistance', 'pointIds': [entity['pointId']],
             'entityId': edge['id'], 'value': 10, 'units': 'mm',
             'annotation': {'uMm': label[0], 'vMm': label[1]}})
    assert session()['solverDiagnostics']['dof'] == 0
    point_sketch = draft()
    capture('03-two-edge-distances')
    call('sketch.finish')
    evaluated = inspect()['evaluation']['sketches'][-1]
    assert not evaluated['profiles'] and evaluated['standalonePointCount'] == 1
    point = next(p for p in evaluated['worldPoints'] if p['id'] == entity['pointId'])
    assert math.dist(point['positionMm'], [-27.5, 15, 10]) < 1e-6
    return {'parameters': parameters, 'baseSketchId': base['id'],
            'targetBodyId': body['bodyId'], 'sourceSketchId': point_sketch['id'],
            'sourceEntityId': entity['id'], 'sourcePointId': entity['pointId']}
