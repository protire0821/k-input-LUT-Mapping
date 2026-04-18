#ifndef BLIF_WRITER_H
#define BLIF_WRITER_H

#include <string>
#include <vector>
#include "types.h"

class BlifWriter {
public:
    BlifWriter() = default;
    ~BlifWriter() = default;

    bool write(const std::string&              filePath,
               const std::string&              modelName,
               const std::vector<std::string>& pis,
               const std::vector<std::string>& pos,
               const std::vector<Lut>&         luts);

    void print() const;   // print last written content to stdout

private:
    // Store last written data for print()
    std::string              lastModel_;
    std::vector<std::string> lastPis_;
    std::vector<std::string> lastPos_;
    std::vector<Lut>         lastLuts_;
};

#endif // BLIF_WRITER_H
