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
#include "inc/lut_eval.h"
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

    // TODO: Step 2. Build AIG
    // AigBuilder builder;
    // builder.build(parser.getNetwork());

    // TODO: Step 3. LUT Mapping
    // LutEval eval(k);
    // eval.enumerateCuts(builder.getAig());
    // eval.selectCuts(builder.getAig());

    // TODO: Step 4. Write output BLIF
    // BlifWriter writer;
    // writer.write(outputPath, ...);

    (void)k;
    return 0;
}
