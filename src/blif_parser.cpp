#include "blif_parser.h"

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
bool BlifParser::readLogicalLine(std::ifstream& f, std::string& out) {
    out.clear();
    std::string raw;

    while (std::getline(f, raw)) {
        stripComment(raw);

        // Trim trailing whitespace (including '\r' on Windows).
        while (!raw.empty() && (raw.back() == ' ' || raw.back() == '\t' || raw.back() == '\r'))
            raw.pop_back();

        if (!raw.empty() && raw.back() == '\\') {
            raw.pop_back();  // remove backslash
            out += raw;
            out += ' ';      // separate tokens across joined lines
            continue;
        }

        out += raw;

        // Skip blank logical lines; keep reading until we get content.
        if (out.find_first_not_of(" \t") == std::string::npos) {
            out.clear();
            continue;
        }
        return true;
    }

    return !out.empty();  // flush final line if continuation ended at EOF
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

    while (readLogicalLine(f, line)) {
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
            NamesBlock& blk = network_.namesBlocks[currentIdx];

            for (size_t i = 1; i + 1 < tokens.size(); ++i)
                blk.inputs.push_back(tokens[i]);
            if (tokens.size() >= 2)
                blk.output = tokens.back();

        } else if (dir == ".end") {
            break;

        } else if (!dir.empty() && dir[0] == '.') {
            // Unknown directive — skip silently.

        } else {
            // SOP row: belongs to the current .names block.
            if (currentIdx < 0) {
                std::cerr << "[BlifParser] SOP row before any .names: " << line << "\n";
                continue;
            }
            SopTerm term;
            term.pattern = tokens[0];
            term.onset   = (tokens.size() >= 2) ? tokens[1][0] : '1';
            network_.namesBlocks[currentIdx].terms.push_back(std::move(term));
        }
    }

    return true;
}
