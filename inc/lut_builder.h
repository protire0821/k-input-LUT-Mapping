#ifndef LUT_BUILDER_H
#define LUT_BUILDER_H

#include <vector>
#include <unordered_map>
#include "types.h"

class LutBuilder {
public:
    explicit LutBuilder(int k) : k_(k) {}
    ~LutBuilder() = default;

    // poNames: primary-output names in the same order as aig.primaryOutputs.
    // Needed so that outputs which merely alias a PI / an inverted signal /
    // a constant still get a (buffer / inverter / constant) LUT.
    void buildLuts(const Aig& aig,
                   const std::unordered_map<NodeId, Cut>& bestCuts,
                   const std::vector<std::string>& poNames = {});

    void print() const;

    const std::vector<Lut>& getLuts() const { return luts_; }

private:
    int              k_;
    std::vector<Lut> luts_;
};

#endif // LUT_BUILDER_H
