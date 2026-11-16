#include "doctest_compatibility.h"
#include <nlohmann/json.hpp>
#include <nlohmann/detail/input/json_sax.hpp>
#include <limits>

using nlohmann::json;

TEST_CASE("DOM parsing preserves exception types")
{
    CHECK_THROWS_AS(json::parse("["), json::parse_error);
    const auto callback = [](int, json::parse_event_t, json &) { return true; };
    CHECK_THROWS_AS(json::parse("[", callback), json::parse_error);
    CHECK(json::parse("[", nullptr, false).is_discarded());
    CHECK(json::parse("[", callback, false).is_discarded());
    json root;
    nlohmann::detail::json_sax_dom_parser parser(root);
    const auto error = json::out_of_range::create(408, "excessive array size");
    CHECK_THROWS_AS(parser.parse_error(0, "", error), json::out_of_range);
}

TEST_CASE("pretty output clamps indentation width")
{
    const json object = {{"x", 1}};
    CHECK(object.dump(4096) == object.dump(32));
    CHECK(object.dump((std::numeric_limits<int>::max)()) == object.dump(32));
    CHECK(object.dump(32) == "{\n" + std::string(32, ' ') + "\"x\": 1\n}");
    CHECK(object.dump(-1) == "{\"x\":1}");
    CHECK(json::parse(object.dump(4096)) == object);
    json nested = object;
    for (int i = 0; i < 20; ++i) {
        nested = json::object({{"child", nested}});
    }
    const auto output = nested.dump(32, '\t');
    CHECK(output.find('\0') == std::string::npos);
    CHECK(output.find("\n" + std::string(672, '\t') + "\"x\"") != std::string::npos);
    CHECK(json::parse(output) == nested);
}
