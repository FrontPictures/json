#include "input_adapters.hpp"

nlohmann::detail::iterator_input_adapter::iterator_input_adapter(const std::string &str)
    : current(str.begin()), end(str.end())
{}

nlohmann::detail::iterator_input_adapter::iterator_input_adapter(
    const std::vector<uint8_t> &container)
    : vcurrent(container.begin()), vend(container.end()), vectorType(true)
{}

nlohmann::detail::iterator_input_adapter::iterator_input_adapter(std::string::const_iterator first,
                                                                 std::string::const_iterator last)
    : current(first), end(last)
{}

nlohmann::detail::iterator_input_adapter::iterator_input_adapter(
    std::vector<uint8_t>::const_iterator first, std::vector<uint8_t>::const_iterator last)
    : vcurrent(first), vend(last), vectorType(true)
{}

uint64_t nlohmann::detail::iterator_input_adapter::get_character()
{
    if (!vectorType) {
        if (current != end) {
            auto result = uint64_t(uint8_t(*current));
            std::advance(current, 1);
            return result;
        }
    } else {
        if (vcurrent != vend) {
            auto result = uint64_t(uint8_t(*vcurrent));
            std::advance(vcurrent, 1);
            return result;
        }
    }
    return uint64_t(-1);
}

nlohmann::detail::iterator_input_adapter nlohmann::detail::iterator_input_adapter_factory::create(
    std::string::const_iterator first, std::string::const_iterator last)
{
    return iterator_input_adapter(first, last);
}

nlohmann::detail::iterator_input_adapter nlohmann::detail::iterator_input_adapter_factory::create(
    std::vector<uint8_t>::const_iterator first, std::vector<uint8_t>::const_iterator last)
{
    return iterator_input_adapter(first, last);
}

nlohmann::detail::iterator_input_adapter
nlohmann::detail::container_input_adapter_factory_impl::container_input_adapter_factory::create(
    const std::string &container)
{
    return iterator_input_adapter(container.begin(), container.end());
}

nlohmann::detail::iterator_input_adapter
nlohmann::detail::container_input_adapter_factory_impl::container_input_adapter_factory::create(
    const std::vector<uint8_t> &container)
{
    return iterator_input_adapter(container.begin(), container.end());
}
nlohmann::detail::iterator_input_adapter nlohmann::detail::input_adapter(
    const std::string &container)
{
    return container_input_adapter_factory_impl::container_input_adapter_factory::create(container);
}
nlohmann::detail::iterator_input_adapter nlohmann::detail::input_adapter(
    const std::vector<uint8_t> &container)
{
    return container_input_adapter_factory_impl::container_input_adapter_factory::create(container);
}
