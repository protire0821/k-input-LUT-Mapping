// ============================================================================
// test/test_runner.cpp
// Usage:
//   ./test_runner [module] [tc]
//   module : parser | aig_builder | lut_eval | writer | all  (default: all)
//   tc     : 1 | 2 | 3 | all                                 (default: all)
// ============================================================================

#include <iostream>
#include <string>
#include <vector>

#include "blif_parser.h"
#include "aig_builder.h"
#include "lut_eval.h"
#include "blif_writer.h"

static constexpr int TEST_K = 4;

struct Testcase {
    int         id;
    const char* input;
    const char* output;
};

static const Testcase TESTCASES[] = {
    {1, "testcase/testcase1.blif", "testcase/out1.blif"},
    {2, "testcase/testcase2.blif", "testcase/out2.blif"},
    {3, "testcase/testcase3.blif", "testcase/out3.blif"},
};
static constexpr int N_TC = 3;

// ----------------------------------------------------------------------------

static bool test_parser(const Testcase& tc) {
    std::cout << "--- [parser] testcase" << tc.id << " ---\n";
    BlifParser parser;
    if (!parser.parse(tc.input)) { std::cout << "[FAIL]\n\n"; return false; }
    parser.print();
    std::cout << "[PASS]\n\n";
    return true;
}

static bool test_aig_builder(const Testcase& tc) {
    std::cout << "--- [aig_builder] testcase" << tc.id << " ---\n";
    BlifParser parser;
    if (!parser.parse(tc.input)) { std::cout << "[FAIL] parse\n\n"; return false; }
    AigBuilder builder;
    if (!builder.build(parser.getNetwork())) { std::cout << "[FAIL] build\n\n"; return false; }
    builder.print();
    std::cout << "[PASS]\n\n";
    return true;
}

static bool test_lut_eval(const Testcase& tc) {
    std::cout << "--- [lut_eval k=" << TEST_K << "] testcase" << tc.id << " ---\n";
    BlifParser parser;
    if (!parser.parse(tc.input)) { std::cout << "[FAIL] parse\n\n"; return false; }
    AigBuilder builder;
    if (!builder.build(parser.getNetwork())) { std::cout << "[FAIL] build\n\n"; return false; }
    LutEval eval(TEST_K);
    eval.enumerateCuts(builder.getAig());
    eval.selectCuts(builder.getAig());
    eval.print();
    std::cout << "[PASS]\n\n";
    return true;
}

static bool test_writer(const Testcase& tc) {
    std::cout << "--- [writer] testcase" << tc.id << " ---\n";
    BlifParser parser;
    if (!parser.parse(tc.input)) { std::cout << "[FAIL] parse\n\n"; return false; }
    const BlifNetwork& net = parser.getNetwork();

    std::vector<Lut> luts;
    for (const NamesBlock& b : net.namesBlocks) {
        Lut lut;
        lut.inputs = b.inputs;
        lut.output = b.output;
        lut.terms  = b.terms;
        luts.push_back(std::move(lut));
    }

    BlifWriter writer;
    if (!writer.write(tc.output, net.modelName, net.primaryInputs, net.primaryOutputs, luts)) {
        std::cout << "[FAIL] write\n\n"; return false;
    }
    writer.print();
    std::cout << "[PASS]\n\n";
    return true;
}

// ----------------------------------------------------------------------------

int main(int argc, char* argv[]) {
    std::string module = (argc >= 2) ? argv[1] : "all";
    std::string tcArg  = (argc >= 3) ? argv[2] : "all";

    if (module != "all" && module != "parser" && module != "aig_builder" &&
        module != "lut_eval" && module != "writer") {
        std::cerr << "Valid modules: parser | aig_builder | lut_eval | writer | all\n";
        return 1;
    }

    std::vector<const Testcase*> toRun;
    if (tcArg == "all") {
        for (int i = 0; i < N_TC; ++i) toRun.push_back(&TESTCASES[i]);
    } else {
        int id = std::stoi(tcArg);
        for (int i = 0; i < N_TC; ++i)
            if (TESTCASES[i].id == id) { toRun.push_back(&TESTCASES[i]); break; }
        if (toRun.empty()) { std::cerr << "Unknown testcase: " << tcArg << "\n"; return 1; }
    }

    std::cout << "==============================\n";
    std::cout << "  module=" << module << "  tc=" << tcArg << "\n";
    std::cout << "==============================\n\n";

    int passed = 0, total = 0;
    auto run = [&](bool (*fn)(const Testcase&), const Testcase* tc) {
        ++total; if (fn(*tc)) ++passed;
    };

    for (const Testcase* tc : toRun) {
        if (module == "parser"      || module == "all") run(test_parser,      tc);
        if (module == "aig_builder" || module == "all") run(test_aig_builder, tc);
        if (module == "lut_eval"    || module == "all") run(test_lut_eval,    tc);
        if (module == "writer"      || module == "all") run(test_writer,      tc);
    }

    std::cout << "------------------------------\n";
    std::cout << "  Result: " << passed << "/" << total << " passed\n";
    std::cout << "------------------------------\n";
    return (passed == total) ? 0 : 1;
}
