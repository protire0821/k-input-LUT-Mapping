#ifndef LUT_EVAL_H
#define LUT_EVAL_H

#include <vector>
#include <unordered_map>
#include "types.h"

class LutEval {
public:
    explicit LutEval(int k) : k_(k) {}
    ~LutEval() = default;

    void enumerateCuts(const Aig& aig);
    void selectCuts   (const Aig& aig);
    void print() const;

    const std::vector<Lut>& getLuts() const { return luts_; }

private:
    int k_;
    std::unordered_map<NodeId, std::vector<Cut>> nodeCuts_;
    std::unordered_map<NodeId, Cut>              bestCut_;
    std::vector<Lut>                             luts_;
};

#endif // LUT_EVAL_H
