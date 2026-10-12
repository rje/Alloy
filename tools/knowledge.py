#!/usr/bin/env python3
"""Validate and search the external, portable workflow knowledge corpus."""

from __future__ import annotations

import argparse
import hashlib
import json
import sqlite3
import sys
from pathlib import Path
from typing import Any


REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_ROOT = REPO_ROOT.parent / "Alloy-reference-records" / "knowledge"
REQUIRED = {
    "schema_version", "id", "topic", "title", "status", "summary",
    "evidence", "workflow", "edge_cases", "dependencies", "uncertainties",
    "verification", "alloy_gaps", "proposal", "visual_refs",
}
WORKFLOW_FIELDS = {
    "preconditions", "inputs", "defaults", "selection", "keys_pointer",
    "preview", "commit_cancel", "undo_edit_storage", "visual_rules",
}
EVIDENCE_TYPES = {"documented", "observed-video", "observed-screenshot", "inferred", "proposal"}


def read_records(root: Path) -> list[dict[str, Any]]:
    record_dir = root / "records"
    if not record_dir.is_dir():
        raise ValueError(f"record directory does not exist: {record_dir}")
    records = []
    for path in sorted(record_dir.glob("*.json")):
        try:
            item = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as exc:
            raise ValueError(f"{path}: {exc}") from exc
        if not isinstance(item, dict):
            raise ValueError(f"{path}: record must be a JSON object")
        records.append(item)
    if not records:
        raise ValueError(f"no JSON records found in {record_dir}")
    return records


def validate(records: list[dict[str, Any]], root: Path | None = None) -> list[str]:
    errors: list[str] = []
    source_ids: set[str] | None = None
    visual_items: dict[str, dict[str, Any]] = {}
    if root is not None:
        source_path = root / "sources.json"
        if not source_path.is_file():
            errors.append(f"required source registry is missing: {source_path}")
        else:
            try:
                source_data = json.loads(source_path.read_text(encoding="utf-8"))
                sources = source_data.get("sources")
                if not isinstance(sources, dict):
                    errors.append(f"{source_path}: sources must be an object")
                else:
                    source_ids = set(sources)
            except (OSError, json.JSONDecodeError, AttributeError) as exc:
                errors.append(f"{source_path}: invalid source registry: {exc}")
        manifest_path = root / "visuals" / "manifest.json"
        records_have_visuals = any(record.get("visual_refs") for record in records)
        if records_have_visuals and not manifest_path.is_file():
            errors.append(f"required visual manifest is missing: {manifest_path}")
        elif manifest_path.is_file():
            try:
                manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
                visual_items = {item["id"]: item for item in manifest.get("items", [])}
            except (OSError, json.JSONDecodeError, KeyError, AttributeError) as exc:
                errors.append(f"{manifest_path}: invalid visual manifest: {exc}")
    ids: set[str] = set()
    for record in records:
        label = record.get("id", "<missing id>")
        missing = REQUIRED - record.keys()
        if missing:
            errors.append(f"{label}: missing fields: {', '.join(sorted(missing))}")
        if record.get("schema_version") != 1:
            errors.append(f"{label}: schema_version must be 1")
        rid = record.get("id")
        if not isinstance(rid, str) or not rid.strip():
            errors.append(f"{label}: id must be a non-empty string")
        elif rid in ids:
            errors.append(f"{rid}: duplicate id")
        else:
            ids.add(rid)
        for field in ("topic", "title", "status", "summary"):
            if not isinstance(record.get(field), str) or not record[field].strip():
                errors.append(f"{label}: {field} must be a non-empty string")
        workflow = record.get("workflow")
        if not isinstance(workflow, dict):
            errors.append(f"{label}: workflow must be an object")
        else:
            missing_workflow = WORKFLOW_FIELDS - workflow.keys()
            if missing_workflow:
                errors.append(f"{label}: workflow missing: {', '.join(sorted(missing_workflow))}")
            for field, value in workflow.items():
                if not isinstance(value, list) or not all(isinstance(entry, str) for entry in value):
                    errors.append(f"{label}: workflow.{field} must be an array of strings")
        evidence = record.get("evidence")
        if not isinstance(evidence, list) or not evidence:
            errors.append(f"{label}: evidence must be a non-empty array")
        else:
            for index, fact in enumerate(evidence):
                prefix = f"{label}: evidence[{index}]"
                if not isinstance(fact, dict):
                    errors.append(f"{prefix} must be an object")
                    continue
                for field in ("type", "source_id", "location", "retrieved", "fact"):
                    if not isinstance(fact.get(field), str) or not fact[field].strip():
                        errors.append(f"{prefix}.{field} must be a non-empty string")
                if fact.get("type") not in EVIDENCE_TYPES:
                    errors.append(f"{prefix}.type must be one of: {', '.join(sorted(EVIDENCE_TYPES))}")
                if source_ids is not None and fact.get("source_id") not in source_ids:
                    errors.append(f"{prefix}: source_id is missing from sources.json: {fact.get('source_id')}")
        for field in ("edge_cases", "dependencies", "uncertainties", "verification", "alloy_gaps", "proposal"):
            value = record.get(field)
            if not isinstance(value, list):
                errors.append(f"{label}: {field} must be an array")
        visual_refs = record.get("visual_refs")
        if not isinstance(visual_refs, list) or not visual_refs or not all(isinstance(item, str) and item for item in visual_refs):
            errors.append(f"{label}: visual_refs must be a non-empty array of IDs")
        if root is not None:
            for visual_id in visual_refs if isinstance(visual_refs, list) else []:
                visual = visual_items.get(visual_id)
                if visual is None:
                    errors.append(f"{label}: visual reference missing from visuals/manifest.json: {visual_id}")
                    continue
                visual_source_id = visual.get("source_id")
                if source_ids is not None and visual_source_id not in source_ids:
                    errors.append(f"{label}: visual source_id is missing from sources.json: {visual_source_id}")
                visual_path = root / "visuals" / visual.get("file", "")
                if not visual_path.is_file():
                    errors.append(f"{label}: visual file is missing: {visual_path}")
                elif hashlib.sha256(visual_path.read_bytes()).hexdigest() != visual.get("sha256"):
                    errors.append(f"{label}: visual checksum mismatch: {visual_path}")
    return errors


def build(root: Path, db_path: Path) -> int:
    records = read_records(root)
    errors = validate(records, root)
    if errors:
        raise ValueError("\n".join(errors))
    db_path.parent.mkdir(parents=True, exist_ok=True)
    temporary = db_path.with_suffix(db_path.suffix + ".tmp")
    if temporary.exists():
        temporary.unlink()
    with sqlite3.connect(temporary) as db:
        db.execute("PRAGMA journal_mode=DELETE")
        db.execute("CREATE TABLE records (id TEXT PRIMARY KEY, topic TEXT NOT NULL, title TEXT NOT NULL, status TEXT NOT NULL, summary TEXT NOT NULL, json TEXT NOT NULL)")
        db.execute("CREATE VIRTUAL TABLE records_fts USING fts5(id UNINDEXED, topic, title, body, tokenize='unicode61')")
        for item in records:
            payload = json.dumps(item, ensure_ascii=False, sort_keys=True)
            body = " ".join((item["summary"], json.dumps(item["evidence"], ensure_ascii=False), json.dumps(item["workflow"], ensure_ascii=False), json.dumps(item["edge_cases"], ensure_ascii=False), json.dumps(item["dependencies"], ensure_ascii=False), json.dumps(item["uncertainties"], ensure_ascii=False), json.dumps(item["verification"], ensure_ascii=False), json.dumps(item["proposal"], ensure_ascii=False)))
            db.execute("INSERT INTO records VALUES (?, ?, ?, ?, ?, ?)", (item["id"], item["topic"], item["title"], item["status"], item["summary"], payload))
            db.execute("INSERT INTO records_fts VALUES (?, ?, ?, ?)", (item["id"], item["topic"], item["title"], body))
        db.execute("CREATE INDEX records_topic_idx ON records(topic)")
        db.commit()
    temporary.replace(db_path)
    print(f"built {db_path} from {len(records)} records")
    return len(records)


def search(db_path: Path, query: str, limit: int) -> int:
    if not db_path.is_file():
        raise ValueError(f"database not found; run build first: {db_path}")
    with sqlite3.connect(db_path) as db:
        db.row_factory = sqlite3.Row
        rows = db.execute("SELECT r.id, r.topic, r.title, r.status, snippet(records_fts, 3, '[', ']', ' … ', 18) AS excerpt FROM records_fts JOIN records r ON r.id = records_fts.id WHERE records_fts MATCH ? ORDER BY bm25(records_fts), r.id LIMIT ?", (query, limit)).fetchall()
    for row in rows:
        print(f"{row['id']}\t{row['topic']}\t{row['title']} [{row['status']}]\n  {row['excerpt']}")
    if not rows:
        print("no matching records")
    return len(rows)


def show(db_path: Path, record_id: str) -> None:
    if not db_path.is_file():
        raise ValueError(f"database not found; run build first: {db_path}")
    with sqlite3.connect(db_path) as db:
        row = db.execute("SELECT json FROM records WHERE id = ?", (record_id,)).fetchone()
    if row is None:
        raise ValueError(f"record not found: {record_id}")
    print(json.dumps(json.loads(row[0]), indent=2, ensure_ascii=False))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=DEFAULT_ROOT, help="external knowledge directory (default: sibling reference archive)")
    parser.add_argument("--db", type=Path, help="SQLite output/input path (default: <root>/knowledge.sqlite)")
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("validate", help="validate portable JSON records")
    sub.add_parser("build", help="rebuild the derived SQLite FTS index")
    query_parser = sub.add_parser("search", help="search indexed workflow records")
    query_parser.add_argument("query", help="FTS5 query, for example: 'lost support' or 'point edge distance'")
    query_parser.add_argument("--limit", type=int, default=8)
    show_parser = sub.add_parser("show", help="print one full record by stable ID")
    show_parser.add_argument("id")
    args = parser.parse_args()
    db_path = args.db or (args.root / "knowledge.sqlite")
    try:
        if args.command == "validate":
            records = read_records(args.root)
            errors = validate(records, args.root)
            if errors:
                print("\n".join(errors), file=sys.stderr)
                return 1
            print(f"valid: {len(records)} records")
        elif args.command == "build":
            build(args.root, db_path)
        elif args.command == "search":
            if args.limit < 1:
                raise ValueError("--limit must be positive")
            search(db_path, args.query, args.limit)
        elif args.command == "show":
            show(db_path, args.id)
        return 0
    except (OSError, sqlite3.Error, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
