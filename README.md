# k-input LUT Mapping

A small, dependency-free C++17 technology mapper for FPGA-style **k-input look-up tables (LUTs)**.
It reads a combinational circuit in BLIF, converts it to an And-Inverter Graph (AIG), enumerates
k-feasible cuts, picks one cut per node with a **depth-oriented mapping followed by area recovery**,
and writes the resulting LUT network back out as BLIF (one `.names` block per LUT).

It started as Programming Assignment 2 of NCU **EE6094 "CAD for VLSI Design"** and is published
here as a readable reference implementation of the classic cut-based mapping flow.

## Pipeline

```
input.blif ──▶ BlifParser ──▶ AigBuilder ──▶ CutSelector ──▶ LutBuilder ──▶ BlifWriter ──▶ output.blif
               (.names SOP)   (AIG, hashed)  (cuts, depth,   (truth tables    (.names per LUT)
                                              area recovery)  per chosen cut)
```

`main.cpp` runs these five stages in order and prints a diagnostic dump of each stage to stdout.

### 1. BLIF parsing (`src/blif_parser.cpp`)
- Joins `\`-continued lines, strips `#` comments, tolerates `\r` line endings.
- Handles `.model`, `.inputs`, `.outputs`, `.names` (with SOP rows using `0`/`1`/`-`) and `.end`.
- Each `.names` block is stored as a list of SOP terms; a missing output value on a row defaults to `1`.
- Any other `.`-directive (e.g. `.latch`, `.subckt`, `.exdc`) makes `parse()` return `false` and the program exits with status 1.

### 2. AIG construction (`src/aig_builder.cpp`)
- Node 0 is `CONST0`; literal `1` is constant 1. A literal is `(id << 1) | invert`.
- `makeAnd` does constant propagation (`0&x`, `1&x`, `x&x`, `x&~x`), canonical operand ordering and
  **structural hashing**, so identical AND gates are shared.
- Each SOP row becomes a **balanced AND tree** of its literals (depth `ceil(log2 n)` instead of `n-1`);
  a row with output value `0` is complemented. The rows are combined with a **balanced OR tree**,
  where OR is implemented via De Morgan (`a|b = ~(~a & ~b)`). A `.names` block with no rows maps to constant 0.
- Blocks are visited in dependency order with a recursive DFS, so the input file does not need to be topologically sorted.

### 3. k-feasible cut enumeration (`CutSelector::enumerateCuts`)
- Bottom-up in node-id order. Every node gets its trivial cut `{node}`; an AND node additionally
  gets the pairwise merge (sorted set-union) of every cut of `fanin0` with every cut of `fanin1`.
- **Pruning:** a merged cut is discarded when it has more than `k` leaves, or when an existing cut of the
  same node is a subset of it (dominance). Conversely, existing cuts that are supersets of the new cut are removed.
- Enumeration is **exhaustive** apart from dominance: there is no priority-cut limit and no cost-based
  truncation, so cut counts can grow quickly for large `k` (see Limitations).

### 4. Cut selection (`depthMapping` + `areaRecovery`)
- **Phase 2, depth mapping.** Unit delay per LUT, arrival time 0 at PIs/constants. For each AND node the
  cut with the smallest `max(arrival(leaf)) + 1` is chosen (first one wins on ties). The circuit depth is
  the maximum arrival time over the primary outputs.
- **Phase 3, area recovery (area flow).** Walking backward from the POs through the chosen cuts marks
  the *required* nodes; required times are propagated backward from `depth` (`req(leaf) = min(req(leaf), req(node) - 1)`).
  Area flow is `areaFlow(v) = (1 + Σ areaFlow(leaf)) / fanout(v)` with PIs at 0 and AIG structural fanout as the divisor.
  Each required node is then re-assigned the cut with minimum area flow among those whose leaves all
  satisfy `arrival(leaf) < req(node)`, so the mapped depth is never increased.
- Objective, in short: **minimum depth first, then area-flow reduction under that depth constraint.**
  There is no separate exact-area mode and no switch to change the objective.

### 5. LUT construction and writing (`src/lut_builder.cpp`, `src/blif_writer.cpp`)
- Only AND nodes reachable from the POs through the final cuts become LUTs (the cover is re-derived here).
- The truth table of each LUT is obtained by **simulating the AIG cone** for all `2^m` leaf assignments (`m ≤ k`)
  and emitting one SOP row per on-set minterm (no don't-cares, no minimisation).
- If a BLIF signal is the complement of an AND node and that node is not used elsewhere as a cut leaf, the
  inversion is absorbed into the LUT; otherwise a one-input inverter LUT (`0 1`) is emitted.
- Internal nodes that need a name are called `n<id>`. `.names` lines longer than ~80 characters are wrapped with `\`.

## Building

```bash
make            # g++ -std=c++17 -O2 -Wall, objects in build/, binary ./lutmap
make test       # maps every testcase/*.blif with k=4 into output/
make clean      # removes build/ and the binary
make run input=testcase/testcase1.blif output=out.blif k=4
```

No external dependencies (standard library only). Tested with GCC on Linux; MinGW (`mingw32-make`)
should work as well since the `clean` target also removes `lutmap.exe`.

## Usage

```
./lutmap -input <in.blif> -output <out.blif> [-k <2..10>] [-quiet]
```

- `-input` and `-output` are required; the program prints a usage line and exits with 1 otherwise.
- `-k` defaults to `4` and is validated to the range 2..10 (out-of-range values exit with 1).
- `-quiet` suppresses the per-stage dumps and prints only `LUTs: <n>  depth: <d>`.
- Unknown arguments are rejected.

## Supported BLIF subset

| Supported | Not supported |
|---|---|
| `.model`, `.inputs`, `.outputs`, `.names`, `.end` | `.latch` (sequential logic), `.subckt`, `.exdc`, `.clock`, and any other directive: parsing aborts |
| `#` comments, `\` continuation lines | Don't-care specification (`.exdc`); `-` inside SOP rows *is* supported |
| Rows with output `0` (off-set) and `1` (on-set) | Multi-model files: parsing stops at the first `.end` |
| Multi-input `.names` of any width (decomposed into a balanced AIG) | |
| Outputs that are a buffer / inverter of a PI, an alias of another signal, or a constant (a 1-input or 0-input LUT is emitted) | |

Output files contain exactly `.model`, `.inputs`, `.outputs`, one `.names` per LUT with fully specified on-set minterms, and `.end`.

## Example

`testcase/testcase1.blif` is a 12-input, 8-output PLA-style circuit whose blocks each read 3 or 4 inputs.

```bash
make
./lutmap -input testcase/testcase1.blif -output output/testcase1_cut4.blif -k 4
```

The console shows the parsed blocks, the AIG (45 nodes here: 1 constant, 12 PIs, 32 ANDs), then

```
======Cut Selection Results======
CutSelector (k=4, depth=1)
```

and a dump of the written BLIF. With `k = 4` every block fits in one LUT, so the output has **8 LUTs and depth 1**;
`v12.0` becomes a 4-input `.names` with its 12 on-set minterms listed one per row. Shrinking `k` forces decomposition:

| k | LUTs | depth |
|---|---|---|
| 2 | 33 | 3 |
| 3 | 23 | 2 |
| 4 – 8 | 8 | 1 |

The `output/` directory keeps the results for `testcase1..3` with `k = 2..8` (`testcaseN_cutK.blif`).

## Verifying equivalence

The mapper does not self-check. For small circuits (≤ 16 primary inputs) an exhaustive simulator is included:

```bash
python3 tools/verify_blif.py testcase/testcase1.blif output/testcase1_cut4.blif
# EQUIVALENT
```

`testcase1` maps equivalently for every `k` in 2..8. For larger circuits use
[ABC](https://github.com/berkeley-abc/abc) (an `abc/` checkout can sit next to the sources; it is git-ignored):

```
abc 01> cec testcase/testcase1.blif output/testcase1_cut4.blif
Networks are equivalent.
```

To compare quality against ABC's own mapper: `read testcase1.blif; strash; if -K 4; print_stats`.

## Complexity and limitations

- **Cut explosion.** Without a per-node cut limit the number of k-feasible cuts grows roughly exponentially
  in `k`; the pairwise merge is `O(|C(f0)| · |C(f1)|)` per node and every merge runs a linear dominance scan.
  Circuits with a few thousand AND nodes are fine at `k ≤ 6`; large designs at `k = 8..10` will be slow and memory-hungry.
- **Truth-table extraction** re-simulates the AIG prefix for each of the `2^k` assignments, i.e. `O(2^k · N)` per LUT.
- **Output size.** Every on-set minterm is written explicitly (up to `2^k` rows per LUT); no SOP minimisation.
- Combinational only; single model per file; no `.latch`.
- Depth is measured in unit LUT levels; no wire delay, no fanout-dependent delay.
- Diagnostic dumps go to stdout for every stage and are verbose (they list every AIG node and every chosen cut); use `-quiet` to suppress them.

## Project structure

```
.
├── Makefile
├── src/
│   ├── main.cpp            # CLI + five-stage pipeline
│   ├── blif_parser.cpp
│   ├── aig_builder.cpp
│   ├── cut_selector.cpp    # cut enumeration, depth mapping, area recovery
│   ├── lut_builder.cpp
│   └── blif_writer.cpp
├── inc/
│   ├── types.h             # BlifNetwork, Aig, Cut, Lut
│   ├── blif_parser.h  aig_builder.h  cut_selector.h  lut_builder.h  blif_writer.h
├── tools/
│   └── verify_blif.py      # exhaustive equivalence check for small circuits
├── testcase/               # testcase1..3.blif + inv_po.blif (regression for PO aliases)
└── output/                 # git-ignored; `make test` writes testcaseN_k4.blif here
```

## Roadmap / 目標

1. Add a `-mode {depth,area}` switch: pure area-flow mapping vs the current depth-then-area-recovery flow.
2. Second area-recovery pass with **exact area** (reference counting), as in the classic
   depth-optimal + area-recovery mappers.
3. **Priority cuts** (`-C <n>`, e.g. keep the best 8–16 cuts per node) to bound cut explosion at large `k`.
4. Support `.latch` by treating latch outputs as PIs and latch inputs as POs of the combinational core.
5. Unit tests (parser round-trip, AIG constant folding, cut dominance) plus a GitHub Actions job that
   builds, maps the test cases, and checks them with ABC `cec`.
6. Benchmark table on ISCAS'85 / MCNC circuits: LUT count and depth vs ABC `if -K k`.
7. Write truth tables with don't-cares or a minimised SOP to shrink output files.

## Licence and author

Suggested licence: **MIT** (add a `LICENSE` file before tagging a release).

Author: Tung Lun Yang — https://github.com/protire0821

## Academic honesty

This code was written as a graded programming assignment for EE6094 at National Central University.
It is shared for learning and as a reference implementation. If you are currently taking this course
(or one with the same assignment), do not submit this code or derivatives of it as your own work;
check your course's policy before even reading it in detail.

## 快速開始 (中文)

```bash
make                                                    # 以 g++ -std=c++17 -O2 編譯，產生 ./lutmap
./lutmap -input testcase/testcase1.blif -output out.blif -k 4
python3 tools/verify_blif.py testcase/testcase1.blif out.blif   # 小電路可直接驗證等價
```

- 流程：BLIF 解析 → 建立 AIG（SOP 拆成平衡 AND/OR 樹、結構雜湊）→ 列舉 k-feasible cuts（支配剪枝）
  → 深度導向選 cut → 在不增加深度的前提下以 area flow 做面積回復 → 對每個 cut 模擬真值表 → 輸出 `.names`。
- 目標函數：**先求最小深度（LUT 層數），再在深度限制下降低面積**。
- 只支援組合電路（`.model/.inputs/.outputs/.names/.end`），不支援 `.latch`。`k` 需 ≥ 2。
- 驗證等價性：`abc` 中執行 `cec <原始.blif> <輸出.blif>`。

<!--
Questions for the author to confirm before publishing:
1. Optimisation objective: the code is depth-first with area-flow recovery. Was that the assignment's
   required objective (e.g. "minimise depth, then LUT count"), and is the ranking/grading metric known?
2. testcase1 (k=2..8) was verified with tools/verify_blif.py. testcase2/3 have too many PIs for the
   exhaustive checker — please run ABC `cec` on them once and note the result.
3. Should the assignment report (.docx / .pdf) be excluded from the public repo? .gitignore already
   ignores *.pdf but not *.docx.
4. Does the EE6094 course policy allow publishing assignment code publicly? Some courses forbid it or
   require a delay after the semester.
5. Should output/ stay tracked in git? It is listed in .gitignore, yet the repo contains output/*.blif.
6. Are testcase2/3 original course-provided files that may be redistributed, or derived from a
   benchmark suite (ISCAS/MCNC) that needs attribution?
-->
