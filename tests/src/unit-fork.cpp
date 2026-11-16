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
