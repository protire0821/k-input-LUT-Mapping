#include "../inc/blif_writer.h"
#include <fstream>
#include <iostream>

// ─── static helpers ──────────────────────────────────────────────────────────

// Write a BLIF directive line (e.g. ".inputs a b c ...") with '\' continuation
// when the line would exceed `wrap` characters.
static void writeTokenLine(std::ofstream& f,
                            const std::string& directive,
                            const std::vector<std::string>& tokens,
                            int wrap = 80) {
    std::string line = directive;
    for (const std::string& tok : tokens) {
        // +1 for the space, +2 for potential " \" suffix
        if ((int)(line.size() + 1 + tok.size()) > wrap - 2 && line != directive) {
            f << line << " \\\n";
            line = " " + tok; // indent continuation
        } else {
            line += ' ';
            line += tok;
        }
    }
    f << line << '\n';
}

// ─── BlifWriter::write ───────────────────────────────────────────────────────

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

    std::ofstream f(filePath);
    if (!f.is_open()) {
        std::cerr << "[BlifWriter] Cannot open for writing: " << filePath << "\n";
        return false;
    }

    // Header
    f << ".model " << modelName << '\n';
    writeTokenLine(f, ".inputs",  pis);
    writeTokenLine(f, ".outputs", pos);
    f << '\n';

    // LUT blocks
    for (const Lut& lut : luts) {
        // .names <inputs...> <output>
        std::vector<std::string> nameTokens = lut.inputs;
        nameTokens.push_back(lut.output);
        writeTokenLine(f, ".names", nameTokens);

        // SOP rows
        for (const SopTerm& t : lut.terms) {
            // Pattern string (empty if 0-input constant LUT)
            for (int v : t.pattern)
                f << (v == 1 ? '1' : v == 0 ? '0' : '-');
            if (!t.pattern.empty()) f << ' ';
            f << t.onset << '\n';
        }
    }

    f << ".end\n";
    return true;
}

// ─── BlifWriter::print ───────────────────────────────────────────────────────

void BlifWriter::print() const {
    std::cout << "\n======BLIF Write Results======" << std::endl;

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
            if (!t.pattern.empty()) std::cout << ' ';
            std::cout << t.onset << "\n";
        }
    }
    std::cout << ".end\n";
}
