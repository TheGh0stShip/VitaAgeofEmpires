#!/usr/bin/env python3
"""Match functions between executables by normalized instruction sequence.

Maps disc Rise of Rome (build/ror/disc/GAME/EMPIRESX.EXE) and patched base
AoE 1.0c (build/patch_1_0c/EMPIRES.EXE) functions onto patched RoR 1.0a
(build/patch_1_0c/EMPIRESX.EXE), so citations of those revisions can be
related to the primary target. Function starts come from the Ghidra
indexes in build/decompile plus every direct call target in the
disassembly and every data-section dword that points at an instruction
(vtable slots), since the disc index misses some starts, which would merge
bodies.
A body runs to the next start.

Normalization keeps mnemonics and small constants (struct offsets,
counts) and masks every value at or above 0x400000 (code and data
addresses, relocated between builds) and branch targets.

Pass 1 (exact): hashes unique on both sides. The pairs that keep link
order (longest increasing run) become anchors.
Pass 2 (similar): between consecutive anchors, functions pair by the
Jaccard similarity of their instruction multisets with registers masked
(order-insensitive, so register reallocation and rescheduling still
match); only mutual best pairs at or above 0.6 are kept.

Writes build/decompile/ror_10a_match_<name>.tsv (source, patched, kind,
score).

usage: revision_match.py
"""
import collections
import hashlib
import os
import re
import subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DECOMPILE = os.path.join(ROOT, "build", "decompile")
TARGET = ("ror_10a", os.path.join(ROOT, "build/patch_1_0c/EMPIRESX.EXE"))
SOURCES = [
    ("disc", "ror", os.path.join(ROOT, "build/ror/disc/GAME/EMPIRESX.EXE")),
    ("base_10c", "base_10c", os.path.join(ROOT, "build/patch_1_0c/EMPIRES.EXE")),
]
INSN = re.compile(r"^\s*([0-9a-f]+):\s+(?:[0-9a-f]{2} )+\s*(.*)$")
HEX = re.compile(r"0x([0-9a-f]+)")
BRANCH = re.compile(r"^(call|j[a-z]+|loop[a-z]*)\s+[0-9a-f]+(?: <[^>]*>)?$")


def starts_of(index):
    with open(os.path.join(DECOMPILE, index + "_index.tsv")) as f:
        next(f)
        return sorted(int(line.split("\t")[0], 16) for line in f)


def norm(text):
    text = text.split("#")[0].strip()
    if BRANCH.match(text):
        return text.split()[0] + " T"
    return HEX.sub(lambda m: "A" if int(m.group(1), 16) >= 0x400000
                   else m.group(0), text)


REG = re.compile(r"\b(e?[abcd]x|[abcd][lh]|e?[sd]i|e?[sb]p)\b")


def data_pointers(exe):
    import struct
    with open(exe, "rb") as f:
        d = f.read()
    pe = struct.unpack_from("<I", d, 0x3c)[0]
    count = struct.unpack_from("<H", d, pe + 6)[0]
    opt = struct.unpack_from("<H", d, pe + 20)[0]
    base = struct.unpack_from("<I", d, pe + 52)[0]
    out = set()
    for i in range(count):
        name, vsize, va, rsize, raw = struct.unpack_from(
            "<8sIIII", d, pe + 24 + opt + 40 * i)
        flags = struct.unpack_from("<I", d, pe + 24 + opt + 40 * i + 36)[0]
        if flags & 0x20000000:  # executable
            continue
        n = min(vsize, rsize) // 4
        out.update(struct.unpack_from("<%dI" % n, d, raw))
    return out


def bodies_of(exe, starts):
    lo, hi = starts[0], starts[-1] + 0x1000
    asm = subprocess.run(["objdump", "-d", "-Mintel", "--no-show-raw-insn",
                          "--start-address=0x%x" % lo,
                          "--stop-address=0x%x" % hi, exe],
                         check=True, capture_output=True, text=True).stdout
    calls = re.compile(r"\scall\s+([0-9a-f]+)\b")
    extra = set()
    for line in asm.splitlines():
        m = calls.search(line)
        if m:
            t = int(m.group(1), 16)
            if lo <= t < starts[-1]:
                extra.add(t)
    insns = {int(m.group(1), 16) for m in
             re.finditer(r"^\s*([0-9a-f]+):", asm, re.M)}
    extra |= {p for p in data_pointers(exe)
              if lo <= p < starts[-1] and p in insns}
    starts_set = set(starts) | extra
    bodies = collections.OrderedDict()
    current = None
    for line in asm.splitlines():
        m = re.match(r"^\s*([0-9a-f]+):\s+(.*)$", line)
        if not m:
            continue
        a = int(m.group(1), 16)
        if a in starts_set:
            current = a
            bodies[a] = []
        if current is not None and m.group(2) and m.group(2) != "nop" and \
                not m.group(2).startswith("(bad)"):
            bodies[current].append(norm(m.group(2)))
    for body in bodies.values():
        while body and body[-1] in ("nop", "int3", "lea    esi,[esi+0x0]"):
            body.pop()
    return {a: b for a, b in bodies.items() if len(b) >= 4}


def hashes(bodies):
    return {a: hashlib.sha1("\n".join(b).encode()).hexdigest()
            for a, b in bodies.items()}


def bag(body):
    return collections.Counter(REG.sub("R", x) for x in body)


def jaccard(x, y):
    inter = sum((x & y).values())
    union = sum((x | y).values())
    return inter / union if union else 0.0


def increasing(pairs):
    """Longest run of pairs increasing in both addresses."""
    import bisect
    tails, tail_idx, prev = [], [], [None] * len(pairs)
    for i, (_, t) in enumerate(pairs):
        k = bisect.bisect_left(tails, t)
        if k == len(tails):
            tails.append(t)
            tail_idx.append(i)
        else:
            tails[k] = t
            tail_idx[k] = i
        prev[i] = tail_idx[k - 1] if k else None
    out, i = [], tail_idx[-1] if tail_idx else None
    while i is not None:
        out.append(pairs[i])
        i = prev[i]
    return out[::-1]


def unique(h):
    count = collections.Counter(h.values())
    return {v: a for a, v in h.items() if count[v] == 1}


def main():
    tb = bodies_of(TARGET[1], starts_of(TARGET[0]))
    target = unique(hashes(tb))
    tkeys = sorted(tb)
    for name, index, exe in SOURCES:
        if not os.path.exists(exe):
            continue
        sb = bodies_of(exe, starts_of(index))
        source = unique(hashes(sb))
        exact = sorted((a, target[v]) for v, a in source.items()
                       if v in target)
        anchors = increasing(exact)
        result = {s: (t, "exact", 1.0) for s, t in exact}
        skeys = sorted(sb)
        bounds = [(0, 0)] + anchors + [(1 << 32, 1 << 32)]
        import bisect
        for (s0, t0), (s1, t1) in zip(bounds, bounds[1:]):
            ss = [a for a in skeys[bisect.bisect_right(skeys, s0):
                                   bisect.bisect_left(skeys, s1)]
                  if a not in result]
            ts = tkeys[bisect.bisect_right(tkeys, t0):
                       bisect.bisect_left(tkeys, t1)]
            if not ss or not ts or len(ss) * len(ts) > 40000:
                continue
            sbags = {a: bag(sb[a]) for a in ss}
            tbags = {a: bag(tb[a]) for a in ts}
            best_s, best_t = {}, {}
            for a in ss:
                for b in ts:
                    j = jaccard(sbags[a], tbags[b])
                    if j > best_s.get(a, (None, 0))[1]:
                        best_s[a] = (b, j)
                    if j > best_t.get(b, (None, 0))[1]:
                        best_t[b] = (a, j)
            for a, (b, j) in best_s.items():
                if j >= 0.6 and best_t[b][0] == a:
                    result[a] = (b, "similar", j)
        path = os.path.join(DECOMPILE, "ror_10a_match_%s.tsv" % name)
        with open(path, "w") as f:
            f.write("source\tpatched_ror_1_0a\tkind\tscore\n")
            for a in sorted(result):
                b, k, j = result[a]
                f.write("%08x\t%08x\t%s\t%.3f\n" % (a, b, k, j))
        print("%s: %d exact (%d anchors), %d similar -> %s" %
              (name, len(exact), len(anchors), len(result) - len(exact),
               os.path.relpath(path, ROOT)))


if __name__ == "__main__":
    main()
