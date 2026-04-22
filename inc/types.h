#ifndef TYPES_H
#define TYPES_H

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <cstdint>


// SOP (Sum-Of-Products, one line) term, e.g. "10-1 1"
struct SopTerm {
    std::vector<int> pattern; // inputs pattern, int{0, 1, -1} for '0','1','-'
    int onset;                // 1 or 0
};

// .names block
struct NamesBlock {
    std::vector<std::string> inputs; // input signal names
    std::string              output; // output signal name
    std::vector<SopTerm>     terms;  // SOP rows
};

struct BlifNetwork {
    std::string              modelName;
    std::vector<std::string> primaryInputs;
    std::vector<std::string> primaryOutputs;
    std::vector<NamesBlock>  namesBlocks;
};

using NodeId = int32_t;  // AIG node id
using AigLit = int32_t;  // AIG literal: (id << 1) | invert_bit

enum class AigNodeType {
    CONST0,                 // constant 0
    PI,                     // primary input
    AND                     // two-input AND
};

struct AigNode {
    NodeId       id   = -1;
    AigNodeType  type = AigNodeType::AND;
    AigLit       fanin0 = 0;                // only valid for AND
    AigLit       fanin1 = 0;                // only valid for AND
    std::string  name;                      // for PI / PO mapping (optional)
};

struct Aig {
    std::vector<AigNode>                    nodes;         // indexed by NodeId
    std::vector<NodeId>                     primaryInputs; // order preserved
    std::vector<AigLit>                     primaryOutputs;// order preserved (literals)
    std::unordered_map<std::string, AigLit> nameToLit;     // signal name -> AIG literal
};

struct Cut {
    std::vector<NodeId> leaves;  // cut inputs (AIG node ids)
    int    level = 0;
    double cost  = 0.0;
};

struct Lut {
    std::vector<std::string> inputs;  // input signal names (in the output BLIF)
    std::string              output;  // output signal name
    std::vector<SopTerm>     terms;   // truth table in SOP form
};

#endif // TYPES_H
