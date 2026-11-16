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

TEST_CASE("value-initialized iterators compare equal")
{
    json::iterator a{}, b{};
    json::const_iterator c{};
    CHECK(a == b);
    CHECK(a == c);
    CHECK_FALSE(a != b);
    CHECK_FALSE(a < b);
    CHECK(a <= b);
    CHECK_FALSE(a > b);
    CHECK(a >= b);
    json value = {1, 2};
    CHECK_THROWS_AS(a == value.begin(), json::invalid_iterator);
    CHECK_THROWS_AS(a < value.begin(), json::invalid_iterator);
}

TEST_CASE("pretty arrays group primitive values on one line")
{
    CHECK(json::array().dump(2) == "[]");
    CHECK(json::array({1, true, nullptr, "short", json::object(), json::array()}).dump(2)
          == "[1, true, null, \"short\", {}, []]");
    CHECK(json::array({1, json::object({{"x", 2}}), 3}).dump(2)
          == "[1,\n  {\n    \"x\": 2\n  },\n  3\n]");
    CHECK(json::array({json::array({1, 2}), json::array({3, 4})}).dump(2)
          == "[\n  [1, 2],\n  [3, 4]\n]");
    CHECK(json::object({{"values", json::array({1, 2})}}).dump(2)
          == "{\n  \"values\": [1, 2]\n}");
    const json strings = json::array({std::string(20, 'a'), std::string(21, 'b'), "end"});
    CHECK(strings.dump(2) == "[\"" + std::string(20, 'a') + "\",\n  \""
          + std::string(21, 'b') + "\",\n  \"end\"\n]");
    // The threshold counts UTF-8 bytes, before JSON escaping.
    const json unicode = json::array({"éééééééééé", "ééééééééééé"});
    CHECK(unicode.dump(2) == "[\"éééééééééé\",\n  \"ééééééééééé\"\n]");
    CHECK(json::array({"\"\\\n", "é"}).dump(2, ' ', true)
          == "[\"\\\"\\\\\\n\", \"\\u00e9\"]");
    for (const auto &value : {strings, unicode, json::array({1, json::object({{"x", 2}}), 3})}) {
        CHECK(json::parse(value.dump(2)) == value);
        CHECK(json::parse(value.dump(0, ' ', true)) == value);
        CHECK(json::parse(value.dump(2, '\t')) == value);
        CHECK(json::parse(value.dump()) == value);
    }
}

TEST_CASE("fixed signed integer storage and object reservation")
{
    CHECK(json((std::numeric_limits<uint64_t>::max)()).get<int64_t>() == -1);
    CHECK(json(-1).get<uint64_t>() == (std::numeric_limits<uint64_t>::max)());
    CHECK(json(short(42)).get<short>() == 42);
    CHECK(json(42L).get<long>() == 42);
    CHECK(json(42LL).get<long long>() == 42);
    CHECK(json(42U).get<uint32_t>() == 42);
    CHECK(json(42).get<int32_t>() == 42);
    json object = {{"first", 1}, {"second", 2}};
    const auto before = object.dump();
    object.reserve_object_items(size_t(100));
    CHECK(object.dump() == before);
    object["third"] = 3;
    CHECK(object.dump() == "{\"first\":1,\"second\":2,\"third\":3}");
    CHECK_THROWS_AS(json::array().reserve_object_items(10), json::type_error);
}

TEST_CASE("items iterators support forward traversal")
{
    json object = {{"a", 1}, {"b", 2}};
    auto items = object.items();
    using iterator = decltype(items.begin());
    static_assert(std::is_same<typename std::iterator_traits<iterator>::iterator_category,
                              std::forward_iterator_tag>::value, "items is a forward range");
    auto first = items.begin();
    const auto copy = first;
    ++first;
    CHECK(copy.key() == "a");
    CHECK(first.key() == "b");
    CHECK(std::distance(items.begin(), items.end()) == 2);
}
