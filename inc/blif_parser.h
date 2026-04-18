#ifndef BLIF_PARSER_H
#define BLIF_PARSER_H

#include <string>
#include <fstream>
#include <vector>
#include "types.h"

class BlifParser {
public:
    BlifParser() = default;
    ~BlifParser() = default;

    bool parse(const std::string& filePath);
    void print() const;

    const BlifNetwork& getNetwork() const { return network_; }

private:
    BlifNetwork network_;

    static void stripComment(std::string& line);
    static bool readLogicalLine(std::ifstream& f, std::string& out);
    static void tokenize(const std::string& line, std::vector<std::string>& tokens);
};

#endif // BLIF_PARSER_H
