#!/usr/bin/env python3
"""Builds the generated copies of the known-CAN-ID table.

Master file:  android/app/src/main/assets/can_ids.json   (the app reads it directly)
Generated:    firmware/obduke/web_ui.h   (block between the KIDS markers, for the web page sniffer)
              docs/can-ids.md            (block between the IDS markers, human-readable tables)

  python3 tools/gen_can_ids.py          rewrite the generated blocks
  python3 tools/gen_can_ids.py --check  exit 1 if they are out of date (CI uses this)
"""
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
MASTER = ROOT / "android/app/src/main/assets/can_ids.json"
WEB = ROOT / "firmware/obduke/web_ui.h"
DOC = ROOT / "docs/can-ids.md"

WEB_BEGIN, WEB_END = "/*KIDS-BEGIN*/", "/*KIDS-END*/"
DOC_BEGIN, DOC_END = "<!-- IDS-BEGIN -->", "<!-- IDS-END -->"

FIELD_KEYS = {"name", "b", "n", "shift", "bits", "signed", "le", "mul", "add", "dp", "unit", "min", "max",
              "map", "yes", "no", "bikes", "note", "text"}
ENTRY_KEYS = {"id", "to", "name", "ms", "from", "try", "note", "src", "bikes", "fields"}


def hex_id(s):
    v = int(s, 16)
    assert 0 <= v <= 0x7FF, f"{s}: not an 11-bit ID"
    return v


def validate(d):
    errs = []
    status = set(d["status"])
    bikes = set(d["bikes"])
    srcs = set(d["sources"])
    seen = []
    for e in d["ids"]:
        tag = f"ID {e.get('id')}"
        for k in e:
            if k not in ENTRY_KEYS:
                errs.append(f"{tag}: unknown key {k}")
        try:
            lo = hex_id(e["id"])
            hi = hex_id(e["to"]) if "to" in e else lo
            if hi < lo:
                errs.append(f"{tag}: 'to' is below 'id'")
            for a, b in seen:
                if not (hi < a or lo > b):
                    errs.append(f"{tag}: overlaps another entry")
            seen.append((lo, hi))
        except Exception as ex:
            errs.append(f"{tag}: {ex}")
        for s in e.get("src", []):
            if s not in srcs:
                errs.append(f"{tag}: unknown source {s}")
        for b, st in e.get("bikes", {}).items():
            if b not in bikes or st not in status:
                errs.append(f"{tag}: bad bike/status {b}/{st}")
        for i, f in enumerate(e.get("fields", [])):
            ft = f"{tag} field {i} ({f.get('name', f.get('text', '?'))})"
            for k in f:
                if k not in FIELD_KEYS:
                    errs.append(f"{ft}: unknown key {k}")
            if "text" in f:
                continue
            if "name" not in f or "b" not in f:
                errs.append(f"{ft}: needs name and b")
                continue
            n = f.get("n", 1)
            bits = f.get("bits", 8 * n)
            if f["b"] + n > 8:
                errs.append(f"{ft}: runs past byte 7")
            if f.get("shift", 0) + bits > 8 * n:
                errs.append(f"{ft}: shift+bits wider than the {n} byte(s)")
            if n > 4:
                errs.append(f"{ft}: more than 4 bytes")
            if not f.get("bikes"):
                errs.append(f"{ft}: needs bikes")
            for b, st in f.get("bikes", {}).items():
                if b not in bikes or st not in status:
                    errs.append(f"{ft}: bad bike/status {b}/{st}")
            if "yes" in f and bits != 1:
                errs.append(f"{ft}: yes/no only for 1-bit fields")
            if ("min" in f) != ("max" in f):
                errs.append(f"{ft}: give both min and max, or neither")
            whole_bytes = not f.get("map") and "yes" not in f and not f.get("shift") and bits == 8 * n
            if whole_bytes and len(f["name"]) > 28:
                errs.append(f"{ft}: name over 28 chars (it becomes a Custom signals name)")
    return errs


# ---------- web page block ----------
def web_json(d):
    out = {
        "bikes": d["bikes"],
        "ids": [{k: v for k, v in e.items() if k != "src"} for e in d["ids"]],
    }
    return json.dumps(out, separators=(",", ":"), ensure_ascii=True)


def build_web(text, d):
    block = f"{WEB_BEGIN}const KIDS={web_json(d)};{WEB_END}"
    if WEB_BEGIN not in text:
        raise SystemExit(f"{WEB}: markers {WEB_BEGIN} ... {WEB_END} not found")
    return re.sub(re.escape(WEB_BEGIN) + ".*?" + re.escape(WEB_END), lambda m: block, text, flags=re.S)


# ---------- docs block ----------
def byte_range(f):
    n = f.get("n", 1)
    b = f["b"]
    return f"D{b}" if n == 1 else f"D{b}-D{b + n - 1}"


def how(f):
    n = f.get("n", 1)
    bits = f.get("bits", 8 * n)
    shift = f.get("shift", 0)
    parts = []
    if bits == 1:
        parts.append(f"bit {shift} (0x{1 << shift:02X})")
    elif bits != 8 * n or shift:
        parts.append(f"{bits} bits" + (f", shifted right {shift}" if shift else ""))
    if n > 1:
        parts.append("little-endian" if f.get("le") else "big-endian")
    if f.get("signed"):
        parts.append("signed")
    if "map" in f:
        parts.append("values: " + ", ".join(f"{k}={v}" for k, v in f["map"].items()))
    elif "yes" in f:
        parts.append(f"1={f['yes']}, 0={f['no']}")
    if "mul" in f or "add" in f:
        parts.append(f"x {f.get('mul', 1)}" + (f" + {f['add']}" if f.get("add") else ""))
    if f.get("unit"):
        parts.append(f["unit"])
    return "; ".join(parts) if parts else "raw byte"


def md_escape(s):
    return s.replace("|", "\\|")


def build_doc_block(d):
    out = []
    for e in d["ids"]:
        idtxt = "0x" + e["id"] + (f" to 0x{e['to']}" if "to" in e else "")
        meta = []
        if e.get("ms"):
            meta.append(f"every {e['ms']} ms")
        if e.get("from"):
            meta.append(f"from {e['from']}")
        out.append(f"#### {idtxt}: {e['name']}" + (f" ({', '.join(meta)})" if meta else ""))
        out.append("")
        if e.get("bikes"):
            out.append("Seen on: " + ", ".join(f"{d['bikes'][b]} ({s})" for b, s in e["bikes"].items()) + "  ")
        if e.get("try"):
            out.append(f"Test: {e['try']}  ")
        if e.get("note"):
            out.append(f"Note: {e['note']}  ")
        out.append("")
        if e.get("fields"):
            out.append("| Bytes | Meaning | How to read it | Bike: status |")
            out.append("| --- | --- | --- | --- |")
            for f in e["fields"]:
                if "text" in f:
                    out.append(f"| D{f['b']} | {md_escape(f['text'])} | | |")
                    continue
                st = ", ".join(f"{b}: {s}" for b, s in f["bikes"].items())
                meaning = f["name"] + (f" ({f['note']})" if f.get("note") else "")
                out.append(f"| {byte_range(f)} | {md_escape(meaning)} | {md_escape(how(f))} | {st} |")
            out.append("")
        if e.get("src"):
            out.append("Sources: " + ", ".join(f"[{s}]({d['sources'][s]['url']})" for s in e["src"]))
            out.append("")
    return "\n".join(out).rstrip() + "\n"


def build_doc(text, d):
    if DOC_BEGIN not in text:
        raise SystemExit(f"{DOC}: markers {DOC_BEGIN} ... {DOC_END} not found")
    block = f"{DOC_BEGIN}\n{build_doc_block(d)}{DOC_END}"
    return re.sub(re.escape(DOC_BEGIN) + ".*?" + re.escape(DOC_END), lambda m: block, text, flags=re.S)


def main():
    check = "--check" in sys.argv
    d = json.loads(MASTER.read_text(encoding="utf-8"))
    errs = validate(d)
    if errs:
        print("can_ids.json problems:\n  " + "\n  ".join(errs))
        return 1
    stale = []
    for path, build in ((WEB, build_web), (DOC, build_doc)):
        old = path.read_text(encoding="utf-8")
        new = build(old, d)
        if new != old:
            if check:
                stale.append(str(path.relative_to(ROOT)))
            else:
                path.write_text(new, encoding="utf-8")
                print("updated", path.relative_to(ROOT))
    if stale:
        print("Out of date (run: python3 tools/gen_can_ids.py): " + ", ".join(stale))
        return 1
    print(f"can_ids.json OK: {len(d['ids'])} entries" + (" (generated files up to date)" if check else ""))
    return 0


if __name__ == "__main__":
    sys.exit(main())
