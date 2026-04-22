#include "../inc/blif_parser.h"

#include <fstream>
#include <sstream>
#include <iostream>

// Strip everything from '#' onward.
void BlifParser::stripComment(std::string& line) {
    auto pos = line.find('#');
    if (pos != std::string::npos)
        line.erase(pos);
}

// Read one logical line from the file, joining '\'-continued physical lines.
// Returns false when EOF is reached with no content.
bool BlifParser::readLogicalLine(std::ifstream& in_f, std::string& out_f) {
    out_f.clear();
    std::string raw;

    while (std::getline(in_f, raw)) {
        stripComment(raw);

        // Trim trailing whitespace (including '\r' on Windows).
        while (!raw.empty() && (raw.back() == ' ' || raw.back() == '\t' || raw.back() == '\r'))
            raw.pop_back();

        if (!raw.empty() && raw.back() == '\\') {
            raw.pop_back();  // remove backslash
            out_f += raw;
            out_f += ' ';      // separate tokens across joined lines
            continue;
        }

        out_f += raw;

        // Skip blank logical lines; keep reading until we get content.
        if (out_f.find_first_not_of(" \t") == std::string::npos) {
            out_f.clear();
            continue;
        }
        return true;
    }

    return !out_f.empty();  // flush final line if continuation ended at EOF
}

// Split a line into whitespace-delimited tokens.
void BlifParser::tokenize(const std::string& line, std::vector<std::string>& tokens) {
    tokens.clear();
    const char* p = line.c_str();
    while (*p) {
        while (*p == ' ' || *p == '\t') ++p;
        if (!*p) break;
        const char* start = p;
        while (*p && *p != ' ' && *p != '\t') ++p;
        tokens.emplace_back(start, p);
    }
}

bool BlifParser::parse(const std::string& filePath) {
    std::ifstream f(filePath);
    if (!f.is_open()) {
        std::cerr << "[BlifParser] Cannot open: " << filePath << "\n";
        return false;
    }

    network_ = BlifNetwork{};  // reset

    std::string line;
    std::vector<std::string> tokens;
    int currentIdx = -1;  // index into network_.namesBlocks; avoids dangling ptr on realloc

    while (readLogicalLine(f, line)) {  // read in lines
        tokenize(line, tokens);
        if (tokens.empty()) continue;

        const std::string& dir = tokens[0];
        if (dir == ".model") {
            if (tokens.size() >= 2)
                network_.modelName = tokens[1];

        } else if (dir == ".inputs") {
            for (size_t i = 1; i < tokens.size(); ++i)
                network_.primaryInputs.push_back(tokens[i]);

        } else if (dir == ".outputs") {
            for (size_t i = 1; i < tokens.size(); ++i)
                network_.primaryOutputs.push_back(tokens[i]);

        } else if (dir == ".names") {
            // tokens: .names i0 i1 ... iN-1 out
            // last token is the output, everything in between are inputs
            network_.namesBlocks.emplace_back();
            currentIdx = static_cast<int>(network_.namesBlocks.size()) - 1; 

            for (size_t i = 1; i + 1 < tokens.size(); ++i)
                network_.namesBlocks[currentIdx].inputs.push_back(tokens[i]);
            if (tokens.size() >= 2)
                network_.namesBlocks[currentIdx].output = tokens.back();

        } else if (dir == ".end") {
            break;

        } else if (!dir.empty() && dir[0] == '.') {
            return false;  // unrecognized directive
        } else {
            // SOP row: belongs to the current .names block.
            if (currentIdx < 0) {
                std::cerr << "[BlifParser] SOP row before .names: " << line << "\n";
                return false;
            }
            SopTerm term;
            for (char c : tokens[0]) {
                if      (c == '1') term.pattern.push_back( 1);
                else if (c == '0') term.pattern.push_back( 0);
                else if (c == '-') term.pattern.push_back(-1);
                else return false;
            }
            term.onset = (tokens.size() >= 2) ? (tokens[1][0] - '0') : 1;
            network_.namesBlocks[currentIdx].terms.push_back(term);
        }
    }

    return true;
}


void BlifParser::print() const {
    const BlifNetwork& net = network_;
    std::cout << "\n======BLIF Parse Results======" << std::endl;

    std::cout << ".model " << net.modelName << "\n";

    std::cout << ".inputs";
    for (const auto& s : net.primaryInputs)  std::cout << " " << s;
    std::cout << "\n";

    std::cout << ".outputs";
    for (const auto& s : net.primaryOutputs) std::cout << " " << s;
    std::cout << "\n\n";

    std::cout << "Blocks: " << net.namesBlocks.size() << "\n";
    for (size_t bi = 0; bi < net.namesBlocks.size(); ++bi) {
        const NamesBlock& b = net.namesBlocks[bi];
        std::cout << "  [" << bi << "] (";
        for (size_t i = 0; i < b.inputs.size(); ++i)
            std::cout << b.inputs[i] << (i + 1 < b.inputs.size() ? ", " : "");
        std::cout << ") -> " << b.output << "\n";
        for (size_t ti = 0; ti < b.terms.size(); ++ti) {
            const SopTerm& t = b.terms[ti];
            std::cout << "      [" << ti << "] [";
            for (int v : t.pattern) std::cout << v << " ";
            std::cout << "] onset=" << t.onset << "\n";
        }
    }
}
