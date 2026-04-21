#ifndef AIG_BUILDER_H
#define AIG_BUILDER_H

#include <unordered_map>
#include <utility>
#include "types.h"

class AigBuilder {
public:
    AigBuilder() = default;

    bool build   (const BlifNetwork& network);
    void print   () const;

    const Aig& getAig() const { return aig_; }

private:
    Aig aig_;

    struct PairHash {
        size_t operator()(std::pair<AigLit,AigLit> p) const {
            return std::hash<int64_t>()(((int64_t)p.first << 32) | (uint32_t)p.second);
        }
    };
    std::unordered_map<std::pair<AigLit,AigLit>, AigLit, PairHash> andCache_;

    AigLit makeAnd(AigLit a, AigLit b); // structural hashing — too complex to inline
};

#endif // AIG_BUILDER_H
