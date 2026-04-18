#ifndef AIG_BUILDER_H
#define AIG_BUILDER_H

#include <unordered_map>
#include <utility>
#include <cstdint>
#include "types.h"

class AigBuilder {
public:
    AigBuilder() = default;
    ~AigBuilder() = default;

    bool build(const BlifNetwork& network);
    void print() const;

    const Aig& getAig() const { return aig_; }

private:
    Aig aig_;

    struct PairHash {
        size_t operator()(std::pair<AigLit,AigLit> p) const {
            return std::hash<int64_t>()(((int64_t)p.first << 32) | (uint32_t)p.second);
        }
    };
    std::unordered_map<std::pair<AigLit,AigLit>, AigLit, PairHash> andCache_;

    AigLit litOf   (NodeId id, bool invert = false) { return (id << 1) | (int)invert; }
    AigLit makeNot (AigLit a)                       { return a ^ 1; }
    AigLit makeAnd (AigLit a, AigLit b);
    AigLit makeOr  (AigLit a, AigLit b)             { return makeNot(makeAnd(makeNot(a), makeNot(b))); }

    AigLit buildSopTerm   (const SopTerm& term, const std::vector<std::string>& inputs);
    AigLit buildNamesBlock(const NamesBlock& block);
};

#endif // AIG_BUILDER_H
