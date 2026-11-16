#include <nlohmann/json.hpp>
#include <cstdint>
#include <vector>

int main()
{
    using nlohmann::json;
    const auto value = json::object({{"z", 1}, {"a", json::array({2, 3})}});
    if (json::from_cbor(json::to_cbor(value)) != value
        || json::from_msgpack(json::to_msgpack(value)) != value
        || json::from_ubjson(json::to_ubjson(value, true, true)) != value
        || json::from_bjdata(json::to_bjdata(value, true, true)) != value
        || json::from_bson(json::to_bson(value)) != value) {
        return 1;
    }

    const auto binary = json::binary({1, 2, 3}, 42);
    std::vector<uint8_t> bytes;
    json::to_cbor(binary, bytes);
    const auto decoded = json::from_cbor(bytes, true, true, json::cbor_tag_handler_t::store);
    return decoded == binary && decoded.get_binary().subtype() == 42 ? 0 : 1;
}
