#!/usr/bin/env python3
"""Sync Alloy's Release scope field from the accepted v1 release dependency graph.

Dry run by default. Uses gh/GraphQL; never changes issue state or board Status.
"""
import argparse
from collections import Counter
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]
PROJECT = 'PVT_kwHOAAPeGs4BmnSv'


def gh(*args, payload=None):
    result = subprocess.run(['gh', *args], input=json.dumps(payload) if payload else None,
                            text=True, check=True, capture_output=True)
    value = json.loads(result.stdout)
    if isinstance(value, dict) and value.get('errors'):
        raise RuntimeError(value['errors'])
    return value


def graphql(query):
    return gh('api', 'graphql', '--input', '-', payload={'query': query})['data']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apply', action='store_true', help='Update changed project scope values')
    args = parser.parse_args()
    issues = {i['key']: i for i in json.loads((ROOT / 'docs/backlog/issues.json').read_text())['issues']}
    manifest = json.loads((ROOT / 'docs/backlog/github.json').read_text())['issues']
    required = set()

    def visit(key):
        if key not in required:
            required.add(key)
            for dependency in issues[key]['depends_on']:
                visit(dependency)

    visit('REL-054')
    fields = gh('project', 'field-list', '1', '--owner', 'rje', '--format', 'json')['fields']
    field = next(f for f in fields if f['name'] == 'Release scope')
    options = {o['name']: o['id'] for o in field['options']}
    desired = {item['number']: (key, 'v1' if key in required else 'Later roadmap')
               for key, item in manifest.items() if key in issues}
    items, cursor = {}, None
    while True:
        after = ',after:' + json.dumps(cursor) if cursor else ''
        data = graphql('''{node(id:"%s"){... on ProjectV2{items(first:100%s){
          pageInfo{hasNextPage endCursor} nodes{id
          content{... on Issue{number repository{nameWithOwner}}}
          fieldValueByName(name:"Release scope"){... on ProjectV2ItemFieldSingleSelectValue{name}}
        }}}}}''' % (PROJECT, after))['node']['items']
        for item in data['nodes']:
            content = item.get('content') or {}
            if content.get('repository', {}).get('nameWithOwner') == 'rje/Alloy':
                items[content['number']] = item
        if not data['pageInfo']['hasNextPage']:
            break
        cursor = data['pageInfo']['endCursor']
    missing = sorted(set(desired) - set(items))
    if missing:
        raise RuntimeError(f'Canonical issues missing from project: {missing}')
    changes = []
    for number, (key, scope) in desired.items():
        current = (items[number].get('fieldValueByName') or {}).get('name')
        if current != scope:
            changes.append((items[number]['id'], scope))
    counts = Counter(scope for _, scope in desired.values())
    open_counts = Counter('v1' if key in required else 'Later roadmap' for key, item in manifest.items()
                          if key in issues and item['state'] == 'open')
    print(f'Scope totals: {dict(counts)}; open (manifest): {dict(open_counts)}; changes: {len(changes)}', flush=True)
    if args.apply:
        for offset in range(0, len(changes), 20):
            mutations = []
            for index, (item_id, scope) in enumerate(changes[offset:offset + 20]):
                mutations.append('m%d:updateProjectV2ItemFieldValue(input:{projectId:"%s",itemId:"%s",fieldId:"%s",value:{singleSelectOptionId:"%s"}}){projectV2Item{id}}'
                                 % (index, PROJECT, item_id, field['id'], options[scope]))
            graphql('mutation{' + ' '.join(mutations) + '}')
            print(f'Updated {min(offset + 20, len(changes))}/{len(changes)}', flush=True)


if __name__ == '__main__':
    main()
