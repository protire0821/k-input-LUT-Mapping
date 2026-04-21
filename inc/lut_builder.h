#ifndef LUT_BUILDER_H
#define LUT_BUILDER_H

#include <vector>
#include <unordered_map>
#include "types.h"

// Given the best-cut map produced by CutSelector, evaluate the truth table for
// each required AND node and assemble the final LUT list.
class LutBuilder {
public:
    explicit LutBuilder(int k) : k_(k) {}
    ~LutBuilder() = default;

    void buildLuts(const Aig& aig,
                   const std::unordered_map<NodeId, Cut>& bestCuts);

    void print() const;

    const std::vector<Lut>& getLuts() const { return luts_; }

private:
    int              k_;
    std::vector<Lut> luts_;
};

#endif // LUT_BUILDER_H
