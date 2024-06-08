#include "to_chars.hpp"
#include <limits>
#include <cmath>

#include "grisu2.h"

char *nlohmann::detail::to_chars(char *first, const char *last, double value)
{
    static_cast<void>(last);

    if (std::signbit(value)) {
        value = -value;
        *first++ = '-';
    }

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
    if (value == 0) {
        *first++ = '0';

        *first++ = '.';
        *first++ = '0';
        return first;
    }
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

    int len = 0;
    int decimal_exponent = 0;
    grisu2::grisu2(first, len, decimal_exponent, value);

    constexpr int kMinExp = -4;

    constexpr int kMaxExp = std::numeric_limits<double>::digits10;

    return grisu2::format_buffer(first, len, decimal_exponent, kMinExp, kMaxExp);
}
