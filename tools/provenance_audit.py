#!/usr/bin/env python3
"""Provenance audit for the reconstruction (docs/PROVENANCE.md).

1. References: the tracked sources, tools, tests and docs must not name or
   link other Genie-engine reconstructions, format libraries or generated
   helper code as sources. docs/PROVENANCE.md, which records the history,
   is the one exception. Any hit fails the run.
2. Evidence: every C file under src/ is classed by what it cites:
     translation  cites functions of the executables (0x004xxxxx /
                  0x005xxxxx), in the file or its header;
     port         cites none: code written for the port (platform, host,
                  tools glue) or derived from the user's data alone.
   Third-party code (third_party/) is listed separately.

    python3 tools/provenance_audit.py [--table]
"""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Other reconstructions, format libraries and generated-code sources whose
# work must not be relied on. Matched case-insensitively.
FORBIDDEN = [
    r"openage", r"genie[-_ ]?utils", r"genie-rs", r"freeaoe", r"chariot ?engine",
    r"folkertvanverseveld", r"sandsmark", r"\btapsa\b", r"sfttech", r"siegeengineers",
    r"deepseek", r"aoe2tools", r"aoc-mgz",
]
EXEMPT = {"docs/PROVENANCE.md", "tools/provenance_audit.py"}
ADDRESS = re.compile(r"(?:0x00?|\b00)([45][0-9a-f]{5})\b")


def tracked():
    out = subprocess.run(["git", "ls-files"], cwd=ROOT, capture_output=True, text=True,
                         check=True).stdout.split("\n")
    return [p for p in out if p]


def references(files):
    pattern = re.compile("|".join(FORBIDDEN), re.I)
    hits = []
    for path in files:
        if path in EXEMPT or not path.endswith((".c", ".h", ".md", ".py", ".sh", ".java", ".txt",
                                               ".cmake", "CMakeLists.txt", ".yml")):
            continue
        with open(os.path.join(ROOT, path), errors="replace") as f:
            for n, line in enumerate(f, 1):
                if pattern.search(line):
                    hits.append("%s:%d: %s" % (path, n, line.strip()[:100]))
    return hits


def citations(path):
    try:
        with open(os.path.join(ROOT, path), errors="replace") as f:
            return set(ADDRESS.findall(f.read()))
    except OSError:
        return set()


DEFINITION = re.compile(r"^(?!static)[A-Za-z_][\w \*]*?\b(aoe_\w+)\s*\(", re.M)


def header_docs(files):
    """Each prototype name in include/ with the addresses in the sixty lines
    above it (its documentation)."""
    docs = {}
    for path in files:
        if not (path.startswith("include/") and path.endswith(".h")):
            continue
        with open(os.path.join(ROOT, path), errors="replace") as f:
            lines = f.read().split("\n")
        for i, line in enumerate(lines):
            for name in re.findall(r"\b(aoe_\w+)\s*\(", line):
                above = "\n".join(lines[max(0, i - 60):i + 1])
                docs.setdefault(name, set()).update(ADDRESS.findall(above))
    return docs


def classes(files):
    docs = header_docs(files)
    rows = []
    for path in sorted(p for p in files if p.startswith("src/") and p.endswith(".c")):
        cited = citations(path) | citations("include/" + os.path.basename(path)[:-2] + ".h")
        with open(os.path.join(ROOT, path), errors="replace") as f:
            for name in DEFINITION.findall(f.read()):
                cited |= docs.get(name, set())
        rows.append((path, "translation" if cited else "port", len(cited)))
    return rows


def main():
    files = tracked()
    hits = references(files)
    rows = classes(files)
    third = sorted(p for p in files if p.startswith("third_party/"))
    if "--table" in sys.argv:
        print("| File | Class | Executable functions cited |")
        print("| --- | --- | ---: |")
        for path, kind, n in rows:
            print("| `%s` | %s | %d |" % (path, kind, n))
    else:
        t = sum(1 for r in rows if r[1] == "translation")
        print("src C files: %d translation, %d port" % (t, len(rows) - t))
        for path, kind, _ in rows:
            if kind == "port":
                print("  port: " + path)
        print("third-party files: " + (", ".join(third) if third else "none"))
    if hits:
        print("references to other projects (%d):" % len(hits))
        for h in hits:
            print("  " + h)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
