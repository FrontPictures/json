#ifndef GRISU2_H
#define GRISU2_H

#include "diyfp.h"

namespace grisu2 {

struct cached_power
{
    uint64_t f;
    int e;
    int k;
};

cached_power get_cached_power_for_binary_exponent(int e);
int find_largest_pow10(const std::uint32_t n, std::uint32_t &pow10);
void grisu2_round(char *buf, int len, uint64_t dist, uint64_t delta, uint64_t rest, uint64_t ten_k);
void grisu2_digit_gen(char *buffer, int &length, int &decimal_exponent, diyfp M_minus, diyfp w,
                      diyfp M_plus);
void grisu2(char *buf, int &len, int &decimal_exponent, diyfp m_minus, diyfp v, diyfp m_plus);
void grisu2(char *buf, int &len, int &decimal_exponent, double value);
char *append_exponent(char *buf, int e);
char *format_buffer(char *buf, int len, int decimal_exponent, int min_exp, int max_exp);

} // namespace grisu2

#endif
