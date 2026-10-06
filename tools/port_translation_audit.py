#!/usr/bin/env python3
"""Which cited patched RoR 1.0a game functions are documented as translations.

tools/port_worklist.py counts a function as cited when its address appears
anywhere in src/ or include/. Many addresses appear only because a
translation names the function it calls (an ops-struct callback comment,
"0x0041c120(2)" in a description). This audit is stricter: a function counts
as translated when its 8-digit address appears

  - in a header outside struct bodies (the documentation of a translated
    function sits above its prototype; ops structs list callees), or
  - in a src/ comment within four lines above a function definition.

It is still a heuristic: a callee named in a function's documentation passes
(for example a constructor named in a screen's header text), so the list is a
lower bound on what is untranslated. Review each entry before acting on it.

    python3 tools/port_translation_audit.py [--all]
"""
import bisect
import collections
import glob
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import port_worklist as w  # noqa: E402

ADDR = re.compile(r"0x00([45][0-9a-f]{5})")
FUNC_DEF = re.compile(r"^[A-Za-z_][\w \*]*\b\w+\s*\([^;]*\)\s*\{", re.M)
STRUCT_OPEN = re.compile(r"\s*(typedef\s+)?struct\b[^;]*$")


def header_mentions(path, doc, anywhere):
    depth = 0
    in_struct = False
    with open(path, errors="replace") as f:
        for line in f:
            code = re.sub(r"/\*.*?\*/", "", line)
            if not in_struct and STRUCT_OPEN.match(code) and "(" not in code:
                in_struct = True
                depth = 0
            for m in ADDR.finditer(line):
                a = int(m.group(1), 16)
                anywhere[a].add(path)
                if not in_struct:
                    doc[a].add(path)
            if in_struct:
                depth += code.count("{") - code.count("}")
                if "}" in code and depth <= 0:
                    in_struct = False


def source_mentions(path, doc, anywhere):
    with open(path, errors="replace") as f:
        lines = f.read().split("\n")
    for i, line in enumerate(lines):
        found = ADDR.findall(line)
        if not found:
            continue
        comment = "/*" in line or line.strip().startswith("*")
        follows = FUNC_DEF.search("\n".join(lines[i + 1:i + 5])) is not None
        for g in found:
            a = int(g, 16)
            anywhere[a].add(path)
            if comment and follows:
                doc[a].add(path)


def main():
    root = w.ROOT
    os.chdir(root)
    starts, size = w.load_index()
    game = [a for a in starts if 0x401000 <= a and w.kind(a) == "game" and a < w.TEXT_END]
    doc = collections.defaultdict(set)
    anywhere = collections.defaultdict(set)
    for path in sorted(glob.glob("include/*.h")):
        header_mentions(path, doc, anywhere)
    for path in sorted(glob.glob("src/*.c") + glob.glob("src/*/*.c")):
        source_mentions(path, doc, anywhere)
    cited = [a for a in game if anywhere.get(a)]
    missing = [a for a in cited if not doc.get(a)]
    print("Patched RoR 1.0a game code: %d functions, %d with an 8-digit citation, "
          "%d of those only mentioned as callees (%d bytes)"
          % (len(game), len(cited), len(missing), sum(size[a] for a in missing)))
    limit = None if "--all" in sys.argv else 40
    print()
    print("| Address | Size | Mentioned in |")
    print("| --- | ---: | --- |")
    for a in sorted(missing, key=lambda a: -size[a])[:limit]:
        print("| 0x%08x | %d | %s |" % (a, size[a], ", ".join(sorted(anywhere[a])[:2])))


if __name__ == "__main__":
    main()
