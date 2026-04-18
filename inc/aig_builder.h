// ============================================================================
// inc/aig_builder.h
// Builds an And-Inverter Graph (AIG) from a parsed BLIF network.
// Each complex .names block is decomposed into 2-input AND nodes + inverters.
// ============================================================================
#ifndef AIG_BUILDER_H
#define AIG_BUILDER_H

#include "types.h"

class AigBuilder {
public:
    AigBuilder() = default;
    ~AigBuilder() = default;

    // Build AIG from a BlifNetwork. Returns true on success.
    bool build(const BlifNetwork& network);

    // Accessor for the built AIG.
    const Aig& getAig() const { return aig_; }

private:
    Aig aig_;

    // --- Helpers (to be implemented) ---
    // AigLit buildConst0();
    // AigLit buildPi(const std::string& name);
    // AigLit buildAnd(AigLit a, AigLit b);
    // AigLit buildSop(const NamesBlock& block);          // convert SOP -> AIG
    // AigLit buildProductTerm(...);                      // one product row
};

#endif // AIG_BUILDER_H
