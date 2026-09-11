#!/usr/bin/env bash
# Build the browser demo: compiles the mapper to WebAssembly and inlines
# everything into a single self-contained docs/index.html for GitHub Pages.
#
# Requires Emscripten (emcc). On Ubuntu: sudo apt-get install emscripten
# Run from the repository root:  ./web/build.sh
set -euo pipefail
cd "$(dirname "$0")/.."

emcc -std=c++17 -O2 \
  web/wasm_api.cpp \
  src/blif_parser.cpp src/aig_builder.cpp src/cut_selector.cpp \
  src/lut_builder.cpp src/blif_writer.cpp \
  -o web/lutmap.js \
  -sEXPORTED_FUNCTIONS='["_lutmap_map","_lutmap_error","_lutmap_luts","_lutmap_depth","_malloc","_free"]' \
  -sEXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
  -sMODULARIZE=1 -sEXPORT_NAME=createLutmap -sSINGLE_FILE=1 \
  -sALLOW_MEMORY_GROWTH=1 -sENVIRONMENT=web,worker,node

python3 web/bundle.py
echo "wrote docs/index.html"
