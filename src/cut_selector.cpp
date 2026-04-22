#include "../inc/cut_selector.h"
#include <iostream>
#include <algorithm>
#include <climits>
#include <limits>


// ─── Phase 1: Cut Enumeration ───
void CutSelector::enumerateCuts(const Aig& aig) {
    nodeCuts_.clear();

    // start from low order AND
    for (const AigNode& node : aig.nodes) {
        NodeId id   = node.id;
        auto&  cuts = nodeCuts_[id];

        // 1. Trivial cut itself
        cuts.push_back(Cut{{id}});

        if (node.type != AigNodeType::AND) continue;

        NodeId f0 = node.fanin0 >> 1; 
        NodeId f1 = node.fanin1 >> 1;

        for (const Cut& c0 : nodeCuts_[f0]) { 
            for (const Cut& c1 : nodeCuts_[f1]) { 

                // 2. Merge c0 c1 leaves
                std::vector<NodeId> merged_cuts;
                merged_cuts.reserve(c0.leaves.size() + c1.leaves.size());
                std::merge(c0.leaves.begin(), c0.leaves.end(),
                           c1.leaves.begin(), c1.leaves.end(), std::back_inserter(merged_cuts));
                merged_cuts.erase(std::unique(merged_cuts.begin(), merged_cuts.end()), merged_cuts.end()); // remove duplicates

                // 3. kick out cuts over k leaves
                if ((int)merged_cuts.size() > k_) continue;

                Cut newCut{std::move(merged_cuts)};

                // 4. deminance check: check newCut is dominated by others or not
                bool dominated = false;
                for (const Cut& existing : cuts) {
                    if (existing.leaves.size() <= newCut.leaves.size() &&
                        std::includes(newCut.leaves.begin(), newCut.leaves.end(),  // existing is in newCut
                                      existing.leaves.begin(), existing.leaves.end())) {
                        dominated = true;   // newCut won't be better, e.g. {1, 2} dominates {1}
                        break;
                    }
                }
                if (dominated) continue;

                // 5. Dominance prune: remove cuts dominated by newCut
                auto isDominatedByNewCut = [&](const Cut& existing) {   // cuts sent in for existing node
                    if (newCut.leaves.size() > existing.leaves.size()) return false;
                    return std::includes(existing.leaves.begin(), existing.leaves.end(),
                                         newCut.leaves.begin(), newCut.leaves.end());
                };
                
                // remove_if: delete nodes moved to end and return new iterater end
                cuts.erase(std::remove_if(cuts.begin(), cuts.end(), isDominatedByNewCut), cuts.end());

                // 6. Insert newCut to cuts (nodeCuts_[id])
                cuts.push_back(std::move(newCut)); 
            }
        }
    }
}


// ─── Phase 2: Depth-Oriented Mapping ───
// Assume AND node has delay 1, PI/CONST0 has delay 0.
void CutSelector::depthMapping(const Aig& aig) {
    bestCut_.clear();
    arrTime_.clear();

    for (const AigNode& node : aig.nodes) {  // by id order small to large
        NodeId id = node.id;

        if (node.type != AigNodeType::AND) {
            arrTime_[id] = 0; 
            bestCut_[id] = nodeCuts_.count(id) ? nodeCuts_.at(id)[0] : Cut{{id}};
            continue;
        }

        int        bestLv    = INT_MAX;   // max -> min
        const Cut* bestCut_p = nullptr;   // pointer to best cut

        for (const Cut& cut : nodeCuts_.at(id)) { 
            if (cut.leaves.size() == 1 && cut.leaves[0] == id) continue;  // skip trivial cut

            int lv = 0;
            for (NodeId leaf : cut.leaves) lv = std::max(lv, arrTime_[leaf]); // max leaf level
            ++lv;                                                             // plus 1 LUT level（self）

            if (lv < bestLv) { bestLv = lv; bestCut_p = &cut; } 
        }

        bestCut_[id] = *bestCut_p;
        arrTime_[id] = bestLv;
    }

    depth_ = 0;
    for (AigLit poLit : aig.primaryOutputs) {
        NodeId poId = poLit >> 1;
        if (arrTime_.count(poId))
            depth_ = std::max(depth_, arrTime_.at(poId));
    }
}


// ─── Phase 3: Area Recovery ───
// Avoid increasing the depth
void CutSelector::areaRecovery(const Aig& aig) {

    // 1. Cal AIG structural fanout 
    std::unordered_map<NodeId, int> fanout;
    for (const AigNode& n : aig.nodes) fanout[n.id] = 0;
    for (const AigNode& n : aig.nodes) {
        if (n.type != AigNodeType::AND) continue;
        fanout[n.fanin0 >> 1]++; 
        fanout[n.fanin1 >> 1]++; 
    }
    for (AigLit poLit : aig.primaryOutputs)
        fanout[poLit >> 1]++;      // PO fanout

    // 2. Find required nodes through best cuts from PO (backward)
    std::unordered_set<NodeId> required;
    std::vector<NodeId> worklist;           // DFS stack

    for (AigLit poLit : aig.primaryOutputs) {
        NodeId poId = poLit >> 1;  
        if (poId < (NodeId)aig.nodes.size() && aig.nodes[poId].type == AigNodeType::AND && !required.count(poId)) {                                   // 避免重複加入
            required.insert(poId);
            worklist.push_back(poId);
        }
    }

    while (!worklist.empty()) {
        NodeId id = worklist.back(); 
        worklist.pop_back();

        auto id_it = bestCut_.find(id);
        for (NodeId leaf : id_it->second.leaves) {  // go through best cut leaves
            if (aig.nodes[leaf].type == AigNodeType::AND && !required.count(leaf)) { 
                required.insert(leaf);
                worklist.push_back(leaf);   // keep backward until PI/CONST0
            }
        }
    }

    // 3. Required time backward pass
    reqTime_.clear();
    for (const AigNode& n : aig.nodes) reqTime_[n.id] = depth_;     // latest depth_

    for (auto node_it = aig.nodes.rbegin(); node_it != aig.nodes.rend(); ++node_it) {
        const AigNode& node = *node_it;
        if (node.type != AigNodeType::AND || !required.count(node.id)) continue;  // only required AND nodes

        auto cut_it = bestCut_.find(node.id);
        if (cut_it == bestCut_.end()) continue;

        for (NodeId leaf : cut_it->second.leaves)
            reqTime_[leaf] = std::min(reqTime_[leaf], reqTime_.at(node.id) - 1);  // tighten required time
    }

    // 4. Cal initial area flow (Phase 2 cut)
    //   areaFlow(PI) = 0
    //   areaFlow(v)  = (1 + SUM[areaFlow(leaf)]) / fanout(v)
    areaFlow_.clear();
    for (const AigNode& node : aig.nodes) {
        NodeId id = node.id;
        if (node.type != AigNodeType::AND || !required.count(id)) {
            areaFlow_[id] = 0.0;
            continue;
        }
        auto cut_it = bestCut_.find(id);
        if (cut_it == bestCut_.end()) { areaFlow_[id] = 1.0; continue; }  // no cut to be 1

        double flow = 1.0;                                             // self is 1
        for (NodeId leaf : cut_it->second.leaves)
            flow += areaFlow_.count(leaf) ? areaFlow_.at(leaf) : 0.0;

        int fo = std::max(1, fanout.count(id) ? fanout.at(id) : 1);   // fanout at least 1
        areaFlow_[id] = flow / fo; 
    }

    // 5. Area recovery: update bestCut_ and areaFlow_ if find better cut
    for (const AigNode& node : aig.nodes) {
        NodeId id = node.id;
        if (node.type != AigNodeType::AND || !required.count(id)) continue;

        int nodeReqTime = reqTime_.at(id); 

        double     bestFlow   = std::numeric_limits<double>::max();
        const Cut* bestCut_p = nullptr; 

        for (const Cut& c : nodeCuts_.at(id)) {
            if (c.leaves.size() == 1 && c.leaves[0] == id) continue;

            // Depth constraint：arrTime < reqTime
            bool feasible = true;
            for (NodeId leaf : c.leaves) {
                int arr = arrTime_.count(leaf) ? arrTime_.at(leaf) : 0;
                if (arr >= nodeReqTime) { feasible = false; break; }   // violate constraint
            }
            if (!feasible) continue;

            // area flow
            double flow = 1.0;   // self 1 LUT
            for (NodeId leaf : c.leaves)
                flow += areaFlow_.count(leaf) ? areaFlow_.at(leaf) : 0.0;  // add leaf's area flow

            if (flow < bestFlow) { bestFlow = flow; bestCut_p = &c; }  // upadte best cut
        }

        if (bestCut_p) {
            bestCut_[id] = *bestCut_p; 
            int fo = std::max(1, fanout.count(id) ? fanout.at(id) : 1);
            areaFlow_[id] = bestFlow / fo;
        }
        // keep original cut if no better cut (bestCut_p is nullptr)
    }
}


void CutSelector::print() const {
    std::cout << "\n======Cut Selection Results======" << std::endl;

    std::cout << "CutSelector (k=" << k_ << ", depth=" << depth_ << ")\n";
    std::cout << "  Nodes with cuts : " << nodeCuts_.size() << "\n";
    std::cout << "  Best cuts chosen: " << bestCut_.size()  << "\n\n";

    for (const auto& [id, best] : bestCut_) {
        if (best.leaves.size() == 1 && best.leaves[0] == id) continue;

        std::cout << "  Node[" << id << "]"
                  << " arr=" << (arrTime_.count(id) ? arrTime_.at(id) : -1)
                  << " req=" << (reqTime_.count(id) ? reqTime_.at(id) : -1)
                  << " flow=" << (areaFlow_.count(id) ? areaFlow_.at(id) : -1.0)
                  << "  leaves:{";
        for (size_t i = 0; i < best.leaves.size(); ++i)
            std::cout << best.leaves[i] << (i + 1 < best.leaves.size() ? "," : "");
        std::cout << "}\n";
    }
}
