#!/usr/bin/env python3
"""Capture synthetic legacy email/report baselines without importing plugins.

The report plugins import Qt WebEngine at module import time.  This tool compiles
only their pure top-level builder functions from the checked-out source using
``ast``; it never imports a plugin, creates a QApplication, calls a menu action,
or accesses settings, collaboration, files outside --output, or the network.
"""

from __future__ import annotations

import argparse
import ast
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[4]
FIXTURE_DIR = Path(__file__).resolve().parent


def load_functions(relative_path: str, names: set[str], namespace: dict) -> None:
    """Compile selected pure function definitions from a legacy plugin."""
    tree = ast.parse((ROOT / relative_path).read_text(encoding="utf-8"), relative_path)
    selected = [node for node in tree.body if isinstance(node, ast.FunctionDef) and node.name in names]
    missing = names - {node.name for node in selected}
    if missing:
        raise RuntimeError(f"{relative_path}: missing {', '.join(sorted(missing))}")
    module = ast.fix_missing_locations(ast.Module(body=selected, type_ignores=[]))
    exec(compile(module, relative_path, "exec"), namespace)


def load_assignments(relative_path: str, names: set[str], namespace: dict) -> None:
    """Compile selected literal/module-level constants required by a builder."""
    tree = ast.parse((ROOT / relative_path).read_text(encoding="utf-8"), relative_path)
    selected = []
    for node in tree.body:
        if not isinstance(node, ast.Assign):
            continue
        targets = {target.id for target in node.targets if isinstance(target, ast.Name)}
        if targets & names:
            selected.append(node)
    missing = names - {target.id for node in selected for target in node.targets if isinstance(target, ast.Name)}
    if missing:
        raise RuntimeError(f"{relative_path}: missing constants {', '.join(sorted(missing))}")
    exec(compile(ast.fix_missing_locations(ast.Module(body=selected, type_ignores=[])), relative_path, "exec"), namespace)


def load_note_methods(namespace: dict) -> None:
    """Load NoteFormatter methods only; the caller supplies synthetic values."""
    relative_path = "plugins/includes/noteformatter.py"
    tree = ast.parse((ROOT / relative_path).read_text(encoding="utf-8"), relative_path)
    klass = next(
        (node for node in tree.body if isinstance(node, ast.ClassDef) and node.name == "NoteFormatter"),
        None,
    )
    if klass is None:
        raise RuntimeError(f"{relative_path}: NoteFormatter missing")
    methods = [
        node
        for node in klass.body
        if isinstance(node, ast.FunctionDef)
        and node.name
        in {"get_html_header", "get_html_attendee", "get_html_notes", "get_html_trackerheader",
            "get_html_trackerrow", "get_html_footer"}
    ]
    wrapper = ast.ClassDef(name="LegacyNoteLayout", bases=[], keywords=[], decorator_list=[], body=methods)
    module = ast.fix_missing_locations(ast.Module(body=[wrapper], type_ignores=[]))
    exec(compile(module, relative_path, "exec"), namespace)


class EscapeOnlyCommon:
    @staticmethod
    def to_html(value: object) -> str:
        return ("" if value is None else str(value)).replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;").replace('"', "&quot;")


def digest(text: str) -> str:
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path, required=True, help="empty/new directory for generated baselines")
    args = parser.parse_args()
    data = json.loads((FIXTURE_DIR / "synthetic-project.json").read_text(encoding="utf-8"))
    args.output.mkdir(parents=True, exist_ok=True)

    ns: dict = {"re": __import__("re"), "ProjectNotesCommon": EscapeOnlyCommon}
    load_functions("plugins/exporttrackeritems_plugin.py", {"_html_escape", "build_group_status_header", "build_item_row", "generate_tracker_html"}, ns)
    load_functions("plugins/exportstatusreport_plugin.py", {"_html_escape", "_parse_number", "compute_earned_value_metrics", "build_activity_block", "build_issue_row", "build_earned_value_html", "generate_status_report_html"}, ns)
    load_assignments("plugins/exportstatusreport_plugin.py", {"EAC_FORMULA_HTML", "APPENDIX_HTML"}, ns)
    load_functions("plugins/exportnotes_plugin.py", {"_html_escape", "build_action_item_row", "build_meeting_block", "generate_notes_html"}, ns)
    load_note_methods(ns)

    project = data["project"]
    report_date = data["reporting_date"]
    rows = []
    for item in data["tracker_items"]:
        rows.append(ns["build_item_row"](item["number"], item["name"], item["identified_by"], item["date_identified"], item["description"], item["assigned_to"], item["priority"], item["status"], item["date_due"], item["last_update"], item["date_resolved"], item["comments"], item["internal"], item["type"], True))
    tracker_html = ns["generate_tracker_html"](project["number"], project["name"], ns["build_group_status_header"]("New", len(rows), 13) + "".join(rows), report_date, True)

    metrics = ns["compute_earned_value_metrics"]("$100,000.00", "$25,000.00", "$30,000.00", "$35,000.00", "$100,000.00")
    status_html = ns["generate_status_report_html"](
        project["number"], project["name"], data["manager"], ", ".join(data["status_recipients"]), report_date,
        "08/15/2026 to 09/15/2026", ns["build_activity_block"]("Since Last Report", data["activities"]["since_last"]),
        ns["build_activity_block"]("Next Period", data["activities"]["next_period"]), ns["build_activity_block"]("Completed", data["activities"]["completed"]),
        ns["build_issue_row"]("Synthetic risk & mitigation", "Alex Partner", "High", "09/30/2026", "Assigned"),
        ns["build_earned_value_html"]("Monthly", "08/15/2026", "09/15/2026", metrics),
    )

    action_rows = "".join(ns["build_action_item_row"](item["name"], item["assigned_to"], item["status"], item["date_due"]) for item in data["meeting"]["actions"])
    meeting_block = ns["build_meeting_block"](data["meeting"]["title"], data["meeting"]["date"], ", ".join(data["meeting"]["attendees"]), data["meeting"]["notes"], action_rows)
    notes_html = ns["generate_notes_html"](project["number"], project["name"], meeting_block, report_date)

    note = ns["LegacyNoteLayout"]()
    note.pnc = EscapeOnlyCommon()
    note_email_html = "".join([
        note.get_html_header(project["number"], project["name"], data["meeting"]["date"], data["meeting"]["title"]),
        note.get_html_attendee(", ".join(data["meeting"]["attendees"])), note.get_html_notes(data["meeting"]["notes"]),
        note.get_html_trackerheader(),
        *[note.get_html_trackerrow(item["name"], item["assigned_to"], item["status"], item["date_due"]) for item in data["meeting"]["actions"]],
        note.get_html_footer(),
    ])
    outputs = {
        "send-meeting-notes.html": note_email_html,
        "tracker-items-report.html": tracker_html,
        "status-report.html": status_html,
        "meeting-notes-report.html": notes_html,
    }
    for name, content in outputs.items():
        (args.output / name).write_text(content, encoding="utf-8")
    result = {
        "source_revision": data["source_revision"],
        "subjects": {
            "send_meeting_notes": f'{project["number"]} {project["name"]} - {data["meeting"]["date"]} {data["meeting"]["title"]} Notes',
            "tracker_items_report": f'{project["number"]} {project["name"]} - Tracker Items {report_date}',
            "status_report": f'{project["number"]} {project["name"]} - Status Report {report_date}',
            "meeting_notes_report": f'{project["number"]} {project["name"]} - {report_date}',
        },
        "sha256": {name: digest(content) for name, content in outputs.items()},
        "recipients": {"send_meeting_notes": "Full Project Team", "status_report": "Receives Status", "tracker_items_report": "context-derived", "meeting_notes_report": "context-derived"},
    }
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
