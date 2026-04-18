#include "../inc/lut_eval.h"
#include <iostream>

void LutEval::enumerateCuts(const Aig& aig) {
    // TODO
    (void)aig;
}

void LutEval::selectCuts(const Aig& aig) {
    // TODO
    (void)aig;
}

void LutEval::print() const {
    std::cout << "LutEval (k=" << k_ << ")\n";
    std::cout << "  LUTs selected: " << luts_.size() << "\n\n";

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
