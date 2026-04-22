// ============================================================================
// 114521123_PA2.cpp
// EE6094 CAD for VLSI Design - Programming Assignment 2
// k-input LUT Mapping
// ----------------------------------------------------------------------------
// Usage:
//     ./114521123_PA2 -input <input.blif> -output <output.blif> -k <2~10>
// ============================================================================

#include <iostream>
#include <string>

#include "inc/blif_parser.h"
#include "inc/aig_builder.h"
#include "inc/cut_selector.h"
#include "inc/lut_builder.h"
#include "inc/blif_writer.h"

int main(int argc, char* argv[]) {
    std::string inputPath, outputPath;
    int k = 4;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if      (arg == "-input"  && i + 1 < argc) inputPath  = argv[++i];
        else if (arg == "-output" && i + 1 < argc) outputPath = argv[++i];
        else if (arg == "-k"      && i + 1 < argc) k          = std::stoi(argv[++i]);
    }

    if (inputPath.empty() || outputPath.empty()) {
        std::cerr << "Usage: ./114521123_PA2 -input <in.blif> -output <out.blif> -k <2~10>\n";
        return 1;
    }

    // Step 1: Parse
    BlifParser parser;
    if (!parser.parse(inputPath)) return 1;
    parser.print();

    // Step 2: Build AIG
    AigBuilder aig_builder;
    if (!aig_builder.build(parser.getNetwork())) return 1;
    aig_builder.print();

    // Step 3: Cut enumeration (Phase 1), depth mapping (Phase 2), area recovery (Phase 3)
    CutSelector cuts(k);
    cuts.enumerateCuts(aig_builder.getAig());
    cuts.depthMapping(aig_builder.getAig());
    cuts.areaRecovery(aig_builder.getAig());
    cuts.print();

    // Step 4: LUT truth-table extraction
    LutBuilder luts(k);
    luts.buildLuts(aig_builder.getAig(), cuts.getBestCuts());
    luts.print();

    // Step 5: Write output BLIF
    BlifWriter writer;
    if (!writer.write(outputPath,
                      parser.getNetwork().modelName,
                      parser.getNetwork().primaryInputs,
                      parser.getNetwork().primaryOutputs,
                      luts.getLuts())) return 1;
    writer.print();

    return 0;
}
