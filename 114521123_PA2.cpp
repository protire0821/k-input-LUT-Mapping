// ============================================================================
// 114521123_PA2.cpp
// EE6094 CAD for VLSI Design - Programming Assignment 2
// k-input LUT Mapping
// ----------------------------------------------------------------------------
// Main entry point.
// Usage:
//     ./PA2 -input <input.blif> -output <output.blif> -k <number, 2~10>
// ============================================================================

#include <iostream>
#include <string>

#include "types.h"
#include "blif_parser.h"
#include "aig_builder.h"
#include "lut_eval.h"
#include "blif_writer.h"

int main(int argc, char* argv[]) {
    // TODO: parse command line arguments (-input, -output, -k)

    // TODO: Step 1. Parse BLIF
    // BlifParser parser;
    // parser.parse(inputPath);

    // TODO: Step 2. Build AIG from parsed network
    // AigBuilder aigBuilder;
    // aigBuilder.build(parser.getNetwork());

    // TODO: Step 3. k-input LUT Mapping (cut enumeration + selection)
    // LutEval lutEval(k);
    // lutEval.enumerateCuts(aigBuilder.getAig());
    // lutEval.selectCuts(aigBuilder.getAig());

    // TODO: Step 4. Write mapped result back to BLIF
    // BlifWriter writer;
    // writer.write(outputPath, ...);

    (void)argc; (void)argv;
    return 0;
}
