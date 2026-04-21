#include "../inc/lut_builder.h"
#include <iostream>
#include <algorithm>
#include <unordered_set>

// ─── static helpers ──────────────────────────────────────────────────────────

// Evaluate AIG node `target` given a partial assignment to leaf nodes.
// Traverses nodes in id-order (= topological order) up to `target`.
static int evalNode(const Aig& aig, NodeId target,
                    const std::unordered_map<NodeId,int>& leafVals) {
    std::unordered_map<NodeId,int> vals = leafVals;
    vals[0] = 0; // CONST0

    for (const AigNode& nd : aig.nodes) {
        if (nd.id > target) break;
        if (vals.count(nd.id)) continue;              // leaf boundary or already done
        if (nd.type != AigNodeType::AND) { vals[nd.id] = 0; continue; }

        NodeId f0 = nd.fanin0 >> 1, f1 = nd.fanin1 >> 1;
        int v0 = vals.count(f0) ? vals[f0] : 0;
        int v1 = vals.count(f1) ? vals[f1] : 0;
        if (nd.fanin0 & 1) v0 ^= 1;
        if (nd.fanin1 & 1) v1 ^= 1;
        vals[nd.id] = v0 & v1;
    }
    return vals.count(target) ? vals[target] : 0;
}

// ─── LutBuilder::buildLuts ───────────────────────────────────────────────────
//
// 1. Backward pass from POs  — collect all required AND nodes.
// 2. For each required node  — enumerate 2^|leaves| input combos,
//                              record minterms, assemble Lut.
// 3. Inverter LUTs           — for signals whose BLIF name is an inverted
//                              output of a required node.

void LutBuilder::buildLuts(const Aig& aig,
                            const std::unordered_map<NodeId, Cut>& bestCuts) {
    luts_.clear();

    // ── Build reverse maps from aig.nameToLit ────────────────────────────────
    // posLitToName : positive AIG literal  → BLIF signal name
    // invNodeToName: AND node id whose inverted output has a BLIF name → name
    std::unordered_map<AigLit,std::string> posLitToName;
    std::unordered_map<NodeId,std::string> invNodeToName;

    for (const auto& [name, lit] : aig.nameToLit) {
        NodeId nd = lit >> 1;
        if (!(lit & 1)) {
            posLitToName.emplace(lit, name);
        } else if (nd < (NodeId)aig.nodes.size() &&
                   aig.nodes[nd].type == AigNodeType::AND) {
            invNodeToName.emplace(nd, name);
        }
    }

    // Helper: return the BLIF signal name for a node's positive output.
    auto getNodeName = [&](NodeId nd) -> std::string {
        auto it = posLitToName.find(nd << 1);
        if (it != posLitToName.end()) return it->second;
        const AigNode& n = aig.nodes[nd];
        if (n.type == AigNodeType::PI) return n.name;
        return "n" + std::to_string(nd);
    };

    // ── 1. Backward pass: collect required AND nodes ──────────────────────────
    std::unordered_set<NodeId> required;
    std::vector<NodeId>        worklist;

    for (AigLit poLit : aig.primaryOutputs) {
        NodeId nd = poLit >> 1;
        if (nd < (NodeId)aig.nodes.size() &&
            aig.nodes[nd].type == AigNodeType::AND &&
            !required.count(nd)) {
            required.insert(nd);
            worklist.push_back(nd);
        }
    }

    while (!worklist.empty()) {
        NodeId id = worklist.back(); worklist.pop_back();
        auto it = bestCuts.find(id);
        if (it == bestCuts.end()) continue;
        for (NodeId leaf : it->second.leaves) {
            if (leaf == id) continue;
            if (aig.nodes[leaf].type == AigNodeType::AND && !required.count(leaf)) {
                required.insert(leaf);
                worklist.push_back(leaf);
            }
        }
    }

    // ── 2. Build LUTs for required AND nodes (topological / id order) ─────────
    //
    // If node nd has an inverted BLIF name (e.g. v12.0 = NOT(n16)) AND its
    // positive output is not used as a leaf by any other required node, absorb
    // the inversion directly into this LUT (invert truth table, rename output).
    // This avoids a separate 1-input inverter LUT that would add one depth level.

    // Pre-compute which nodes appear as leaves in other required nodes' cuts.
    std::unordered_set<NodeId> usedAsLeaf;
    for (NodeId req : required) {
        auto it = bestCuts.find(req);
        if (it == bestCuts.end()) continue;
        for (NodeId leaf : it->second.leaves)
            if (leaf != req) usedAsLeaf.insert(leaf);
    }

    // Nodes whose inversion was absorbed into Step 2 — skip in Step 3.
    std::unordered_set<NodeId> invertedAbsorbed;

    for (const AigNode& node : aig.nodes) {
        NodeId id = node.id;
        if (node.type != AigNodeType::AND) continue;
        if (!required.count(id))           continue;

        auto it = bestCuts.find(id);
        if (it == bestCuts.end()) continue;
        const Cut& cut = it->second;

        // Decide output name and whether to invert the truth table.
        bool absorbInvert = invNodeToName.count(id) && !usedAsLeaf.count(id);
        std::string outName = absorbInvert ? invNodeToName.at(id) : getNodeName(id);
        if (absorbInvert) invertedAbsorbed.insert(id);

        Lut lut;
        lut.output = outName;
        for (NodeId leaf : cut.leaves)
            lut.inputs.push_back(getNodeName(leaf));

        int m = (int)cut.leaves.size();
        for (int mask = 0; mask < (1 << m); ++mask) {
            std::unordered_map<NodeId,int> leafVals;
            for (int i = 0; i < m; ++i)
                leafVals[cut.leaves[i]] = (mask >> i) & 1;

            int val = evalNode(aig, id, leafVals);
            if (absorbInvert) val ^= 1;

            if (val) {
                SopTerm term;
                term.onset = 1;
                for (int i = 0; i < m; ++i)
                    term.pattern.push_back((mask >> i) & 1);
                lut.terms.push_back(term);
            }
        }

        luts_.push_back(lut);
    }

    // ── 3. Inverter LUTs for signals named via inverted AND outputs ───────────
    // Only emit an inverter LUT when the node's positive output is used as a
    // leaf elsewhere (so we can't absorb the inversion into the parent LUT).
    for (const auto& [nd, name] : invNodeToName) {
        if (!required.count(nd))        continue;
        if (invertedAbsorbed.count(nd)) continue;
        std::string src = getNodeName(nd);
        if (src == name) continue;
        Lut inv;
        inv.inputs = {src};
        inv.output = name;
        inv.terms.push_back(SopTerm{{0}, 1});
        luts_.push_back(inv);
    }
}

// ─── LutBuilder::print ───────────────────────────────────────────────────────

void LutBuilder::print() const {
    std::cout << "\n======LUT Builder Results======\n";
    std::cout << "LutBuilder (k=" << k_ << ")\n";
    std::cout << "  LUTs built: " << luts_.size() << "\n\n";

    for (size_t i = 0; i < luts_.size(); ++i) {
        const Lut& lut = luts_[i];
        std::cout << "  LUT[" << i << "]: (";
        for (size_t j = 0; j < lut.inputs.size(); ++j)
            std::cout << lut.inputs[j] << (j + 1 < lut.inputs.size() ? ", " : "");
        std::cout << ") -> " << lut.output << "\n";
        for (size_t ti = 0; ti < lut.terms.size(); ++ti) {
            const SopTerm& t = lut.terms[ti];
            std::cout << "    [" << ti << "] [";
            for (int v : t.pattern) std::cout << v << " ";
            std::cout << "] onset=" << t.onset << "\n";
        }
    }
}
