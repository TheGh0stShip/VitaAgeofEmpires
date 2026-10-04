#!/usr/bin/env python3
"""Print the local Ghidra reference decompilation of patched RoR 1.0a
functions (build/decompile/ror_10a_functions.txt) by address, for reading
while translating. Reference only: the output is not buildable source.

usage: decomp_show.py ADDR [ADDR...] | --range LO HI [--uncited] [--max N]
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import port_worklist as w  # noqa: E402

PATH = os.path.join(w.DECOMPILE, "ror_10a_functions.txt")


def load_bodies():
    bodies, cur, buf = {}, None, []
    with open(PATH, errors="replace") as f:
        for line in f:
            if line.startswith("/* FUNCTION "):
                if cur is not None:
                    bodies[cur] = "".join(buf)
                cur = int(line.split()[2], 16)
                buf = []
            elif cur is not None:
                buf.append(line)
    if cur is not None:
        bodies[cur] = "".join(buf)
    return bodies


def main():
    args = sys.argv[1:]
    bodies = load_bodies()
    starts, size = w.load_index()
    if args and args[0] == "--range":
        lo, hi = int(args[1], 16), int(args[2], 16)
        uncited = "--uncited" in args
        cited = w.load_citations(starts) if uncited else {}
        mx = int(args[args.index("--max") + 1]) if "--max" in args else 10**9
        addrs = [a for a in starts if lo <= a < hi and a not in cited][:mx]
    else:
        addrs = [int(a, 16) for a in args]
    for a in addrs:
        body = "\n".join(l for l in bodies.get(a, "?").splitlines()
                         if l.strip())
        print("==== %08x (%d bytes)\n%s" % (a, size.get(a, 0), body))


if __name__ == "__main__":
    main()
