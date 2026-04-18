#include "aig_builder.h"

#include <iostream>

bool AigBuilder::build(const BlifNetwork& network) {
    // TODO:
    // 1. Create constant-0 node.
    // 2. Create one PI node per primary input; record name -> literal.
    // 3. For each NamesBlock (in topological order):
    //      - Convert its SOP to a series of 2-input AND + inverter nodes.
    //      - Register output signal name -> resulting literal.
    // 4. Resolve primary outputs via nameToLit -> aig_.primaryOutputs.
    (void)network;
    return true;
}
