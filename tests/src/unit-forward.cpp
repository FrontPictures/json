#include <nlohmann/json_fwd.hpp>
#include <type_traits>
static_assert(std::is_same<nlohmann::json, nlohmann::ordered_json>::value,
              "the standalone forward header declares both JSON names");
#include "doctest_compatibility.h"
#include <nlohmann/json.hpp>
TEST_CASE("forward declarations match the complete JSON type")
{
    nlohmann::json value = {{"key", 42}};
    nlohmann::ordered_json *pointer = &value;
    CHECK(pointer->at("key").get<int>() == 42);
}
