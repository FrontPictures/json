#include "value_t.hpp"
#include <array>
#include <cstddef>

std::partial_ordering nlohmann::detail::operator<=>(const value_t lhs, const value_t rhs) noexcept
{
    static constexpr std::array<uint8_t, 9> order = {{
        0 /* null */, 3 /* object */, 4 /* array */, 5 /* string */, 1 /* boolean */,
        2 /* integer */, 2 /* unsigned */, 2 /* float */, 6 /* binary */
    }};

    const auto l_index = static_cast<size_t>(lhs);
    const auto r_index = static_cast<size_t>(rhs);
    if (l_index < order.size() && r_index < order.size()) {
        return order[l_index] <=> order[r_index];
    }
    return std::partial_ordering::unordered;
}
