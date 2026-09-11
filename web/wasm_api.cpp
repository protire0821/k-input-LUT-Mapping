// ============================================================================
// wasm_api.cpp — Emscripten entry point for the browser demo.
// Not part of the native build; see web/build.sh.
// ============================================================================
#include <emscripten.h>
#include <fstream>
#include <sstream>
#include <string>

#include "../inc/blif_parser.h"
#include "../inc/aig_builder.h"
#include "../inc/cut_selector.h"
#include "../inc/lut_builder.h"
#include "../inc/blif_writer.h"

namespace {
std::string g_result;
std::string g_error;
int g_luts  = 0;
int g_depth = 0;
}

extern "C" {

EMSCRIPTEN_KEEPALIVE
const char* lutmap_map(const char* blifText, int k) {
    g_result.clear();
    g_error.clear();
    g_luts = g_depth = 0;

    if (k < 2 || k > 10) { g_error = "k must be between 2 and 10"; return ""; }

    const std::string inPath  = "/tmp/in.blif";
    const std::string outPath = "/tmp/out.blif";
    { std::ofstream f(inPath); if (!f) { g_error = "cannot stage input"; return ""; } f << blifText; }

    BlifParser parser;
    if (!parser.parse(inPath)) {
        g_error = "BLIF parse failed — check the file uses only "
                  ".model/.inputs/.outputs/.names/.end";
        return "";
    }

    AigBuilder aig;
    if (!aig.build(parser.getNetwork())) { g_error = "AIG construction failed"; return ""; }

    CutSelector cuts(k);
    cuts.enumerateCuts(aig.getAig());
    cuts.depthMapping(aig.getAig());
    cuts.areaRecovery(aig.getAig());

    LutBuilder luts(k);
    luts.buildLuts(aig.getAig(), cuts.getBestCuts(), parser.getNetwork().primaryOutputs);

    BlifWriter writer;
    if (!writer.write(outPath,
                      parser.getNetwork().modelName,
                      parser.getNetwork().primaryInputs,
                      parser.getNetwork().primaryOutputs,
                      luts.getLuts())) {
        g_error = "failed to write output";
        return "";
    }

    std::ifstream f(outPath);
    std::ostringstream ss; ss << f.rdbuf();
    g_result = ss.str();
    g_luts   = (int)luts.getLuts().size();
    g_depth  = cuts.getDepth();
    return g_result.c_str();
}

EMSCRIPTEN_KEEPALIVE const char* lutmap_error() { return g_error.c_str(); }
EMSCRIPTEN_KEEPALIVE int lutmap_luts()  { return g_luts;  }
EMSCRIPTEN_KEEPALIVE int lutmap_depth() { return g_depth; }

} // extern "C"
