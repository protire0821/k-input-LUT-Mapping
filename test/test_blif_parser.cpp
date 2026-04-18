// ============================================================================
// test/test_blif_parser.cpp
// Manual test for BlifParser — run with: make test
// ============================================================================

#include <iostream>
#include <cassert>
#include "blif_parser.h"

// --------------------------------------------------------------------------
// Helper: print one parsed network to stdout
// --------------------------------------------------------------------------
static void printNetwork(const char* filename, const BlifNetwork& net) {
    std::cout << "=== " << filename << " ===\n";
    std::cout << "  Model   : " << net.modelName << "\n";
    std::cout << "  Inputs  : " << net.primaryInputs.size() << "\n";
    std::cout << "  Outputs : " << net.primaryOutputs.size() << "\n";
    std::cout << "  Blocks  : " << net.namesBlocks.size() << "\n";

    size_t totalTerms = 0;
    for (const auto& b : net.namesBlocks)
        totalTerms += b.terms.size();
    std::cout << "  SOP rows: " << totalTerms << "\n";

    // Spot-check: print first block's details
    if (!net.namesBlocks.empty()) {
        const NamesBlock& b = net.namesBlocks[0];
        std::cout << "  Block[0]: inputs=" << b.inputs.size()
                  << "  output=\"" << b.output << "\""
                  << "  terms=" << b.terms.size() << "\n";
        if (!b.terms.empty())
            std::cout << "  Term[0] : pattern=\"" << b.terms[0].pattern
                      << "\"  onset='" << b.terms[0].onset << "'\n";
    }
    std::cout << "\n";
}

// --------------------------------------------------------------------------
// Test 1 — basic parse + spot-check known values
// --------------------------------------------------------------------------
static void test_testcase1() {
    BlifParser parser;
    assert(parser.parse("testcase/testcase1.blif") && "parse failed");

    const BlifNetwork& net = parser.getNetwork();
    assert(net.modelName == "source.pla");
    assert(net.primaryInputs.size()  == 12);
    assert(net.primaryOutputs.size() == 8);
    assert(net.namesBlocks.size()    == 8);

    // First .names block: v0 v4 v10 v11 -> v12.0, 3 SOP rows
    const NamesBlock& blk = net.namesBlocks[0];
    assert(blk.inputs.size() == 4);
    assert(blk.output        == "v12.0");
    assert(blk.terms.size()  == 3);
    assert(blk.terms[0].pattern == "0---");
    assert(blk.terms[0].onset   == '1');

    printNetwork("testcase1.blif", net);
    std::cout << "[PASS] test_testcase1\n\n";
}

// --------------------------------------------------------------------------
// Test 2 — larger file + line-continuation (\)
// --------------------------------------------------------------------------
static void test_testcase2() {
    BlifParser parser;
    assert(parser.parse("testcase/testcase2.blif") && "parse failed");

    const BlifNetwork& net = parser.getNetwork();
    assert(net.modelName == "alu4_cl");
    assert(net.primaryInputs.size()  == 10);
    assert(net.primaryOutputs.size() == 6);
    assert(net.namesBlocks.size()    == 59);

    // First block has 23 inputs (long line, no continuation needed)
    assert(net.namesBlocks[0].inputs.size() == 23);
    assert(net.namesBlocks[0].output        == "k");

    printNetwork("testcase2.blif", net);
    std::cout << "[PASS] test_testcase2\n\n";
}

// --------------------------------------------------------------------------
// Test 3 — even larger file
// --------------------------------------------------------------------------
static void test_testcase3() {
    BlifParser parser;
    assert(parser.parse("testcase/testcase3.blif") && "parse failed");

    const BlifNetwork& net = parser.getNetwork();
    assert(net.modelName == "alu4_cl");
    assert(net.primaryInputs.size()  == 14);
    assert(net.primaryOutputs.size() == 8);
    assert(net.namesBlocks.size()    == 112);

    printNetwork("testcase3.blif", net);
    std::cout << "[PASS] test_testcase3\n\n";
}

// --------------------------------------------------------------------------
// Test 4 — bad file path returns false, does not crash
// --------------------------------------------------------------------------
static void test_bad_file() {
    BlifParser parser;
    bool ok = parser.parse("testcase/does_not_exist.blif");
    assert(!ok && "should return false for missing file");
    std::cout << "[PASS] test_bad_file\n\n";
}

// --------------------------------------------------------------------------
int main() {
    std::cout << "==============================\n";
    std::cout << "  BlifParser Tests\n";
    std::cout << "==============================\n\n";

    test_testcase1();
    test_testcase2();
    test_testcase3();
    test_bad_file();

    std::cout << "All tests passed.\n";
    return 0;
}
