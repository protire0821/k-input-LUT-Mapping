#ifndef CUT_SELECTOR_H
#define CUT_SELECTOR_H

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include "types.h"

// Three-phase k-LUT mapping:
//   Phase 1 – enumerateCuts : build all k-feasible cuts per AIG node
//   Phase 2 – depthMapping  : choose cuts minimising arrival time (depth)
//   Phase 3 – areaRecovery  : re-choose cuts minimising area flow while
//                             keeping arrival time within required time
class CutSelector {
public:
    explicit CutSelector(int k) : k_(k) {}

    void enumerateCuts(const Aig& aig);  // Phase 1
    void depthMapping (const Aig& aig);  // Phase 2
    void areaRecovery (const Aig& aig);  // Phase 3

    void print() const;

    const std::unordered_map<NodeId, std::vector<Cut>>& getNodeCuts() const { return nodeCuts_; }
    const std::unordered_map<NodeId, Cut>&              getBestCuts() const { return bestCut_;  }
    int  getDepth() const { return depth_; }

private:
    int k_;
    int depth_ = 0;

    std::unordered_map<NodeId, std::vector<Cut>> nodeCuts_;
    std::unordered_map<NodeId, Cut>              bestCut_;
    std::unordered_map<NodeId, int>              arrTime_;
    std::unordered_map<NodeId, int>              reqTime_;
    std::unordered_map<NodeId, double>           areaFlow_;
};

#endif // CUT_SELECTOR_H
