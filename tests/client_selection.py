"""Focused selection-policy regression; metadata only, no mocked CAD state."""
from pathlib import Path
import sys
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from alloyctl import choose_endpoint

records = [{'discoveryStatus': 'live', 'instanceId': 'first', 'socket': '/first'},
           {'discoveryStatus': 'live', 'instanceId': 'second', 'socket': '/second'}]
with patch('alloyctl.discover', return_value=records):
    assert choose_endpoint(instance='second', environment_endpoint='/first') == '/second'
    assert choose_endpoint(environment_endpoint='/first') == '/first'
    for selectors in ({'socket_path': '/first', 'instance': 'second'},
                      {'ready_file': '/first.json', 'instance': 'second'}):
        try:
            choose_endpoint(**selectors)
        except RuntimeError:
            pass
        else:
            raise AssertionError('Conflicting explicit selectors accepted')
print('PASS: explicit instance selection overrides environment; conflicting selectors refused')
