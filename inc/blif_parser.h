#ifndef BLIF_PARSER_H
#define BLIF_PARSER_H

#include <string>
#include "types.h"

class BlifParser {
public:
    BlifParser() = default;
    ~BlifParser() = default;

    // Parse BLIF file. Returns true on success.
    bool parse(const std::string& filePath);

    // Accessor for parsed network.
    const BlifNetwork& getNetwork() const { return network_; }

private:
    BlifNetwork network_;

    // --- Helpers (to be implemented) ---
    // void handleLineContinuations(...);
    // void stripComments(...);
    // void parseModel(...);
    // void parseInputs(...);
    // void parseOutputs(...);
    // void parseNames(...);
};

#endif // BLIF_PARSER_H
