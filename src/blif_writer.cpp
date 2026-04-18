#include "blif_writer.h"

#include <fstream>
#include <iostream>

bool BlifWriter::write(const std::string&              filePath,
                       const std::string&              modelName,
                       const std::vector<std::string>& pis,
                       const std::vector<std::string>& pos,
                       const std::vector<Lut>&         luts) {
    // TODO:
    // 1. Open output file.
    // 2. Emit header:
    //      .model <modelName>
    //      .inputs <pis...>
    //      .outputs <pos...>
    // 3. For each Lut:
    //      .names <inputs...> <output>
    //      <SOP rows>
    // 4. Emit .end
    (void)filePath; (void)modelName; (void)pis; (void)pos; (void)luts;
    return true;
}
