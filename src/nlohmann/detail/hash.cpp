#include "hash.hpp"
#include "nlohmann/json.hpp"

size_t nlohmann::detail::combine(size_t seed, size_t h) noexcept
{
    seed ^= h + 0x9e3779b9 + (seed << 6U) + (seed >> 2U);
    return seed;
}

size_t nlohmann::detail::hash(const ordered_json &j)
{
    const auto type = static_cast<size_t>(j.type());
    switch (j.type()) {
    case ordered_json::value_t::null:
    case ordered_json::value_t::discarded: {
        return combine(type, 0);
    }

    case ordered_json::value_t::object: {
        auto seed = combine(type, j.size());
        for (const auto &element : j.items()) {
            const auto h = std::hash<std::string>{}(element.key());
            seed = combine(seed, h);
            seed = combine(seed, hash(element.value()));
        }
        return seed;
    }

    case ordered_json::value_t::array: {
        auto seed = combine(type, j.size());
        for (const auto &element : j) {
            seed = combine(seed, hash(element));
        }
        return seed;
    }

    case ordered_json::value_t::string: {
        const auto h = std::hash<std::string>{}(j.get_string_ref());
        return combine(type, h);
    }

    case ordered_json::value_t::boolean: {
        const auto h = std::hash<bool>{}(j.get<bool>());
        return combine(type, h);
    }

    case ordered_json::value_t::number_integer: {
        const auto h = std::hash<int64_t>{}(j.get<int64_t>());
        return combine(type, h);
    }

    case ordered_json::value_t::number_float: {
        const auto h = std::hash<double>{}(j.get<double>());
        return combine(type, h);
    }

    case ordered_json::value_t::binary: {
        auto seed = combine(type, j.get_binary().size());
        const auto h = std::hash<bool>{}(j.get_binary().has_subtype());
        seed = combine(seed, h);
        seed = combine(seed, static_cast<size_t>(j.get_binary().subtype()));
        for (const auto byte : j.get_binary()) {
            seed = combine(seed, std::hash<uint8_t>{}(byte));
        }
        return seed;
    }

    default: return 0;
    }
}
