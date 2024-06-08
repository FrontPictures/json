//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <utility>
#include <vector>

namespace nlohmann {

class byte_container_with_subtype : public std::vector<uint8_t>
{
public:
    using container_type = std::vector<uint8_t>;
    using subtype_type = uint64_t;

    byte_container_with_subtype() : container_type() {}
    byte_container_with_subtype(const container_type &b) : container_type(b) {}
    byte_container_with_subtype(container_type &&b) : container_type(std::move(b)) {}
    byte_container_with_subtype(const container_type &b, subtype_type subtype_);
    byte_container_with_subtype(container_type &&b, subtype_type subtype_);

    bool operator==(const byte_container_with_subtype &rhs) const;
    bool operator!=(const byte_container_with_subtype &rhs) const { return !(rhs == *this); }

    void set_subtype(subtype_type subtype_) noexcept;
    subtype_type subtype() const noexcept;
    bool has_subtype() const noexcept { return m_has_subtype; }
    void clear_subtype() noexcept;

private:
    subtype_type m_subtype = 0;
    bool m_has_subtype = false;
};

} // namespace nlohmann
