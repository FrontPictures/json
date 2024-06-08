#include "diyfp.h"

diyfp diyfp::sub(const diyfp &x, const diyfp &y) noexcept { return {x.f - y.f, x.e}; }

diyfp diyfp::mul(const diyfp &x, const diyfp &y) noexcept
{

    const uint64_t u_lo = x.f & 0xFFFFFFFFu;
    const uint64_t u_hi = x.f >> 32u;
    const uint64_t v_lo = y.f & 0xFFFFFFFFu;
    const uint64_t v_hi = y.f >> 32u;

    const uint64_t p0 = u_lo * v_lo;
    const uint64_t p1 = u_lo * v_hi;
    const uint64_t p2 = u_hi * v_lo;
    const uint64_t p3 = u_hi * v_hi;

    const uint64_t p0_hi = p0 >> 32u;
    const uint64_t p1_lo = p1 & 0xFFFFFFFFu;
    const uint64_t p1_hi = p1 >> 32u;
    const uint64_t p2_lo = p2 & 0xFFFFFFFFu;
    const uint64_t p2_hi = p2 >> 32u;

    uint64_t Q = p0_hi + p1_lo + p2_lo;
    Q += uint64_t{1} << (64u - 32u - 1u);

    const uint64_t h = p3 + p2_hi + p1_hi + (Q >> 32u);

    return {h, x.e + y.e + 64};
}

diyfp diyfp::normalize(diyfp x) noexcept
{
    while ((x.f >> 63u) == 0) {
        x.f <<= 1u;
        x.e--;
    }
    return x;
}

diyfp diyfp::normalize_to(const diyfp &x, const int target_exponent) noexcept
{
    const int delta = x.e - target_exponent;
    return {x.f << delta, target_exponent};
}
