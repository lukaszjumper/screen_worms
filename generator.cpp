#include "generator.h"

Generator::Generator(uint32_t seed) {
    r = seed;
}

// Generuje kolejną liczbę pseudolosową 32-bitową.
uint32_t Generator::get_random() {
    r_value = r;
    r = (r * p) % mod;
    return r_value;
}