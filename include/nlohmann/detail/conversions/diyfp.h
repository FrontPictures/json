#ifndef DIYFP_H
#define DIYFP_H

#include <cstdint>

class diyfp
{
public:
    static constexpr int kPrecision = 64;

    uint64_t f = 0;
    int e = 0;

    constexpr diyfp(uint64_t f_, int e_) noexcept : f(f_), e(e_) {}

    static diyfp sub(const diyfp &x, const diyfp &y) noexcept;
    static diyfp mul(const diyfp &x, const diyfp &y) noexcept;
    static diyfp normalize(diyfp x) noexcept;
    static diyfp normalize_to(const diyfp &x, const int target_exponent) noexcept;
};

#endif
