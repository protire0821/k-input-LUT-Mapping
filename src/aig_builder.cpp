#include "../inc/aig_builder.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include <functional>

// makeAnd stays as a method: it has real logic (constant prop + structural hashing)
AigLit AigBuilder::makeAnd(AigLit a, AigLit b) {
    if (a == 0 || b == 0) return 0;   // 0 & x = 0
    if (a == 1)           return b;   // 1 & x = x
    if (b == 1)           return a;   // x & 1 = x
    if (a == b)           return a;   // x & x = x
    if (a == (b ^ 1))     return 0;   // x & ~x = 0

    if (a > b) std::swap(a, b);       // canonical order → halve cache entries

    auto key = std::make_pair(a, b);
    auto it  = andCache_.find(key);
    if (it != andCache_.end()) return it->second;

    NodeId  id = (NodeId)aig_.nodes.size();
    aig_.nodes.push_back({id, AigNodeType::AND, a, b});
    return andCache_[key] = id << 1;
}

bool AigBuilder::build(const BlifNetwork& network) {
    aig_      = {};
    andCache_ = {};

    // ── Constants ────────────────────────────────────────────────────────────
    // node 0 = CONST0,  lit 0 = CONST0,  lit 1 = CONST1  (NOT CONST0)
    aig_.nodes.push_back({0, AigNodeType::CONST0});

    // ── Primary inputs ───────────────────────────────────────────────────────
    for (const std::string& name : network.primaryInputs) {
        NodeId id = (NodeId)aig_.nodes.size();
        aig_.nodes.push_back({id, AigNodeType::PI, 0, 0, name});
        aig_.primaryInputs.push_back(id);
        aig_.nameToLit[name] = id << 1;   // positive literal
    }

    // ── Helpers defined here so they can see aig_ and makeAnd ────────────────

    auto NOT = [](AigLit a) { return a ^ 1; };  // NOT: flip invert bit (no new node needed)

    auto OR = [&](AigLit a, AigLit b) {         // OR via De Morgan: a | b = ~(~a & ~b)
        return makeAnd(a ^ 1, b ^ 1) ^ 1;
    };

    // One .names block (SOP) → OR of AND-chains, one per product term
    auto buildBlock = [&](const NamesBlock& blk) -> AigLit {
        AigLit BlockLit = 0; // neutral for OR
        for (const SopTerm& term : blk.terms) {

            AigLit termLit = 1; // neutral for AND
            for (size_t i = 0; i < term.pattern.size(); ++i) {
                if (term.pattern[i] == -1) continue;      // don't-care
                AigLit lit = aig_.nameToLit.at(blk.inputs[i]);
                if (term.pattern[i] == 0) lit ^= 1;       // complement
                termLit = makeAnd(termLit, lit);
            }

            if (term.onset == 0) termLit = NOT(termLit);

            BlockLit = OR(BlockLit, termLit);
        }
        return BlockLit;
    };

    // ── Topological sort (recursive DFS) ────────────────────────────────────
    std::unordered_map<std::string, size_t> outputToBlock;
    for (size_t blkIdx = 0; blkIdx < network.namesBlocks.size(); ++blkIdx)
        outputToBlock[network.namesBlocks[blkIdx].output] = blkIdx;

    std::unordered_set<std::string> resolvedSignals(network.primaryInputs.begin(), network.primaryInputs.end());

    std::function<void(const std::string&)> resolve = [&](const std::string& name) {
        if (resolvedSignals.count(name)) return;

        const NamesBlock& blk = network.namesBlocks[outputToBlock.at(name)];

        for (const std::string& inp : blk.inputs)       // ensure fanins are resolved first
            resolve(inp);

        aig_.nameToLit[name] = buildBlock(blk);         // then build this block and cache its literal
        resolvedSignals.insert(name);
    };

    for (const NamesBlock& blk : network.namesBlocks)
        resolve(blk.output);

    // ── Primary outputs ──────────────────────────────────────────────────────
    for (const std::string& name : network.primaryOutputs)
        aig_.primaryOutputs.push_back(aig_.nameToLit.at(name));

    return true;
}

void AigBuilder::print() const {
    int nPI = 0, nAnd = 0;
    for (const AigNode& n : aig_.nodes)
        if      (n.type == AigNodeType::PI)  ++nPI;
        else if (n.type == AigNodeType::AND) ++nAnd;

    std::cout << "\n======AIG Build Results======" << std::endl;

    std::cout << "AIG: " << aig_.nodes.size()
              << " nodes (CONST=1, PI=" << nPI << ", AND=" << nAnd << ")\n"
              << "PIs: " << aig_.primaryInputs.size()
              << "  POs: " << aig_.primaryOutputs.size() << "\n";

    for (const AigNode& n : aig_.nodes) {
        std::cout << "  Node[" << n.id << "] ";
        if (n.type == AigNodeType::CONST0)
            std::cout << "CONST0\n";
        else if (n.type == AigNodeType::PI)
            std::cout << "PI \"" << n.name << "\"  lit=" << (n.id << 1) << "\n";
        else {
            auto fmt = [](AigLit l) {
                return "node" + std::to_string(l >> 1) + (l & 1 ? "'" : "");
            };
            std::cout << "AND  f0=" << fmt(n.fanin0)
                      << "  f1=" << fmt(n.fanin1) << "\n";
        }
    }

    std::cout << "\nPO literals: ";
    for (AigLit l : aig_.primaryOutputs)
        std::cout << (l >> 1) << (l & 1 ? "' " : " ");
    std::cout << "\n";
}
