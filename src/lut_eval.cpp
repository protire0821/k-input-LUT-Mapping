#include "lut_eval.h"

#include <iostream>

void LutEval::enumerateCuts(const Aig& aig) {
    // TODO:
    // For each AIG node in topological order:
    //   - If PI or CONST: trivial cut { {self} }.
    //   - If AND(a,b): merge cuts of a and b, keep only cuts with |leaves| <= k_,
    //                  drop dominated cuts, append trivial {self} cut.
    (void)aig;
}

void LutEval::selectCuts(const Aig& aig) {
    // TODO:
    // Choose cut per node to minimize Cost = Level * |LUTs|.
    // Typical flow:
    //   1. Forward pass: compute best cut per node w.r.t. some heuristic
    //      (area-flow / depth-optimal / area-oriented).
    //   2. Backward traversal from POs: mark required cuts -> luts_.
    //   3. Fill luts_ with truth tables + input/output names.
    (void)aig;
}
