#!/usr/bin/env python3
"""Exhaustive equivalence check of two combinational BLIF files (small PI count only).
Usage: python tools/verify_blif.py golden.blif mapped.blif
"""
import sys, itertools

def parse(path):
    pis, pos, names = [], [], []
    cur = None
    with open(path) as f:
        lines = []
        for ln in f:
            ln = ln.split('#')[0].rstrip('\n')
            if lines and lines[-1].endswith('\\'):
                lines[-1] = lines[-1][:-1] + ln
            else:
                lines.append(ln)
    for ln in lines:
        t = ln.split()
        if not t: continue
        if t[0] == '.inputs': pis += t[1:]
        elif t[0] == '.outputs': pos += t[1:]
        elif t[0] == '.names':
            cur = (t[1:-1], t[-1], []); names.append(cur)
        elif t[0].startswith('.'): cur = None
        elif cur is not None:
            pat, val = (t[0], t[1]) if len(t) == 2 else ('', t[0])
            cur[2].append((pat, val))
    return pis, pos, names

def evaluate(names, env):
    # names are assumed in topological order (true for both files here); loop until stable otherwise
    for _ in range(len(names) + 1):
        changed = False
        for ins, out, rows in names:
            if any(i not in env for i in ins): continue
            onset = any(v == '1' for _, v in rows)
            val = 0
            for pat, v in rows:
                if all(p == '-' or int(p) == env[i] for p, i in zip(pat, ins)):
                    val = int(v); break
            else:
                val = 0 if onset else 1
            if env.get(out) != val: env[out] = val; changed = True
        if not changed: break
    return env

g_pis, g_pos, g_names = parse(sys.argv[1])
m_pis, m_pos, m_names = parse(sys.argv[2])
assert set(g_pis) == set(m_pis) and set(g_pos) == set(m_pos), "PI/PO sets differ"
if len(g_pis) > 16: sys.exit("too many PIs for exhaustive check; use ABC cec")
bad = 0
for bits in itertools.product([0, 1], repeat=len(g_pis)):
    env = dict(zip(g_pis, bits))
    a = evaluate(g_names, dict(env)); b = evaluate(m_names, dict(env))
    for po in g_pos:
        if a.get(po, 0) != b.get(po, 0):
            bad += 1
            if bad <= 5: print("MISMATCH", dict(env), po, a.get(po), b.get(po))
print("EQUIVALENT" if bad == 0 else f"NOT EQUIVALENT ({bad} mismatches)")
sys.exit(1 if bad else 0)
