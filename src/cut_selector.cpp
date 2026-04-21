#include "../inc/cut_selector.h"
#include <iostream>
#include <algorithm>
#include <climits>
#include <limits>

// ─── static helpers ──────────────────────────────────────────────────────────

static std::vector<NodeId> mergeLeaves(const std::vector<NodeId>& a,
                                        const std::vector<NodeId>& b) {
    std::vector<NodeId> out;
    out.reserve(a.size() + b.size());
    std::merge(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(out));
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

// Cut A dominates Cut B iff A.leaves ⊆ B.leaves.
static bool dominates(const Cut& a, const Cut& b) {
    if (a.leaves.size() > b.leaves.size()) return false;
    return std::includes(b.leaves.begin(), b.leaves.end(),
                         a.leaves.begin(), a.leaves.end());
}

static void addCut(std::vector<Cut>& cuts, Cut newCut) {
    for (const Cut& c : cuts)
        if (dominates(c, newCut)) return;
    cuts.erase(std::remove_if(cuts.begin(), cuts.end(),
        [&](const Cut& c){ return dominates(newCut, c); }), cuts.end());
    cuts.push_back(std::move(newCut));
}

// ─── Phase 1: Cut Enumeration ────────────────────────────────────────────────

void CutSelector::enumerateCuts(const Aig& aig) {
    nodeCuts_.clear();

    for (const AigNode& node : aig.nodes) {
        NodeId id   = node.id;
        auto&  cuts = nodeCuts_[id];

        cuts.push_back(Cut{{id}});  // trivial cut

        if (node.type != AigNodeType::AND) continue;

        NodeId f0 = node.fanin0 >> 1;   // get node id
        NodeId f1 = node.fanin1 >> 1;

        for (const Cut& c0 : nodeCuts_[f0]) {
            for (const Cut& c1 : nodeCuts_[f1]) {
                auto merged = mergeLeaves(c0.leaves, c1.leaves);
                if ((int)merged.size() > k_) continue;
                addCut(cuts, Cut{std::move(merged)});
            }
        }
    }
}

// ─── Phase 2: Depth-Oriented Mapping ─────────────────────────────────────────
//
// Forward pass: pick the cut that minimises arrival time at each AND node.
// arrTime_ is saved for Phase 3.

void CutSelector::depthMapping(const Aig& aig) {
    bestCut_.clear();
    arrTime_.clear();

    for (const AigNode& node : aig.nodes) {
        NodeId id = node.id;

        if (node.type != AigNodeType::AND) {
            arrTime_[id] = 0;
            bestCut_[id] = nodeCuts_.count(id) ? nodeCuts_.at(id)[0] : Cut{{id}};
            continue;
        }

        int        bestLv = INT_MAX;
        const Cut* bp     = nullptr;

        for (const Cut& c : nodeCuts_.at(id)) {
            if (c.leaves.size() == 1 && c.leaves[0] == id) continue;  // trivial

            int lv = 0;
            for (NodeId leaf : c.leaves) lv = std::max(lv, arrTime_[leaf]);
            ++lv;

            if (lv < bestLv) { bestLv = lv; bp = &c; }
        }

        if (bp) {
            bestCut_[id] = *bp;
            arrTime_[id] = bestLv;
        } else {
            bestCut_[id] = nodeCuts_.at(id)[0];
            arrTime_[id] = 1;
        }
    }

    // Record circuit depth (max arrival time across PO nodes).
    depth_ = 0;
    for (AigLit poLit : aig.primaryOutputs) {
        NodeId nd = poLit >> 1;
        if (arrTime_.count(nd))
            depth_ = std::max(depth_, arrTime_.at(nd));
    }
}

// ─── Phase 3: Area Recovery ───────────────────────────────────────────────────
//
// Re-select cuts to minimise area flow while keeping arrTime ≤ reqTime.
//
// area flow:  areaFlow(PI) = 0
//             areaFlow(v)  = (1 + Σ areaFlow(leaf) for cut(v)) / fanout(v)
//
// required time backward pass:
//   reqTime[PO]   = depth_
//   reqTime[leaf] = min(reqTime[leaf], reqTime[parent] - 1)

void CutSelector::areaRecovery(const Aig& aig) {
    // ── AIG structural fanout ────────────────────────────────────────────────
    std::unordered_map<NodeId, int> fanout;
    for (const AigNode& n : aig.nodes) fanout[n.id] = 0;
    for (const AigNode& n : aig.nodes) {
        if (n.type != AigNodeType::AND) continue;
        fanout[n.fanin0 >> 1]++;
        fanout[n.fanin1 >> 1]++;
    }
    for (AigLit poLit : aig.primaryOutputs)
        fanout[poLit >> 1]++;

    // ── Required nodes (backward from POs, using Phase 2 bestCut_) ──────────
    std::unordered_set<NodeId> required;
    std::vector<NodeId>        worklist;

    for (AigLit poLit : aig.primaryOutputs) {
        NodeId nd = poLit >> 1;
        if (nd < (NodeId)aig.nodes.size() &&
            aig.nodes[nd].type == AigNodeType::AND && !required.count(nd)) {
            required.insert(nd);
            worklist.push_back(nd);
        }
    }
    while (!worklist.empty()) {
        NodeId id = worklist.back(); worklist.pop_back();
        auto it = bestCut_.find(id);
        if (it == bestCut_.end()) continue;
        for (NodeId leaf : it->second.leaves) {
            if (aig.nodes[leaf].type == AigNodeType::AND && !required.count(leaf)) {
                required.insert(leaf);
                worklist.push_back(leaf);
            }
        }
    }

    // ── Required time backward pass ──────────────────────────────────────────
    reqTime_.clear();
    for (const AigNode& n : aig.nodes) reqTime_[n.id] = depth_;

    for (auto it = aig.nodes.rbegin(); it != aig.nodes.rend(); ++it) {
        const AigNode& node = *it;
        if (node.type != AigNodeType::AND || !required.count(node.id)) continue;
        auto cut_it = bestCut_.find(node.id);
        if (cut_it == bestCut_.end()) continue;
        int rt = reqTime_.at(node.id);
        for (NodeId leaf : cut_it->second.leaves)
            reqTime_[leaf] = std::min(reqTime_[leaf], rt - 1);
    }

    // ── Initial area flow (topological order, Phase 2 cuts) ─────────────────
    areaFlow_.clear();
    for (const AigNode& node : aig.nodes) {
        NodeId id = node.id;
        if (node.type != AigNodeType::AND || !required.count(id)) {
            areaFlow_[id] = 0.0;
            continue;
        }
        auto it = bestCut_.find(id);
        if (it == bestCut_.end()) { areaFlow_[id] = 1.0; continue; }

        double flow = 1.0;
        for (NodeId leaf : it->second.leaves)
            flow += areaFlow_.count(leaf) ? areaFlow_.at(leaf) : 0.0;

        int fo = std::max(1, fanout.count(id) ? fanout.at(id) : 1);
        areaFlow_[id] = flow / fo;
    }

    // ── Area recovery forward pass ───────────────────────────────────────────
    for (const AigNode& node : aig.nodes) {
        NodeId id = node.id;
        if (node.type != AigNodeType::AND || !required.count(id)) continue;

        int nodeReqTime = reqTime_.at(id);

        double     bestFlow    = std::numeric_limits<double>::max();
        const Cut* bestCutPtr  = nullptr;

        for (const Cut& c : nodeCuts_.at(id)) {
            if (c.leaves.size() == 1 && c.leaves[0] == id) continue;  // trivial

            // Depth constraint: every leaf must arrive strictly before reqTime.
            bool feasible = true;
            for (NodeId leaf : c.leaves) {
                int arr = arrTime_.count(leaf) ? arrTime_.at(leaf) : 0;
                if (arr >= nodeReqTime) { feasible = false; break; }
            }
            if (!feasible) continue;

            double flow = 1.0;
            for (NodeId leaf : c.leaves)
                flow += areaFlow_.count(leaf) ? areaFlow_.at(leaf) : 0.0;

            if (flow < bestFlow) { bestFlow = flow; bestCutPtr = &c; }
        }

        if (bestCutPtr) {
            bestCut_[id] = *bestCutPtr;
            int fo = std::max(1, fanout.count(id) ? fanout.at(id) : 1);
            areaFlow_[id] = bestFlow / fo;
        }
    }
}

// ─── print ───────────────────────────────────────────────────────────────────

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
