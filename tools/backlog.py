#!/usr/bin/env python3
"""Validate, publish, and pull Alloy's dependency-ordered GitHub issue backlog.

GitHub operations use gh's documented REST API interface, never browser control.
Publication is explicit and resumable using stable issue-body markers.
"""
import argparse
from collections import Counter
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
BACKLOG = ROOT / "docs/backlog/issues.json"
MANIFEST = ROOT / "docs/backlog/github.json"
MARKER = re.compile(r"<!-- alloy-backlog:([A-Za-z0-9_-]+) -->")
PHASE_NAMES = {
    "P0": "Foundation",
    "P1": "Geometry and documents",
    "P2": "Parametric modeling",
    "P3": "Shapes, exchange, and printing",
    "P4": "Initial release",
    "P5": "Assemblies and drawings",
    "P6": "Advanced modeling",
    "P7": "Manufacturing",
    "P8": "Analysis, electronics, and presentation",
}
KINDS = {"implementation", "research", "research/design", "epic", "validation"}


def load_backlog():
    document = json.loads(BACKLOG.read_text())
    if document.get("version") != 1 or not isinstance(document.get("issues"), list):
        raise ValueError("Expected version 1 with an issues array")
    issues = document["issues"]
    keys = [issue["key"] for issue in issues]
    if len(keys) != len(set(keys)):
        raise ValueError("Duplicate backlog keys")
    by_key = {issue["key"]: issue for issue in issues}
    for issue in issues:
        for field in ("key", "title", "kind", "phase", "priority", "body"):
            if not isinstance(issue.get(field), str) or not issue[field].strip():
                raise ValueError(f"{issue.get('key')}: missing/non-string {field}")
        if not re.fullmatch(r"[A-Za-z0-9_-]+", issue["key"]):
            raise ValueError(f"Invalid key: {issue['key']}")
        if issue["kind"] not in KINDS:
            raise ValueError(f"Unknown kind: {issue['kind']}")
        for field in ("depends_on", "labels"):
            if not isinstance(issue.get(field), list) or not all(isinstance(x, str) for x in issue[field]):
                raise ValueError(f"{issue['key']}: {field} must be a string array")
        if len(set(issue["depends_on"])) != len(issue["depends_on"]):
            raise ValueError(f"Duplicate dependency: {issue['key']}")
        for dep in issue["depends_on"]:
            if dep not in by_key:
                raise ValueError(f"Unknown dependency: {issue['key']} -> {dep}")
    ordered, visiting, visited = [], set(), set()

    def visit(key):
        if key in visiting:
            raise ValueError(f"Dependency cycle at {key}")
        if key in visited:
            return
        visiting.add(key)
        for dep in by_key[key]["depends_on"]:
            visit(dep)
        visiting.remove(key)
        visited.add(key)
        ordered.append(by_key[key])

    for key in keys:
        visit(key)
    return ordered


class GitHub:
    def __init__(self, repo):
        if not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", repo):
            raise ValueError("Repository must be owner/name")
        self.repo = repo
        self.last_write = 0

    def api(self, endpoint, method="GET", payload=None, paginate=False):
        if method != "GET":
            # Content creation is paced across labels, milestones, issues, and updates.
            time.sleep(max(0, 1.3 - (time.monotonic() - self.last_write)))
            self.last_write = time.monotonic()
        command = ["gh", "api", endpoint, "--method", method]
        if paginate:
            command += ["--paginate", "--slurp"]
        if payload is not None:
            command += ["--input", "-"]
        result = subprocess.run(command, input=json.dumps(payload) if payload is not None else None,
                                capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(f"GitHub {method} {endpoint}: {result.stderr.strip()}")
        value = json.loads(result.stdout) if result.stdout.strip() else None
        if paginate:
            return [item for page in value for item in page]
        return value

    def existing(self):
        records = self.api(f"repos/{self.repo}/issues?state=all&per_page=100", paginate=True)
        by_number = {record["number"]: record for record in records if "pull_request" not in record}
        published = json.loads(MANIFEST.read_text()).get("issues", {}) if MANIFEST.exists() else {}
        found = {}
        for key, entry in published.items():
            record = by_number.get(entry.get("number"))
            if record is not None:
                found[key] = record
        for record in records:
            if "pull_request" in record:
                continue
            match = MARKER.search(record.get("body") or "")
            if match:
                key = match.group(1)
                if key in found:
                    if found[key]["number"] != record["number"]:
                        # A canonical manifest entry wins over an orphaned duplicate.
                        # Never publish over the duplicate or silently switch IDs.
                        if key in published:
                            continue
                        raise ValueError(f"Duplicate published key {key}; reconcile before continuing")
                found[key] = record
        return found


def labels_for(issue):
    return sorted(set(issue["labels"] + [f"kind:{issue['kind']}", f"phase:{issue['phase'].split()[0]}",
                                       f"priority:{issue['priority']}"]))


def is_ready(issue, records):
    return all(records.get(dep, {}).get("state") == "closed" for dep in issue["depends_on"])


def body_for(issue, records):
    dependencies = "\n".join(f"- [{dep}]({records[dep]['html_url']})" for dep in issue["depends_on"])
    if not dependencies:
        dependencies = "None."
    return (f"<!-- alloy-backlog:{issue['key']} -->\n"
            f"**Backlog key:** `{issue['key']}` · **Phase:** `{issue['phase']}` · "
            f"**Priority:** `{issue['priority']}` · **Kind:** `{issue['kind']}`\n\n"
            f"{issue['body'].rstrip()}\n\n## Dependencies\n\n{dependencies}\n\n"
            "Pull this issue only after its dependencies are closed with appropriate evidence. "
            "A validation gate requires cumulative real-Alloy workflow evidence, not only a build or unit tests.\n")


def save_manifest(repo, records):
    MANIFEST.parent.mkdir(parents=True, exist_ok=True)
    content = {"repository": repo, "project_url": "https://github.com/users/rje/projects/1",
               "updated": datetime.now(timezone.utc).isoformat(),
               "issues": {key: {"number": value["number"], "url": value["html_url"],
                                "state": value["state"]} for key, value in records.items()}}
    temporary = MANIFEST.with_suffix(".tmp")
    temporary.write_text(json.dumps(content, indent=2) + "\n")
    temporary.replace(MANIFEST)


def save_index(issues, records):
    lines = ["# Published GitHub backlog", "", "Repository: https://github.com/rje/Alloy",
             "", "Project: https://github.com/users/rje/projects/1", "",
             "This is a publication snapshot. Use `python3 tools/backlog.py ready` for the live pull queue.",
             "", "| Issue | Phase | Kind | Priority | Depends on |", "| --- | --- | --- | --- | --- |"]
    for issue in issues:
        record = records[issue["key"]]
        title = issue["title"].replace("|", "\\|")
        dependencies = ", ".join(f"[#{records[key]['number']}]({records[key]['html_url']})" for key in issue["depends_on"]) or "—"
        lines.append(f"| [#{record['number']} {issue['key']}: {title}]({record['html_url']}) | {issue['phase']} | {issue['kind']} | {issue['priority']} | {dependencies} |")
    (MANIFEST.parent / "github.md").write_text("\n".join(lines) + "\n")


def publish(issues, github):
    records = github.existing()
    existing_labels = {row["name"] for row in github.api(f"repos/{github.repo}/labels?per_page=100", paginate=True)}
    needed = {"status:ready", "status:blocked"}
    for issue in issues:
        needed.update(labels_for(issue))
    for label in sorted(needed - existing_labels):
        color = {"status:ready": "0e8a16", "status:blocked": "b60205", "kind:validation": "5319e7"}.get(label, hashlib.sha256(label.encode()).hexdigest()[:6])
        github.api(f"repos/{github.repo}/labels", "POST", {"name": label, "color": color})
    milestones = {row["title"]: row for row in github.api(f"repos/{github.repo}/milestones?state=all&per_page=100", paginate=True)}
    phase_milestones = {}
    for phase in sorted({issue["phase"] for issue in issues}):
        title = phase if " " in phase else f"{phase} — {PHASE_NAMES.get(phase, 'Full replacement')}"
        if title not in milestones:
            milestones[title] = github.api(f"repos/{github.repo}/milestones", "POST", {
                "title": title, "description": "Workflow phase; cumulative validation gates require real-application evidence. No deadline is committed."})
        phase_milestones[phase] = milestones[title]["number"]
    for index, issue in enumerate(issues, 1):
        key = issue["key"]
        title = f"[{key}] {issue['title']}"
        body = body_for(issue, records)
        labels = labels_for(issue)
        if key not in records:
            labels.append("status:ready" if is_ready(issue, records) else "status:blocked")
            records[key] = github.api(f"repos/{github.repo}/issues", "POST", {
                "title": title, "body": body, "labels": labels, "milestone": phase_milestones[issue["phase"]]})
            verb = "created"
        else:
            record = records[key]
            current_labels = {label["name"] for label in record["labels"]}
            milestone = record.get("milestone")
            if (record["title"] != title or record.get("body") != body or not set(labels).issubset(current_labels)
                    or not milestone or milestone["number"] != phase_milestones[issue["phase"]]):
                records[key] = github.api(f"repos/{github.repo}/issues/{record['number']}", "PATCH", {
                    "title": title, "body": body, "labels": sorted(current_labels | set(labels)),
                    "milestone": phase_milestones[issue["phase"]]})
                verb = "updated"
            else:
                verb = "existing"
        save_manifest(github.repo, records)
        print(f"{index}/{len(issues)} {verb} {key} #{records[key]['number']}", flush=True)
    save_index(issues, records)
    print(f"Published {len(issues)} backlog issues to https://github.com/{github.repo}/issues", flush=True)


def number_rank(value):
    match = re.search(r"\d+", value)
    return int(match.group()) if match else 99


def ready(issues, github, apply=False):
    records = github.existing()
    missing = [issue["key"] for issue in issues if issue["key"] not in records]
    if missing:
        raise ValueError(f"Unpublished issues: {', '.join(missing)}")
    pullable = []
    for issue in issues:
        record = records[issue["key"]]
        if record["state"] == "closed":
            desired = set()
        else:
            available = is_ready(issue, records)
            desired = {"status:ready" if available else "status:blocked"}
            if available:
                pullable.append(issue)
        old = {label["name"] for label in record["labels"]}
        new = (old - {"status:ready", "status:blocked"}) | desired
        if apply and old != new:
            records[issue["key"]] = github.api(f"repos/{github.repo}/issues/{record['number']}", "PATCH", {"labels": sorted(new)})
    if apply:
        save_manifest(github.repo, records)
    sequence = json.loads((ROOT / "docs/backlog/tutorial-sequence.json").read_text())
    ranked = [key for checkpoint in sequence["checkpoints"] for key in checkpoint["issues"]]
    # A current priority may reference a task also listed in an older checkpoint.
    # Preserve its first (highest-priority) occurrence.
    ranks = {}
    for index, key in enumerate(ranked):
        ranks.setdefault(key, index)
    for issue in sorted(pullable, key=lambda row: (ranks.get(row["key"], len(ranks)), number_rank(row["priority"]), number_rank(row["phase"]), row["key"])):
        record = records[issue["key"]]
        print(f"#{record['number']} {issue['key']} [{issue['priority']} / {issue['phase']} / {issue['kind']}] {issue['title']}\n  {record['html_url']}")
    print(f"Ready: {len(pullable)}; closed: {sum(records[issue['key']]['state'] == 'closed' for issue in issues)}; total: {len(issues)}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", default="rje/Alloy")
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("validate", help="Validate local schema, keys, dependencies, and acyclic ordering")
    commands.add_parser("publish", help="Explicitly create/update GitHub labels, milestones, and owned issues")
    pull = commands.add_parser("ready", help="Read live GitHub state and list issues with all dependencies closed")
    pull.add_argument("--apply-labels", action="store_true", help="Update ready/blocked labels to match live dependency state")
    options = parser.parse_args()
    issues = load_backlog()
    if options.command == "validate":
        print(f"Valid: {len(issues)} issues; kinds={dict(Counter(row['kind'] for row in issues))}; acyclic dependencies")
    elif options.command == "publish":
        publish(issues, GitHub(options.repo))
    else:
        ready(issues, GitHub(options.repo), options.apply_labels)


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, RuntimeError, KeyError) as error:
        print(f"backlog: {error}", file=sys.stderr)
        sys.exit(1)
