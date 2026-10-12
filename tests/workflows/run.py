#!/usr/bin/env python3
"""Cumulative real-Alloy evidence. Missing coverage blocks gates; no mock CAD state."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import platform
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from backlog import GitHub, load_backlog
LEDGER = ROOT / 'tests/workflows/ledger.json'


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def blockers(required_cases, completed, required_caps, observed, dependencies=None):
    reasons = [f'Missing case: {key}' for key in sorted(set(required_cases) - set(completed))]
    reasons += [f'Missing capability: {key}' for key in sorted(set(required_caps) - set(observed))]
    reasons += [f'Unmet prerequisite: {key} ({state})' for key, state in sorted((dependencies or {}).items()) if state != 'closed']
    return reasons


def ancestors(key, issues):
    result, pending = set(), list(issues[key]['depends_on'])
    while pending:
        dependency = pending.pop()
        if dependency not in result:
            result.add(dependency)
            pending.extend(issues[dependency]['depends_on'])
    return result


def expand_cases(names, cases):
    ordered, visiting = [], set()
    def visit(name):
        if name in ordered:
            return
        if name not in cases or name in visiting:
            raise ValueError(f'Missing or cyclic case: {name}')
        visiting.add(name)
        for dependency in cases[name].get('requires', []):
            visit(dependency)
        visiting.remove(name)
        ordered.append(name)
    for name in names:
        visit(name)
    return ordered


def run_logged(command, path, timeout):
    with path.open('w') as log:
        result = subprocess.run(command, cwd=ROOT, stdout=log, stderr=subprocess.STDOUT, timeout=timeout)
    return {'command': command, 'returncode': result.returncode, 'log': str(path)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    select = parser.add_mutually_exclusive_group(required=True)
    select.add_argument('--case', help='shell_api or FND-010; reviewed cases live in ledger.json')
    select.add_argument('--gate', help='Backlog gate key; prerequisites checked live through gh')
    parser.add_argument('--cumulative', action='store_true', help='Required for gates after GATE-000')
    parser.add_argument('--evidence', required=True)
    default_app = 'build/alloy.app/Contents/MacOS/alloy' if sys.platform == 'darwin' else ('build/Release/alloy.exe' if sys.platform == 'win32' else 'build/alloy')
    parser.add_argument('--app', default=str(ROOT / default_app))
    parser.add_argument('--build', action='store_true', help='Build the configured build/ directory before running')
    parser.add_argument('--exclude-case', action='append', default=[], help='Explicit omission for refusal verification')
    parser.add_argument('--omit-capability', action='append', default=[], help='Mask observed evidence to verify refusal; always recorded')
    args = parser.parse_args()
    output = Path(args.evidence).resolve()
    output.mkdir(parents=True, exist_ok=True)
    run_dir = Path(tempfile.mkdtemp(prefix='run-', dir=output))
    app = Path(args.app).resolve()
    dimensions = {name: {'status': 'not_run', 'reason': 'No applicable case has produced this evidence.'} for name in ('build', 'api', 'hardware', 'geometry', 'save_reopen', 'export', 'physical_input', 'visual')}
    report = {'schema': 'alloy-workflows/1', 'recorded_at': datetime.now(timezone.utc).isoformat(), 'selection': {'case': args.case, 'gate': args.gate, 'cumulative': args.cumulative}, 'candidate': {'app': str(app), 'source_commit': None, 'dirty': None}, 'host': {'system': platform.platform(), 'architecture': platform.machine(), 'python': platform.python_version()}, 'cases': {}, 'dimensions': dimensions, 'omissions': {'cases': args.exclude_case, 'capabilities': args.omit_capability}, 'blocking_reasons': [], 'limits': ['Only real shell and runner-integrity cases exist today.', 'A capture alone does not qualify physical input or visual compatibility.', 'Case success does not close GitHub issues or publish builds.']}
    completed, observed, covered = set(), set(), set()
    required_caps, required_dimensions, dependencies, required_tasks = set(), set(), {}, set()
    inputs, names = {}, []
    try:
        ledger = json.loads(LEDGER.read_text())
        issues = {issue['key']: issue for issue in load_backlog()}
        report['candidate']['source_commit'] = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
        report['candidate']['dirty'] = bool(subprocess.check_output(['git', 'status', '--porcelain', '--untracked-files=normal'], cwd=ROOT, text=True).strip())
        inputs = {str(LEDGER.relative_to(ROOT)): digest(LEDGER), 'docs/backlog/issues.json': digest(ROOT / 'docs/backlog/issues.json'), 'tests/workflows/run.py': digest(Path(__file__))}
        for path in [ROOT / 'CMakeLists.txt', ROOT / 'tools/alloyctl.py', ROOT / 'tools/backlog.py', *sorted((ROOT / 'src').glob('*'))]:
            if path.is_file():
                inputs[str(path.relative_to(ROOT))] = digest(path)
        if ledger['version'] != 1:
            raise ValueError('Unsupported ledger version')
        if args.gate:
            if args.gate not in issues or issues[args.gate]['kind'] != 'validation':
                raise ValueError(f'Unknown gate: {args.gate}')
            if args.gate != 'GATE-000' and not args.cumulative:
                raise ValueError('Later gates require --cumulative')
            predecessors = ancestors(args.gate, issues)
            inherited_gates = predecessors | {args.gate}
            required_tasks = {key for key in predecessors if issues[key]['kind'] != 'validation'}
            for gate in sorted(inherited_gates):
                if issues[gate]['kind'] == 'validation':
                    if gate not in ledger['gates']:
                        report['blocking_reasons'].append(f'Gate evidence specification not implemented: {gate}')
                    requirement = ledger['gates'].get(gate, {})
                    names += requirement.get('cases', [])
                    required_caps.update(requirement.get('capabilities', []))
                    required_dimensions.update(requirement.get('dimensions', []))
            names += [name for name, case in ledger['cases'].items() if set(case.get('covers', [])) & required_tasks]
            report['inherited_gates'] = sorted(key for key in inherited_gates if issues[key]['kind'] == 'validation')
            report['required_task_coverage'] = sorted(required_tasks)
            if predecessors:
                records = GitHub('rje/Alloy').existing()
                dependencies = {key: records.get(key, {}).get('state', 'missing') for key in predecessors}
                report['prerequisites'] = {'source': 'live GitHub API', 'observed_at': datetime.now(timezone.utc).isoformat(), 'states': dependencies}
        else:
            names = [args.case]
        names = expand_cases(names, ledger['cases'])
        report['required_cases'] = names
        required_caps.update(cap for name in names for cap in ledger['cases'][name]['capabilities'])
        if args.build:
            if app != (ROOT / default_app).resolve():
                raise ValueError('--build requires the exact configured default app output; custom candidates must be tested as prebuilt')
            cache = {}
            for line in (ROOT / 'build/CMakeCache.txt').read_text().splitlines():
                if '=' in line and not line.startswith(('#', '//')):
                    key, value = line.split('=', 1)
                    cache[key.split(':', 1)[0]] = value
            if not cache.get('CMAKE_HOME_DIRECTORY') or Path(cache['CMAKE_HOME_DIRECTORY']).resolve() != ROOT:
                raise ValueError('Configured CMake source root does not match this checkout')
            build = run_logged(['cmake', '--build', str(ROOT / 'build'), '--target', 'alloy', '--config', 'Release'], run_dir / 'build.log', 300)
            build['generator'] = cache.get('CMAKE_GENERATOR')
            build['configuration'] = cache.get('CMAKE_BUILD_TYPE') or 'Release'
            build['cache_sha256'] = digest(ROOT / 'build/CMakeCache.txt')
            dimensions['build'] = {'status': 'passed' if build['returncode'] == 0 else 'failed', **build}
            if build['returncode']:
                raise ValueError('Candidate build failed')
        else:
            dimensions['build']['reason'] = 'Prebuilt candidate; compilation was not performed by this run.'
        report['candidate']['sha256'] = digest(app)
        for name in names:
            spec = ledger['cases'][name]
            if name in args.exclude_case:
                report['cases'][name] = {'status': 'skipped', 'reason': 'Explicit --exclude-case omission.'}
                continue
            missing = set(spec.get('requires', [])) - completed
            if missing:
                report['cases'][name] = {'status': 'blocked', 'reason': f'Missing prerequisite cases: {sorted(missing)}'}
                continue
            if spec.get('check') == 'coverage_guards':
                # Fault injection exercises the coverage decision using real shell observations.
                cap = 'shell.commands'
                checks = {
                    'missing_case_refused': bool(blockers(['shell_api', 'absent-case'], completed, [], observed)),
                    'missing_capability_refused': bool(blockers([], completed, [cap], observed - {cap})),
                    'unavailable_modeling_refused': bool(blockers([], completed, ['cad.authored_geometry'], observed)),
                    'open_prerequisite_refused': bool(blockers([], completed, [], observed, {'fault-injected-prerequisite': 'open'})),
                }
                if not all(checks.values()):
                    raise ValueError('Coverage refusal checks failed')
                report['cases'][name] = {'status': 'passed', 'checks': checks, 'note': 'Validation-metadata fault injection only; no application/model state was mocked.'}
                observed.update(spec['capabilities'])
            else:
                case_dir = run_dir / name
                case_dir.mkdir()
                for key in ('script', 'fixture'):
                    inputs[spec[key]] = digest(ROOT / spec[key])
                command = [sys.executable, '-E', str(ROOT / spec['script']), '--app', str(app), '--output', str(case_dir), '--fixture', str(ROOT / spec['fixture'])]
                execution = run_logged(command, case_dir / 'case.log', 60)
                if execution['returncode']:
                    report['cases'][name] = {'status': 'failed', **execution}
                    continue
                result = json.loads((case_dir / spec['result']).read_text())
                if result['result'] != 'passed' or result['fixtureSha256'] != inputs[spec['fixture']]:
                    raise ValueError(f'{name}: invalid or mismatched fresh case receipt')
                caps = set(result['verifiedCapabilities'])
                if set(spec['capabilities']) - caps:
                    raise ValueError(f'{name}: case did not attest its required capabilities')
                observed.update(caps)
                report['cases'][name] = {'status': 'passed', 'verified_capabilities': sorted(caps), 'receipt': str(case_dir / spec['result']), **execution}
                state = result['initialState']
                report['runtime_dependencies'] = state['runtime']
                dimensions['api'] = {'status': 'passed', 'receipt': str(case_dir / spec['result']), 'commands': len(result['transcript'])}
                dimensions['hardware'] = {'status': 'passed', 'diagnostics': result['gpuChecks']['startupBeforeCapture']}
                for dimension in ('geometry', 'save_reopen', 'export'):
                    dimensions[dimension] = {'status': 'not_applicable', 'reason': 'Shell case has no authored CAD document; a later CAD gate requires its own real cases.'}
                dimensions['visual'] = {'status': 'captured', 'receipt': str(case_dir / 'alloy-shell.png'), 'reason': 'Capture produced; no current operator visual-compatibility review.'}
                dimensions['physical_input'] = {'status': 'not_run', 'reason': 'API actions are not physical keyboard/mouse evidence.'}
            completed.add(name)
            covered.update(spec.get('covers', []))
        observed.difference_update(args.omit_capability)
        report['blocking_reasons'] += blockers(names, completed, required_caps, observed, dependencies)
        report['blocking_reasons'] += [f'Missing task case coverage: {key}' for key in sorted(required_tasks - covered)]
        report['blocking_reasons'] += [f'Unaccepted evidence dimension: {name} ({dimensions[name]["status"]})' for name in sorted(required_dimensions) if dimensions[name]['status'] != 'passed']
        if digest(app) != report['candidate']['sha256']:
            raise ValueError('Candidate changed during execution')
        for name, before in inputs.items():
            if digest(ROOT / name) != before:
                raise ValueError(f'Fixture or verification source changed during execution: {name}')
    except (OSError, ValueError, KeyError, TypeError, RuntimeError, subprocess.SubprocessError) as error:
        report['blocking_reasons'].append(str(error))
    report['observed_capabilities'] = sorted(observed)
    report['covered_tasks'] = sorted(covered)
    report['inputs_sha256'] = inputs
    report['artifacts'] = {str(path.relative_to(output)): {'sha256': digest(path), 'bytes': path.stat().st_size} for path in sorted(run_dir.rglob('*')) if path.is_file()}
    report['result'] = 'failed' if any(case['status'] == 'failed' for case in report['cases'].values()) else ('blocked' if report['blocking_reasons'] else 'passed')
    temporary = output / 'report.tmp'
    temporary.write_text(json.dumps(report, indent=2) + '\n')
    temporary.replace(output / 'report.json')
    print(json.dumps({'result': report['result'], 'report': str(output / 'report.json'), 'blocking_reasons': report['blocking_reasons']}, indent=2))
    return 0 if report['result'] == 'passed' else (1 if report['result'] == 'failed' else 2)


if __name__ == '__main__':
    sys.exit(main())
