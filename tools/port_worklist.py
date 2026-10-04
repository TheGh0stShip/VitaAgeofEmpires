#!/usr/bin/env python3
"""Port coverage and translation worklist for patched Rise of Rome 1.0a.

Inputs (generated locally by the Ghidra scripts in tools/, git-ignored):
  build/decompile/ror_10a_index.tsv   function starts
  build/decompile/ror_10a_calls.tsv   direct call graph

A function counts as cited when its start address appears in src/ or
include/ (forms 0x004b6290, 0x4b6290 or 004b6290), either directly or as a
disc-RoR / patched-base-1.0c address that tools/revision_match.py maps onto
it (build/decompile/ror_10a_match_*.tsv). Citations are not
revision-tagged, so an address of one revision that happens to equal a
function start of another is counted too; treat the totals as estimates.

Sizes are the distance to the next function start (padding included).
Functions from CRT_START on are the statically linked Visual C runtime
(0x525c24..0x530fff, Win32 file/heap/locale calls) and compiler-generated
exception-unwind funclets (0x531000 on); the range ASM_START..CRT_START is a
hand-written assembly module (register-passing, no frames). Both are reported
separately and left out of the worklist.

The worklist orders uncited game functions leaf-first: a function is
"ready" when every direct internal callee is cited, runtime or itself.

--classes groups functions by vtable. Vtable starts are the rdata
immediates that constructors store at [this] (`mov dword [reg], imm32`,
found with objdump and cached as build/decompile/ror_10a_vtables.txt); each
splits its pointer run (ror_10a_pointer_runs_after_islands.tsv) up to the
next start. A target found in several vtables is inherited or shared; a
class's own functions are targets found only in its vtable. Classes are
ranked by uncited own bytes.

usage: port_worklist.py [--summary] [--classes] [--region 0x4b]
                        [--limit N] [--markdown OUT.md]
"""
import argparse
import collections
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DECOMPILE = os.path.join(ROOT, "build", "decompile")
TEXT_END = 0x00537000
ASM_START = 0x00524E90
CRT_START = 0x00525C24
CITE = re.compile(r"(?<![0-9A-Za-z_])(?:0x00|0x|00)([45][0-9a-fA-F]{5})(?![0-9A-Za-z_])")


def load_index():
    starts = []
    with open(os.path.join(DECOMPILE, "ror_10a_index.tsv")) as f:
        next(f)
        for line in f:
            starts.append(int(line.split("\t")[0], 16))
    starts.sort()
    size = {}
    for i, a in enumerate(starts):
        end = starts[i + 1] if i + 1 < len(starts) else a + 16
        if a < TEXT_END <= end:
            end = TEXT_END
        size[a] = end - a
    return starts, size


def load_calls():
    callees = collections.defaultdict(set)
    callers = collections.defaultdict(set)
    with open(os.path.join(DECOMPILE, "ror_10a_calls.tsv")) as f:
        next(f)
        for line in f:
            c = line.rstrip("\n").split("\t")
            if len(c) < 5 or c[4] != "internal":
                continue
            a, b = int(c[0], 16), int(c[2], 16)
            callees[a].add(b)
            callers[b].add(a)
    return callees, callers


def load_matches():
    maps = {}
    for name in ("disc", "base_10c"):
        path = os.path.join(DECOMPILE, "ror_10a_match_%s.tsv" % name)
        if not os.path.exists(path):
            continue
        m = {}
        with open(path) as f:
            next(f)
            for line in f:
                c = line.split("\t")
                m[int(c[0], 16)] = int(c[1], 16)
        maps[name] = m
    return maps


def load_citations(starts):
    known = set(starts)
    maps = load_matches()
    cited = collections.defaultdict(set)
    for top in ("src", "include"):
        for dirpath, _, files in os.walk(os.path.join(ROOT, top)):
            for name in files:
                if not name.endswith((".c", ".h")):
                    continue
                path = os.path.join(dirpath, name)
                with open(path, errors="replace") as f:
                    text = f.read()
                rel = os.path.relpath(path, ROOT)
                for m in CITE.finditer(text):
                    a = int(m.group(1), 16)
                    if a in known:
                        cited[a].add(rel)
                    for name, mp in maps.items():
                        b = mp.get(a)
                        if b in known:
                            cited[b].add(rel + " (" + name + ")")
    return cited


EXE = os.path.join(ROOT, "build", "patch_1_0c", "EMPIRESX.EXE")
STORE = re.compile(r"mov\s+DWORD PTR \[e[a-z]{2}\],0x(5[45][0-9a-f]{4})$")


def vtable_starts():
    cache = os.path.join(DECOMPILE, "ror_10a_vtables.txt")
    if not os.path.exists(cache):
        import subprocess
        asm = subprocess.run(
            ["objdump", "-d", "-Mintel", "--start-address=0x401000",
             "--stop-address=0x%x" % ASM_START, EXE],
            check=True, capture_output=True, text=True).stdout
        found = sorted({int(m.group(1), 16) for line in asm.splitlines()
                        for m in [STORE.search(line)] if m})
        with open(cache, "w") as f:
            f.writelines("%08x\n" % a for a in found)
    with open(cache) as f:
        return sorted(int(x, 16) for x in f if x.strip())


def load_vtables():
    runs = collections.OrderedDict()
    path = os.path.join(DECOMPILE, "ror_10a_pointer_runs_after_islands.tsv")
    if not os.path.exists(path):
        return runs
    with open(path) as f:
        next(f)
        for line in f:
            c = line.rstrip("\n").split("\t")
            if len(c) < 4:
                continue
            runs.setdefault(int(c[0], 16), []).append(
                (int(c[2], 16), int(c[3], 16)))
    starts = vtable_starts()
    tables = collections.OrderedDict()
    for run, slots in runs.items():
        cuts = [v for v in starts if run <= v < run + 4 * len(slots)]
        for i, v in enumerate(cuts):
            end = cuts[i + 1] if i + 1 < len(cuts) else run + 4 * len(slots)
            tables[v] = [(slot - (v - run), t) for slot, t in slots
                         if v <= run + slot < end]
    return tables


def kind(a):
    if a >= CRT_START:
        return "crt"
    if a >= ASM_START:
        return "asm"
    return "game"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--summary", action="store_true")
    ap.add_argument("--classes", action="store_true")
    ap.add_argument("--region", help="top address byte, e.g. 0x4b")
    ap.add_argument("--limit", type=int, default=40)
    ap.add_argument("--markdown")
    args = ap.parse_args()
    if not os.path.exists(os.path.join(DECOMPILE, "ror_10a_index.tsv")):
        sys.exit("missing build/decompile exports; run the Ghidra scripts")
    starts, size = load_index()
    callees, callers = load_calls()
    cited = load_citations(starts)
    game = [a for a in starts if kind(a) == "game"]
    done = set(cited) | {a for a in starts if kind(a) != "game"}

    regions = collections.OrderedDict()
    for a in game:
        r = regions.setdefault(a >> 16, [0, 0, 0, 0])
        r[0] += 1
        r[1] += size[a]
        if a in cited:
            r[2] += 1
            r[3] += size[a]
    total_n = len(game)
    total_b = sum(size[a] for a in game)
    cited_n = sum(1 for a in game if a in cited)
    cited_b = sum(size[a] for a in game if a in cited)

    def ready(a):
        return all(b in done or b == a for b in callees.get(a, ()))

    work = [a for a in game if a not in cited]
    if args.region:
        top = int(args.region, 16)
        work = [a for a in work if (a >> 16) == top]
    # Ready first, then by how many uncited callers wait on it, then size.
    work.sort(key=lambda a: (not ready(a),
                             -sum(1 for c in callers.get(a, ())
                                  if c not in cited),
                             size[a], a))

    lines = []
    lines.append("Patched RoR 1.0a game code: %d/%d functions cited (%.1f%%), "
                 "%d/%d bytes (%.1f%%)" % (cited_n, total_n,
                                           100.0 * cited_n / total_n, cited_b,
                                           total_b, 100.0 * cited_b / total_b))
    lines.append("Excluded: assembly module %d functions, C runtime %d "
                 "functions" % (sum(1 for a in starts if kind(a) == "asm"),
                                sum(1 for a in starts if kind(a) == "crt")))
    ready_n = sum(1 for a in game if a not in cited and ready(a))
    lines.append("Ready now (all callees cited): %d" % ready_n)
    lines.append("")
    lines.append("| Region | Functions | Cited | Bytes | Cited bytes |")
    lines.append("| --- | ---: | ---: | ---: | ---: |")
    for top, (n, b, cn, cb) in regions.items():
        lines.append("| 0x%04x0000 | %d | %d (%.0f%%) | %d | %d (%.0f%%) |" %
                     (top, n, cn, 100.0 * cn / n, b, cb, 100.0 * cb / b))
    if args.classes:
        runs = load_vtables()
        owners = collections.Counter()
        for slots in runs.values():
            for t in {t for _, t in slots}:
                owners[t] += 1
        rows = []
        for start, slots in runs.items():
            targets = {t for _, t in slots if t in size and kind(t) == "game"}
            own = [t for t in targets if owners[t] == 1]
            if not own:
                continue
            todo = [t for t in own if t not in cited]
            if args.region and not any((t >> 16) == int(args.region, 16)
                                       for t in own):
                continue
            rows.append((sum(size[t] for t in todo), start, len(slots),
                         len(own), len(todo),
                         sum(1 for t in targets if t in cited)))
        rows.sort(key=lambda r: (-r[0], r[1]))
        lines.append("")
        lines.append("Vtables with own functions: %d (fully cited: %d)" %
                     (len(rows), sum(1 for r in rows if r[4] == 0)))
        lines.append("")
        lines.append("| Vtable | Slots | Own | Own uncited | Uncited bytes | "
                     "Cited slots |")
        lines.append("| --- | ---: | ---: | ---: | ---: | ---: |")
        for r in rows[:args.limit]:
            lines.append("| 0x%08x | %d | %d | %d | %d | %d |" %
                         (r[1], r[2], r[3], r[4], r[0], r[5]))
    elif not args.summary:
        lines.append("")
        lines.append("| Next | Size | Ready | Waiting callers | Uncited callees |")
        lines.append("| --- | ---: | --- | ---: | ---: |")
        for a in work[:args.limit]:
            waiting = sum(1 for c in callers.get(a, ()) if c not in cited)
            missing = sum(1 for b in callees.get(a, ())
                          if b not in done and b != a)
            lines.append("| 0x%08x | %d | %s | %d | %d |" %
                         (a, size[a], "yes" if ready(a) else "no", waiting,
                          missing))
    out = "\n".join(lines) + "\n"
    if args.markdown:
        with open(args.markdown, "w") as f:
            f.write(out)
    sys.stdout.write(out)


if __name__ == "__main__":
    main()
