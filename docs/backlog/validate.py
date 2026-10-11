#!/usr/bin/env python3
"""Validate the local backlog before API-only publication; no network writes."""
import collections
from functools import cache
import json
from pathlib import Path
import re

root = Path(__file__).resolve().parent
data = json.loads((root / 'issues.json').read_text())
assert set(data) == {'version', 'issues'} and data['version'] == 1
assert isinstance(data['issues'], list) and data['issues']
required = {'key', 'title', 'kind', 'phase', 'priority', 'depends_on', 'labels', 'body'}
registry = json.loads((root / 'sources.json').read_text())
urls = {source['url'] for source in registry['sources'].values() if source['url']}
reference_ids = {source.get('reference_id') for source in registry['sources'].values()} - {None}
assert all(source['retrieved'] == registry['retrieved'] for source in registry['sources'].values())
issues = {}
for issue in data['issues']:
    assert set(issue) == required, issue.get('key')
    key = issue['key']
    assert re.fullmatch(r'[A-Z]+-\d{3}', key) and key not in issues, key
    assert issue['kind'] in {'implementation', 'research', 'epic', 'validation'}, key
    assert issue['priority'] in {'P0', 'P1', 'P2'}, key
    assert re.match(r'P\d+ ', issue['phase']), key
    assert all(isinstance(issue[field], str) and issue[field] for field in ('title', 'body')), key
    assert isinstance(issue['labels'], list) and all(isinstance(x, str) for x in issue['labels']), key
    assert len(issue['depends_on']) == len(set(issue['depends_on'])), key
    assert key not in issue['depends_on'], key
    for heading in ('## Scope', '## Acceptance criteria', '## Proposed verification and evidence', '## Public reference evidence', '## Limits and open questions'):
        assert heading in issue['body'], (key, heading)
    body_urls = re.findall(r'\]\((https://[^)]+)\)', issue['body'])
    body_refs = re.findall(r'reference `([^`]+)`', issue['body'])
    assert body_urls or body_refs, key
    assert all(url in urls for url in body_urls), (key, set(body_urls) - urls)
    assert all(ref in reference_ids for ref in body_refs), (key, set(body_refs) - reference_ids)
    assert '2026-10-11' in issue['body'], key
    if issue['kind'] == 'epic':
        assert 'child' in issue['body'] and 'stays open' in issue['body'], key
    issues[key] = issue
for key, issue in issues.items():
    assert all(dep in issues for dep in issue['depends_on']), (key, issue['depends_on'])

visiting, visited, ordered = set(), set(), []
def visit(key):
    assert key not in visiting, f'Dependency cycle at {key}'
    if key in visited:
        return
    visiting.add(key)
    for dep in issues[key]['depends_on']:
        visit(dep)
    visiting.remove(key)
    visited.add(key)
    ordered.append(key)
for key in issues:
    visit(key)

@cache
def ancestors(key):
    result = set()
    for dep in issues[key]['depends_on']:
        result.add(dep)
        result |= ancestors(dep)
    return result
# Every initial task is a prerequisite of the initial-release gate (including
# the standalone feature-level cloud inventory, reconciled at full release).
initial = {k for k, i in issues.items() if int(i['phase'].split()[0][1:]) <= 4}
assert initial - {'GATE-050', 'FULL-001'} <= ancestors('GATE-050')
assert set(issues) - {'GATE-110'} <= ancestors('GATE-110'), 'Full gate misses an open requirement'
gates = [k for k in issues if k.startswith('GATE-')]
for before, after in zip(gates, gates[1:]):
    assert before in ancestors(after), (before, after)
assert not issues['GATE-000']['depends_on'], 'Existing shell gate must not require future CAD'
# The JSON order must support publisher linking dependencies to prior issues.
positions = {key: i for i, key in enumerate(issues)}
assert all(positions[d] < positions[k] for k, v in issues.items() for d in v['depends_on'])
summary = {
    'issues': len(issues),
    'kinds': dict(collections.Counter(i['kind'] for i in issues.values())),
    'phases': dict(sorted(collections.Counter(i['phase'] for i in issues.values()).items())),
    'validation_gates': gates,
    'structurally_ready': [k for k, i in issues.items() if not i['depends_on']],
    'source_entries': len(registry['sources']),
    'unique_source_urls': len(urls),
    'checks': ['schema', 'unique keys', 'existing dependencies', 'acyclic', 'topological publication order', 'dated registered primary-source links', 'cumulative gate chain', 'initial and full scope reachability'],
}
print(json.dumps(summary, indent=2))
