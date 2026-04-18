#include "../inc/blif_writer.h"
#include <fstream>
#include <iostream>

bool BlifWriter::write(const std::string&              filePath,
                       const std::string&              modelName,
                       const std::vector<std::string>& pis,
                       const std::vector<std::string>& pos,
                       const std::vector<Lut>&         luts) {
    // Store for print()
    lastModel_ = modelName;
    lastPis_   = pis;
    lastPos_   = pos;
    lastLuts_  = luts;

    // TODO: write to file
    (void)filePath;
    return true;
}

void BlifWriter::print() const {
    std::cout << ".model " << lastModel_ << "\n";

    std::cout << ".inputs";
    for (const auto& s : lastPis_)  std::cout << " " << s;
    std::cout << "\n";

    std::cout << ".outputs";
    for (const auto& s : lastPos_)  std::cout << " " << s;
    std::cout << "\n\n";

    for (const Lut& lut : lastLuts_) {
        std::cout << ".names";
        for (const auto& s : lut.inputs) std::cout << " " << s;
        std::cout << " " << lut.output << "\n";
        for (const SopTerm& t : lut.terms) {
            for (int v : t.pattern)
                std::cout << (v == 1 ? '1' : v == 0 ? '0' : '-');
            std::cout << " " << t.onset << "\n";
        }
    }
    std::cout << ".end\n";
}
