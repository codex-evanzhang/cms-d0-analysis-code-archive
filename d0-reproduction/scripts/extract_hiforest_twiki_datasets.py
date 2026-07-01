#!/usr/bin/env python3
"""Extract compact dataset locations from the CMS HiForest PbPb TWiki."""

from __future__ import annotations

import argparse
import datetime as dt
import html
import json
import re
import urllib.request
from html.parser import HTMLParser
from pathlib import Path
from typing import Any

import yaml


DEFAULT_URL = "https://twiki.cern.ch/twiki/bin/view/CMS/HiForest2025PbPbHF"
ROOT = Path(__file__).resolve().parents[3]
OUT_DIR = ROOT / "research" / "cms-d0-analysis" / "output"
OUT_YAML = OUT_DIR / "hiforest2025pbpbhf_datasets.yaml"
OUT_MD = OUT_DIR / "hiforest2025pbpbhf_datasets.md"


def utc_now() -> str:
    return dt.datetime.now(dt.UTC).strftime("%Y-%m-%dT%H:%M:%SZ")


def clean(text: str) -> str:
    text = html.unescape(text).replace("\xa0", " ")
    return re.sub(r"\s+", " ", text).strip()


class TwikiParser(HTMLParser):
    def __init__(self) -> None:
        super().__init__(convert_charrefs=True)
        self.current_h2 = ""
        self.current_h3 = ""
        self._heading_tag = ""
        self._heading_buf: list[str] = []
        self._li_buf: list[str] | None = None
        self._table_rows: list[list[str]] | None = None
        self._row: list[str] | None = None
        self._cell_buf: list[str] | None = None
        self.tables: list[dict[str, Any]] = []
        self.list_items: list[dict[str, str]] = []
        self.topic_revision = ""

    @property
    def current_section(self) -> str:
        return self.current_h3 or self.current_h2

    def handle_starttag(self, tag: str, attrs: list[tuple[str, str | None]]) -> None:
        if tag in {"h1", "h2", "h3"}:
            self._heading_tag = tag
            self._heading_buf = []
        elif tag == "li":
            self._li_buf = []
        elif tag == "table":
            self._table_rows = []
        elif tag == "tr" and self._table_rows is not None:
            self._row = []
        elif tag in {"td", "th"} and self._row is not None:
            self._cell_buf = []
        elif tag == "img" and self._cell_buf is not None:
            attr = dict(attrs)
            alt = attr.get("alt") or attr.get("title") or ""
            if alt:
                self._cell_buf.append(alt)

    def handle_endtag(self, tag: str) -> None:
        if self._heading_tag and tag == self._heading_tag:
            title = clean(" ".join(self._heading_buf))
            if tag == "h2":
                self.current_h2 = title
                self.current_h3 = ""
            elif tag == "h3":
                self.current_h3 = title
            self._heading_tag = ""
            self._heading_buf = []
        elif tag == "li" and self._li_buf is not None:
            item = clean(" ".join(self._li_buf))
            if item:
                self.list_items.append(
                    {"h2": self.current_h2, "h3": self.current_h3, "text": item}
                )
            self._li_buf = None
        elif tag in {"td", "th"} and self._cell_buf is not None and self._row is not None:
            self._row.append(clean(" ".join(self._cell_buf)))
            self._cell_buf = None
        elif tag == "tr" and self._row is not None and self._table_rows is not None:
            if any(cell for cell in self._row):
                self._table_rows.append(self._row)
            self._row = None
        elif tag == "table" and self._table_rows is not None:
            if self._table_rows:
                self.tables.append(
                    {
                        "h2": self.current_h2,
                        "h3": self.current_h3,
                        "rows": self._table_rows,
                    }
                )
            self._table_rows = None

    def handle_data(self, data: str) -> None:
        if self._heading_tag:
            self._heading_buf.append(data)
        if self._li_buf is not None:
            self._li_buf.append(data)
        if self._cell_buf is not None:
            self._cell_buf.append(data)
        if "Topic revision:" in data:
            self.topic_revision = clean(data)


def fetch(url: str) -> str:
    request = urllib.request.Request(url, headers={"User-Agent": "codex-cms-dataset-extractor/1.0"})
    with urllib.request.urlopen(request, timeout=60) as response:
        payload = response.read()
    # TWiki declares iso-8859-1 for this page.
    return payload.decode("iso-8859-1", errors="replace")


def is_relevant_context(h2: str, section: str) -> bool:
    if h2.startswith("Obsolete"):
        return False
    targets = [
        "2023 PbPb UPC, Jan 2024 ReReco",
        "2023 PbPb UPC, Feb 2025 ReReco",
        "2024 Official MC",
    ]
    return any(target in section for target in targets)


def rows_to_records(table: dict[str, Any]) -> list[dict[str, str]]:
    rows = table["rows"]
    if not rows:
        return []
    header = rows[0]
    records = []
    for row in rows[1:]:
        if len(row) < len(header):
            row = [*row, *([""] * (len(header) - len(row)))]
        record = {header[idx]: row[idx] for idx in range(len(header))}
        if any(record.values()):
            records.append(record)
    return records


def current_records(records: list[dict[str, str]]) -> list[dict[str, str]]:
    return [row for row in records if "OUTDATED" not in row.get("Status", "").upper()]


def extract(html_text: str, url: str) -> dict[str, Any]:
    parser = TwikiParser()
    parser.feed(html_text)

    relevant_tables = []
    for table in parser.tables:
        section = table["h3"] or table["h2"]
        records = current_records(rows_to_records(table))
        if is_relevant_context(table["h2"], section):
            relevant_tables.append(
                {
                    "section": section,
                    "h2": table["h2"],
                    "records": records,
                }
            )

    overview_rows = []
    for table in parser.tables:
        records = rows_to_records(table)
        for row in records:
            sample = row.get("Sample", "")
            if sample.startswith("2023 data") or sample.startswith("2024 MC"):
                overview_rows.append(row)

    metadata_items = [
        item
        for item in parser.list_items
        if is_relevant_context(item["h2"], item["h3"]) and (
            "Golden JSON" in item["text"]
            or "CMSSW" in item["text"]
            or "Forest" in item["text"]
            or "Dfinder" in item["text"]
            or "Summary of event filters" in item["text"]
            or "Summary of D0 selections" in item["text"]
            or "Particle flow settings" in item["text"]
        )
    ]

    return {
        "source_url": url,
        "extracted_utc": utc_now(),
        "topic_revision": parser.topic_revision,
        "overview_records": overview_rows,
        "section_metadata": metadata_items,
        "tables": relevant_tables,
    }


def write_markdown(data: dict[str, Any], path: Path) -> None:
    lines = [
        "# HiForest2025PbPbHF Dataset Inventory",
        "",
        "<!-- DOC_OWNER: generated by extract_hiforest_twiki_datasets.py. -->",
        "<!-- TOKEN_NOTE: compact TWiki extraction; rerun script instead of pasting the full TWiki. -->",
        "",
        f"- Source: {data['source_url']}",
        f"- Extracted UTC: `{data['extracted_utc']}`",
    ]
    if data.get("topic_revision"):
        lines.append(f"- TWiki revision: `{data['topic_revision']}`")
    lines.append("")

    lines.extend(["## Overview Records", ""])
    for row in data["overview_records"]:
        sample = row.get("Sample", "sample")
        lines.append(f"### {sample}")
        for key in ["Skims", "Forested Files", "MINIAOD", "Event filter", "HF leading tower branch", "HF cut in forest"]:
            value = row.get(key, "")
            if value:
                lines.append(f"- {key}: `{value}`")
        lines.append("")

    lines.extend(["## Detailed Sections", ""])
    for table in data["tables"]:
        lines.append(f"### {table['section']}")
        records = table["records"]
        if not records:
            lines.append("- No records parsed.")
            lines.append("")
            continue
        for row in records:
            pd = row.get("PD") or row.get("Sample") or "row"
            status = row.get("Status", "")
            run_range = row.get("Run range", "")
            lines.append(f"- `{pd}`")
            if run_range:
                lines.append(f"  - run range: `{run_range}`")
            if status:
                lines.append(f"  - status: `{status}`")
            for key in ["MINIAOD", "Forested Files", "Skims", "Note"]:
                value = row.get(key, "")
                if value:
                    lines.append(f"  - {key}: `{value}`")
        lines.append("")

    path.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--url", default=DEFAULT_URL)
    parser.add_argument("--yaml", default=str(OUT_YAML))
    parser.add_argument("--markdown", default=str(OUT_MD))
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()

    data = extract(fetch(args.url), args.url)
    yaml_path = Path(args.yaml)
    md_path = Path(args.markdown)
    yaml_path.parent.mkdir(parents=True, exist_ok=True)
    md_path.parent.mkdir(parents=True, exist_ok=True)
    yaml_path.write_text(yaml.safe_dump(data, sort_keys=False, width=120), encoding="utf-8")
    write_markdown(data, md_path)
    if args.json:
        print(json.dumps(data, indent=2, sort_keys=True))
    else:
        print(f"Wrote {yaml_path}")
        print(f"Wrote {md_path}")
        print(f"overview_records: {len(data['overview_records'])}")
        print(f"tables: {len(data['tables'])}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
