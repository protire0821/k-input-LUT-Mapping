# k-input LUT Mapping

A small, dependency-free C++17 technology mapper for FPGA-style k-input look-up tables.
It reads a combinational circuit in BLIF, converts it to an And-Inverter Graph, enumerates
k-feasible cuts, picks one cut per node (minimum depth first, then area-flow reduction under
that depth), and writes the mapped LUT network back out as BLIF.

Cut-based LUT mapping is a well-trodden problem and this implements the standard flow rather
than anything new — it is written from scratch in about 1,100 lines with no dependencies, so it is
readable end to end if you want to see how the pieces fit together. For production use, reach
for [ABC](https://github.com/berkeley-abc/abc) (`if -K k`).

```
input.blif → BLIF parse → AIG → k-feasible cuts → depth mapping → area recovery → LUT truth tables → output.blif
```

**[Try it in your browser →](https://protire0821.github.io/k-input-LUT-Mapping/)** — the same mapper compiled to
WebAssembly. Load a sample or paste your own netlist; nothing is uploaded.

## Build

```bash
make                    # g++ -std=c++17 -O2, produces ./lutmap
make test               # maps every testcase/*.blif with k=4 into output/
make clean
```

Standard library only, no dependencies. GCC on Linux; MinGW (`mingw32-make`) also works.

## Usage

```bash
./lutmap -input <in.blif> -output <out.blif> [-k <2..10>] [-quiet]
```

- `-input` / `-output` are required.
- `-k` defaults to 4, valid range 2–10.
- `-quiet` prints only `LUTs: <n>  depth: <d>` instead of the per-stage dumps.

## How it works

1. **Parse** — `.model` / `.inputs` / `.outputs` / `.names` / `.end`, with `#` comments and `\`
   continuation lines. Any other directive (`.latch`, `.subckt`, `.exdc`, …) aborts parsing.
2. **AIG** — each SOP row becomes a balanced AND tree, rows are combined with a balanced OR tree
   via De Morgan. Structural hashing and constant folding share identical gates.
3. **Cut enumeration** — bottom-up pairwise merge of fanin cuts, pruned by cut size `k` and by
   subset dominance. Exhaustive otherwise: no priority-cut limit.
4. **Depth mapping** — unit delay per LUT; each node takes the cut minimising
   `max(arrival(leaf)) + 1`.
5. **Area recovery** — nodes are re-assigned the cut with minimum area flow
   `(1 + Σ areaFlow(leaf)) / fanout`, restricted to cuts that keep the mapped depth.
6. **LUT output** — each chosen cut is simulated over all `2^m` leaf assignments; one `.names`
   row per on-set minterm (no don't-cares, no SOP minimisation).

## Example

`testcase/testcase1.blif` is a 12-input, 8-output circuit whose blocks each read 3–4 inputs.

```bash
./lutmap -input testcase/testcase1.blif -output output/testcase1_cut4.blif -k 4
```

| k | LUTs | depth |
|---|---|---|
| 2 | 33 | 3 |
| 3 | 23 | 2 |
| 4–8 | 8 | 1 |

`output/` holds the committed results for `testcase1..3` at `k = 2..8` (`testcaseN_cutK.blif`).

## Verifying equivalence

For small circuits (≤ 16 primary inputs) an exhaustive simulator is included:

```bash
python3 tools/verify_blif.py testcase/testcase1.blif output/testcase1_cut4.blif
# EQUIVALENT
```

`testcase1` maps equivalently for every `k` in 2–8. For larger circuits use
[ABC](https://github.com/berkeley-abc/abc): `cec <in.blif> <out.blif>`.
To compare quality: `read in.blif; strash; if -K 4; print_stats`.

## Limitations

- Combinational only — no `.latch`, single model per file.
- No per-node cut limit, so cut counts grow quickly at large `k`; `k ≤ 6` is comfortable.
- Truth-table extraction is `O(2^k · N)` per LUT; every on-set minterm is written explicitly.
- Depth is unit LUT levels only — no wire or fanout delay.

## Project structure

```
src/     main.cpp, blif_parser, aig_builder, cut_selector, lut_builder, blif_writer
inc/     types.h and one header per module
tools/   verify_blif.py — exhaustive equivalence check
testcase/  testcase1..3.blif, inv_po.blif (PO-alias regression)
output/  mapped results, testcaseN_cutK.blif
```

## Roadmap

- `-mode {depth,area}` switch for a pure area-oriented flow.
- Exact-area recovery pass (reference counting) after area flow.
- Priority cuts (`-C <n>`) to bound cut explosion at large `k`.
- `.latch` support by mapping the combinational core.
- Unit tests plus CI that builds, maps, and checks with ABC `cec`.
- Benchmark table against ABC `if -K k`.

## Web build

`docs/index.html` is a single self-contained page (mapper + samples inlined, ~370 KB) served by
GitHub Pages. To rebuild it you need [Emscripten](https://emscripten.org)
(`apt-get install emscripten` works):

```bash
./web/build.sh          # emcc → web/lutmap.js, then bundles docs/index.html
```

`web/wasm_api.cpp` is the Emscripten entry point — a thin `lutmap_map(text, k)` wrapper around the
same pipeline `main.cpp` drives. It is not part of the native build.

## Test circuits

`testcase1.blif` is a small PLA-style circuit. `testcase2.blif` and `testcase3.blif` are derived
from `alu4` in the MCNC / LGSynth benchmark suite.

## License

MIT — see `LICENSE`.

Tung Lun Yang — https://github.com/protire0821

## 快速開始 (中文)

```bash
make                                                              # 編譯，產生 ./lutmap
./lutmap -input testcase/testcase1.blif -output out.blif -k 4
python3 tools/verify_blif.py testcase/testcase1.blif out.blif     # 小電路可直接驗證等價
```

流程：BLIF 解析 → 建立 AIG（SOP 拆成平衡 AND/OR 樹、結構雜湊）→ 列舉 k-feasible cuts（支配剪枝）
→ 深度導向選 cut → 在不增加深度的前提下用 area flow 降低面積 → 模擬真值表輸出 `.names`。

目標函數是先求最小深度（LUT 層數），再在該深度限制下降低面積。只支援組合電路，不支援 `.latch`，`k` 需介於 2–10。
