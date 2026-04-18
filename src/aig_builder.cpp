#include "../inc/aig_builder.h"
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include <vector>

// ----------------------------------------------------------------------------
// makeAnd — structural hashing: same inputs → same node
// ----------------------------------------------------------------------------
AigLit AigBuilder::makeAnd(AigLit a, AigLit b) {
    // Constant propagation
    if (a == 0 || b == 0) return 0;   // 0 AND x = 0
    if (a == 1)           return b;    // 1 AND x = x
    if (b == 1)           return a;    // x AND 1 = x
    if (a == b)           return a;    // x AND x = x
    if (a == makeNot(b))  return 0;   // x AND NOT(x) = 0

    // Canonical order to halve cache entries
    if (a > b) std::swap(a, b);

    auto key = std::make_pair(a, b);
    auto it  = andCache_.find(key);
    if (it != andCache_.end()) return it->second;

    NodeId id = (NodeId)aig_.nodes.size();
    AigNode node;
    node.id     = id;
    node.type   = AigNodeType::AND;
    node.fanin0 = a;
    node.fanin1 = b;
    aig_.nodes.push_back(node);

    AigLit lit      = litOf(id);
    andCache_[key]  = lit;
    return lit;
}

// ----------------------------------------------------------------------------
// buildSopTerm — one product row → AIG literal
// pattern[i]: 1=positive, 0=negative, -1=don't care
// ----------------------------------------------------------------------------
AigLit AigBuilder::buildSopTerm(const SopTerm& term,
                                 const std::vector<std::string>& inputs) {
    AigLit result = 1;  // CONST1 (neutral for AND)

    for (size_t i = 0; i < term.pattern.size(); ++i) {
        if (term.pattern[i] == -1) continue;

        auto it = aig_.nameToLit.find(inputs[i]);
        if (it == aig_.nameToLit.end()) {
            std::cerr << "[AigBuilder] unknown signal: " << inputs[i] << "\n";
            continue;
        }
        AigLit lit = it->second;
        if (term.pattern[i] == 0) lit = makeNot(lit);
        result = makeAnd(result, lit);
    }
    return result;
}

// ----------------------------------------------------------------------------
// buildNamesBlock — SOP (multiple terms) → AIG literal via OR chain
// ----------------------------------------------------------------------------
AigLit AigBuilder::buildNamesBlock(const NamesBlock& block) {
    if (block.terms.empty()) return 0;  // constant 0

    AigLit result = 0;  // CONST0 (neutral for OR)
    for (const SopTerm& term : block.terms) {
        AigLit termLit = buildSopTerm(term, block.inputs);
        if (term.onset == 0) termLit = makeNot(termLit);
        result = makeOr(result, termLit);
    }
    return result;
}

// ----------------------------------------------------------------------------
// build — main entry
// ----------------------------------------------------------------------------
bool AigBuilder::build(const BlifNetwork& network) {
    aig_      = Aig{};
    andCache_ = {};

    // Node 0 = CONST0
    AigNode c0;
    c0.id   = 0;
    c0.type = AigNodeType::CONST0;
    aig_.nodes.push_back(c0);
    // CONST0 lit = 0, CONST1 lit = 1 (NOT CONST0)

    // Primary inputs → PI nodes
    for (const std::string& name : network.primaryInputs) {
        NodeId id = (NodeId)aig_.nodes.size();
        AigNode node;
        node.id   = id;
        node.type = AigNodeType::PI;
        node.name = name;
        aig_.nodes.push_back(node);
        aig_.primaryInputs.push_back(id);
        aig_.nameToLit[name] = litOf(id);  // positive literal
    }

    // Topological sort: process blocks whose inputs are all resolved
    std::unordered_set<std::string> resolved;
    for (const std::string& pi : network.primaryInputs)
        resolved.insert(pi);

    std::vector<bool> done(network.namesBlocks.size(), false);
    bool progress = true;
    while (progress) {
        progress = false;
        for (size_t i = 0; i < network.namesBlocks.size(); ++i) {
            if (done[i]) continue;
            const NamesBlock& block = network.namesBlocks[i];
            bool ready = true;
            for (const std::string& inp : block.inputs)
                if (!resolved.count(inp)) { ready = false; break; }
            if (!ready) continue;

            AigLit outLit = buildNamesBlock(block);
            aig_.nameToLit[block.output] = outLit;
            resolved.insert(block.output);
            done[i]  = true;
            progress = true;
        }
    }

    // Resolve primary outputs
    for (const std::string& name : network.primaryOutputs) {
        auto it = aig_.nameToLit.find(name);
        if (it == aig_.nameToLit.end()) {
            std::cerr << "[AigBuilder] PO not found: " << name << "\n";
            aig_.primaryOutputs.push_back(0);
        } else {
            aig_.primaryOutputs.push_back(it->second);
        }
    }

    return true;
}

void AigBuilder::print() const {
    int nConst = 0, nPI = 0, nAnd = 0;
    for (const AigNode& n : aig_.nodes) {
        if      (n.type == AigNodeType::CONST0) ++nConst;
        else if (n.type == AigNodeType::PI)     ++nPI;
        else                                     ++nAnd;
    }
    std::cout << "AIG: " << aig_.nodes.size()
              << " nodes (CONST=" << nConst
              << ", PI=" << nPI
              << ", AND=" << nAnd << ")\n";
    std::cout << "PIs: " << aig_.primaryInputs.size()
              << "  POs: " << aig_.primaryOutputs.size() << "\n\n";

    for (const AigNode& n : aig_.nodes) {
        std::cout << "  Node[" << n.id << "] ";
        if (n.type == AigNodeType::CONST0) {
            std::cout << "CONST0\n";
        } else if (n.type == AigNodeType::PI) {
            std::cout << "PI  \"" << n.name << "\"  lit=" << (n.id << 1) << "\n";
        } else {
            auto fmt = [](AigLit l) -> std::string {
                return "node" + std::to_string(l >> 1) + (l & 1 ? "'" : "");
            };
            std::cout << "AND  f0=" << fmt(n.fanin0)
                      << "  f1=" << fmt(n.fanin1) << "\n";
        }
    }

    std::cout << "\nPO literals: ";
    for (AigLit lit : aig_.primaryOutputs)
        std::cout << (lit >> 1) << (lit & 1 ? "' " : " ");
    std::cout << "\n";
}
