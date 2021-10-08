#ifndef __GENERATOR_H__
#define __GENERATOR_H__

#include <cstdint>

class Generator {
private:
    uint64_t r;
    const uint64_t p = 279410273;
    const uint64_t mod = 4294967291;
    uint32_t r_value;
public:
    Generator(uint32_t seed);
    uint32_t get_random();
};

#endif /* __GENERATOR_H__ */