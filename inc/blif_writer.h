// ============================================================================
// inc/blif_writer.h
// Writes the final k-input LUT mapping result out as a BLIF file.
// ============================================================================
#ifndef BLIF_WRITER_H
#define BLIF_WRITER_H

#include <string>
#include <vector>
#include "types.h"

class BlifWriter {
public:
    BlifWriter() = default;
    ~BlifWriter() = default;

    // Write a BLIF file. Returns true on success.
    //   modelName : .model name (must match input)
    //   pis/pos   : primary inputs / outputs (must match input order)
    //   luts      : .names blocks to emit
    bool write(const std::string&              filePath,
               const std::string&              modelName,
               const std::vector<std::string>& pis,
               const std::vector<std::string>& pos,
               const std::vector<Lut>&         luts);

private:
    // --- Helpers (to be implemented) ---
    // void emitHeader(...);
    // void emitNames(const Lut& lut, std::ostream& os);
};

#endif // BLIF_WRITER_H
