// ============================================================================
// inc/lut_eval.h
// k-input cut enumeration + cut selection + LUT truth-table generation.
// ============================================================================
#ifndef LUT_EVAL_H
#define LUT_EVAL_H

#include <vector>
#include <unordered_map>
#include "types.h"

class LutEval {
public:
    explicit LutEval(int k) : k_(k) {}
    ~LutEval() = default;

    // Step 1: enumerate all feasible k-input cuts for every AIG node.
    void enumerateCuts(const Aig& aig);

    // Step 2: pick a set of cuts that covers the circuit outputs
    //         (minimizing LUT count / level, i.e. Cost = Level * |LUTs|).
    void selectCuts(const Aig& aig);

    // Step 3: build the final LUT list (with truth tables in SOP form)
    //         to be fed to the BLIF writer.
    const std::vector<Lut>& getLuts() const { return luts_; }

private:
    int k_;
    // Per-node cut list
    std::unordered_map<NodeId, std::vector<Cut>> nodeCuts_;
    // Selected best cut per node
    std::unordered_map<NodeId, Cut>              bestCut_;
    // Final LUTs (inputs/outputs refer to signal names used in output BLIF)
    std::vector<Lut>                             luts_;

    // --- Helpers (to be implemented) ---
    // std::vector<Cut> mergeCuts(const std::vector<Cut>& a,
    //                            const std::vector<Cut>& b);
    // bool             isDominated(const Cut& c, const std::vector<Cut>& set);
    // std::vector<SopTerm> computeTruthTable(const Aig& aig, NodeId root,
    //                                        const Cut& cut);
};

#endif // LUT_EVAL_H
