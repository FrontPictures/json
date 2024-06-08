#include "byte_container_with_subtype.hpp"
#include <tuple>

nlohmann::byte_container_with_subtype::byte_container_with_subtype(const container_type &b,
                                                                   subtype_type subtype_)
    : container_type(b), m_subtype(subtype_), m_has_subtype(true)
{}

nlohmann::byte_container_with_subtype::byte_container_with_subtype(container_type &&b,
                                                                   subtype_type subtype_)
    : container_type(std::move(b)), m_subtype(subtype_), m_has_subtype(true)
{}

bool nlohmann::byte_container_with_subtype::operator==(const byte_container_with_subtype &rhs) const
{
    return std::tie(static_cast<const std::vector<uint8_t> &>(*this), m_subtype, m_has_subtype)
           == std::tie(static_cast<const std::vector<uint8_t> &>(rhs), rhs.m_subtype,
                       rhs.m_has_subtype);
}

void nlohmann::byte_container_with_subtype::set_subtype(subtype_type subtype_) noexcept
{
    m_subtype = subtype_;
    m_has_subtype = true;
}

nlohmann::byte_container_with_subtype::subtype_type nlohmann::byte_container_with_subtype::subtype()
    const noexcept
{
    return m_has_subtype ? m_subtype : static_cast<subtype_type>(-1);
}

void nlohmann::byte_container_with_subtype::clear_subtype() noexcept
{
    m_subtype = 0;
    m_has_subtype = false;
}
