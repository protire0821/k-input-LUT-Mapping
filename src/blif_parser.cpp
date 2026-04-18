#include "blif_parser.h"

#include <fstream>
#include <sstream>
#include <iostream>

bool BlifParser::parse(const std::string& filePath) {
    // TODO:
    // 1. Open file.
    // 2. Read line-by-line, handling:
    //    - '#' comments -> strip
    //    - backslash '\' line continuation -> join with next line
    //    - empty / whitespace-only lines -> skip
    // 3. Dispatch on directive:
    //    - .model   -> network_.modelName
    //    - .inputs  -> network_.primaryInputs
    //    - .outputs -> network_.primaryOutputs
    //    - .names   -> push a new NamesBlock; following non-directive
    //                  lines are SOP rows until the next directive
    //    - .end     -> stop
    (void)filePath;
    return true;
}
