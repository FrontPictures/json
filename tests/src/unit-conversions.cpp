//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++ (supporting code)
// |  |  |__   |  |  | | | |  version 1.0.0
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// Copyright (c) 2013-2022 Niels Lohmann <http://nlohmann.me>.
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT


#include "doctest_compatibility.h"

#include "nlohmann/json.hpp"
using nlohmann::json;

#include <deque>
#include <forward_list>
#include <list>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <valarray>
#include <map>

// NLOHMANN_JSON_SERIALIZE_ENUM uses a static std::pair
DOCTEST_CLANG_SUPPRESS_WARNING_PUSH
DOCTEST_CLANG_SUPPRESS_WARNING("-Wexit-time-destructors")

TEST_CASE("value conversion")
{
    SECTION("get an object (explicit)")
    {
        const json::object_t o_reference = {{"object", json::object()},
            {"array", {1, 2, 3, 4}},
            {"number", 42},
            {"boolean", false},
            {"null", nullptr},
            {"string", "Hello world"}
        };
        json j(o_reference);

        SECTION("json::object_t")
        {
            json::object_t const o = j.get<json::object_t>();
            CHECK(json(o) == j);
        }





        SECTION("exception in case of a non-object type")
        {
            CHECK_THROWS_WITH_AS(json(json::value_t::null).get<json::object_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(json(json::value_t::array).get<json::object_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(json(json::value_t::string).get<json::object_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(json(json::value_t::boolean).get<json::object_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(json(json::value_t::number_integer).get<json::object_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(json(json::value_t::number_float).get<json::object_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
        }
    }








    SECTION("get an array (explicit)")
    {
        const json::array_t a_reference{json(1),     json(1u),       json(2.2),
                                        json(false), json("string"), json()};
        json j(a_reference);

        SECTION("json::array_t")
        {
            const json::array_t a = j.get<json::array_t>();
            CHECK(json(a) == j);
        }




        SECTION("std::vector<json>")
        {
            const std::vector<json> a = j.get<std::vector<json>>();
            CHECK(json(a) == j);

            CHECK_THROWS_WITH_AS(json(json::value_t::null).get<std::vector<json>>(), "[json.exception.type_error.302] invalid type", json::type_error&);

            SECTION("reserve is called on containers that supports it")
            {
                // make sure all values are properly copied
                const json j2({1, 2, 3, 4, 5, 6, 7, 8, 9, 10});
                auto v2 = j2.get<std::vector<int>>();
                CHECK(v2.size() == 10);
            }
        }


        //     const json j2 = nbs;
        //     const json j3 = str;



        SECTION("exception in case of a non-array type")
        {
            CHECK_THROWS_WITH_AS(json(json::value_t::object).get<std::vector<int>>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(json(json::value_t::null).get<json::array_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(json(json::value_t::object).get<json::array_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(json(json::value_t::string).get<json::array_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(json(json::value_t::boolean).get<json::array_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(json(json::value_t::number_integer).get<json::array_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(json(json::value_t::number_float).get<json::array_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
        }
    }











    SECTION("get a string (explicit)")
    {
        const json::string_t s_reference{"Hello world"};
        json j(s_reference);

        SECTION("string_t")
        {
            const json::string_t s = j.get<json::string_t>();
            CHECK(json(s) == j);
        }

        SECTION("std::string")
        {
            const std::string s = j.get<std::string>();
            CHECK(json(s) == j);
        }

        SECTION("exception in case of a non-string type")
        {
            CHECK_THROWS_WITH_AS(json(json::value_t::null).get<json::string_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(json(json::value_t::object).get<json::string_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(json(json::value_t::array).get<json::string_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(json(json::value_t::boolean).get<json::string_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(json(json::value_t::number_integer).get<json::string_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(json(json::value_t::number_float).get<json::string_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
        }

    }








    SECTION("get a boolean (explicit)")
    {
        const json::boolean_t b_reference{true};
        json j(b_reference);

        SECTION("boolean_t")
        {
            auto b = j.get<json::boolean_t>();
            CHECK(json(b) == j);
        }


        SECTION("bool")
        {
            const bool b = j.get<bool>();
            CHECK(json(b) == j);
        }

        SECTION("exception in case of a non-number type")
        {

            CHECK_THROWS_WITH_AS(json(json::value_t::null).get<json::boolean_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(json(json::value_t::object).get<json::boolean_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(json(json::value_t::array).get<json::boolean_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(json(json::value_t::string).get<json::boolean_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(json(json::value_t::number_integer).get<json::boolean_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(json(json::value_t::number_float).get<json::boolean_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
        }
    }


    SECTION("get an integer number (explicit)")
    {
        const json::number_integer_t n_reference{42};
        json j(n_reference);
        // const json::number_unsigned_t n_unsigned_reference{42u};
        // json j_unsigned(n_unsigned_reference);

        SECTION("number_integer_t")
        {
            auto n = j.get<json::number_integer_t>();
            CHECK(json(n) == j);
        }




        SECTION("int")
        {
            const int n = j.get<int>();
            CHECK(json(n) == j);
        }

        SECTION("unsigned int")
        {
            auto n = j.get<unsigned int>();
            CHECK(json(n) == j);
        }

        SECTION("long")
        {
            const long n = j.get<long>();
            CHECK(json(n) == j);
        }

        SECTION("unsigned long")
        {
            auto n = j.get<unsigned long>();
            CHECK(json(n) == j);
        }





        SECTION("int32_t")
        {
            auto n = j.get<int32_t>();
            CHECK(json(n) == j);
        }

        SECTION("int64_t")
        {
            auto n = j.get<int64_t>();
            CHECK(json(n) == j);
        }


        SECTION("int16_fast_t")
        {
            auto n = j.get<int_fast16_t>();
            CHECK(json(n) == j);
        }

        SECTION("int32_fast_t")
        {
            auto n = j.get<int_fast32_t>();
            CHECK(json(n) == j);
        }

        SECTION("int64_fast_t")
        {
            auto n = j.get<int_fast64_t>();
            CHECK(json(n) == j);
        }



        SECTION("int32_least_t")
        {
            auto n = j.get<int_least32_t>();
            CHECK(json(n) == j);
        }

        SECTION("int64_least_t")
        {
            auto n = j.get<int_least64_t>();
            CHECK(json(n) == j);
        }



        SECTION("uint32_t")
        {
            auto n = j.get<uint32_t>();
            CHECK(json(n) == j);
        }

        SECTION("uint64_t")
        {
            auto n = j.get<uint64_t>();
            CHECK(json(n) == j);
        }


        SECTION("uint16_fast_t")
        {
            auto n = j.get<uint_fast16_t>();
            CHECK(json(n) == j);
        }

        SECTION("uint32_fast_t")
        {
            auto n = j.get<uint_fast32_t>();
            CHECK(json(n) == j);
        }

        SECTION("uint64_fast_t")
        {
            auto n = j.get<uint_fast64_t>();
            CHECK(json(n) == j);
        }



        SECTION("uint32_least_t")
        {
            auto n = j.get<uint_least32_t>();
            CHECK(json(n) == j);
        }

        SECTION("uint64_least_t")
        {
            auto n = j.get<uint_least64_t>();
            CHECK(json(n) == j);
        }

        SECTION("exception in case of a non-number type")
        {
            CHECK_THROWS_WITH_AS(json(json::value_t::null).get<json::number_integer_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(json(json::value_t::object).get<json::number_integer_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(json(json::value_t::array).get<json::number_integer_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(json(json::value_t::string).get<json::number_integer_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK(json(json::value_t::boolean).get<json::number_integer_t>() == 0);

            CHECK_NOTHROW(
                json(json::value_t::number_float).get<json::number_integer_t>());
        }
    }


    SECTION("get a floating-point number (explicit)")
    {
        json::number_float_t const n_reference{42.23};
        json const j(n_reference);

        SECTION("number_float_t")
        {
            auto n = j.get<json::number_float_t>();
            CHECK(json(n).get<double>() == Approx(j.get<double>()));
        }

        SECTION("float")
        {
            auto n = j.get<float>();
            CHECK(json(n).get<double>() == Approx(j.get<double>()));
        }

        SECTION("double")
        {
            auto n = j.get<double>();
            CHECK(json(n).get<double>() == Approx(j.get<double>()));
        }

        SECTION("exception in case of a non-string type")
        {
            CHECK_THROWS_WITH_AS(json(json::value_t::null).get<json::number_float_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(json(json::value_t::object).get<json::number_float_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(json(json::value_t::array).get<json::number_float_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(json(json::value_t::string).get<json::number_float_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(json(json::value_t::boolean).get<json::number_float_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);

            CHECK_NOTHROW(
                json(json::value_t::number_integer).get<json::number_float_t>());
        }
    }


    SECTION("get a binary value (explicit)")
    {
        json::binary_t const n_reference{{1, 2, 3}};
        json j(n_reference);

        SECTION("binary_t")
        {
            json::binary_t const b = j.get<json::binary_t>();
            CHECK(json(b).get_binary() == j.get_binary());
        }

        SECTION("get_binary()")
        {
            SECTION("non-const")
            {
                auto& b = j.get_binary();
                CHECK(json(b).get_binary() == j.get_binary());
            }

            SECTION("non-const")
            {
                const json j_const = j;
                const auto& b = j_const.get_binary();
                CHECK(json(b).get_binary() == j.get_binary());
            }
        }

        SECTION("exception in case of a non-string type")
        {
            json j_null(json::value_t::null);
            json j_object(json::value_t::object);
            json j_array(json::value_t::array);
            json j_string(json::value_t::string);
            json j_boolean(json::value_t::boolean);
            const json j_null_const(json::value_t::null);
            const json j_object_const(json::value_t::object);
            const json j_array_const(json::value_t::array);
            const json j_string_const(json::value_t::string);
            const json j_boolean_const(json::value_t::boolean);

            CHECK_THROWS_WITH_AS(j_null.get<json::binary_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(j_object.get<json::binary_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(j_array.get<json::binary_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(j_string.get<json::binary_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(j_boolean.get<json::binary_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);

            CHECK_THROWS_WITH_AS(j_null_const.get<json::binary_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(j_object_const.get<json::binary_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(j_array_const.get<json::binary_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(j_string_const.get<json::binary_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);
            CHECK_THROWS_WITH_AS(j_boolean_const.get<json::binary_t>(), "[json.exception.type_error.302] invalid type", json::type_error&);

            CHECK_THROWS_WITH_AS(j_null.get_binary(), "[json.exception.type_error.302] type must be binary, but is null", json::type_error&);
            CHECK_THROWS_WITH_AS(j_object.get_binary(), "[json.exception.type_error.302] type must be binary, but is object", json::type_error&);
            CHECK_THROWS_WITH_AS(j_array.get_binary(), "[json.exception.type_error.302] type must be binary, but is array", json::type_error&);
            CHECK_THROWS_WITH_AS(j_string.get_binary(), "[json.exception.type_error.302] type must be binary, but is string", json::type_error&);
            CHECK_THROWS_WITH_AS(j_boolean.get_binary(), "[json.exception.type_error.302] type must be binary, but is boolean", json::type_error&);

            CHECK_THROWS_WITH_AS(j_null_const.get_binary(), "[json.exception.type_error.302] type must be binary, but is null", json::type_error&);
            CHECK_THROWS_WITH_AS(j_object_const.get_binary(), "[json.exception.type_error.302] type must be binary, but is object", json::type_error&);
            CHECK_THROWS_WITH_AS(j_array_const.get_binary(), "[json.exception.type_error.302] type must be binary, but is array", json::type_error&);
            CHECK_THROWS_WITH_AS(j_string_const.get_binary(), "[json.exception.type_error.302] type must be binary, but is string", json::type_error&);
            CHECK_THROWS_WITH_AS(j_boolean_const.get_binary(), "[json.exception.type_error.302] type must be binary, but is boolean", json::type_error&);
        }
    }


    SECTION("get an enum")
    {
        enum c_enum { value_1, value_2 };
        enum class cpp_enum { value_1, value_2 };

        CHECK(json(value_1).get<c_enum>() == value_1);
        CHECK(json(cpp_enum::value_1).get<cpp_enum>() == cpp_enum::value_1);
    }

    SECTION("more involved conversions")
    {
        SECTION("object-like STL containers")
        {
            json const j1 = {{"one", 1}, {"two", 2}, {"three", 3}};
            json const j2 = {{"one", 1u}, {"two", 2u}, {"three", 3u}};
            json const j3 = {{"one", 1.1}, {"two", 2.2}, {"three", 3.3}};
            json const j4 = {{"one", true}, {"two", false}, {"three", true}};
            json const j5 = {{"one", "eins"}, {"two", "zwei"}, {"three", "drei"}};





        }

        SECTION("array-like STL containers")
        {
            json const j1 = {1, 2, 3, 4};
            json const j2 = {1u, 2u, 3u, 4u};
            json const j3 = {1.2, 2.3, 3.4, 4.5};
            json const j4 = {true, false, true};
            json const j5 = {"one", "two", "three"};







            SECTION("std::vector")
            {
                j1.get<std::vector<int>>();
                // j2.get<std::vector<unsigned int>>();
                // j3.get<std::vector<double>>();
                // j4.get<std::vector<bool>>();
                j5.get<std::vector<std::string>>();
            }












        }
    }
}

// enum class cards {kreuz, pik, herz, karo};

// // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays) - false positive
// NLOHMANN_JSON_SERIALIZE_ENUM(cards,
// {
//     {cards::kreuz, "kreuz"},
//     {cards::pik, "pik"},
//     {cards::pik, "puk"},  // second entry for cards::puk; will not be used
//     {cards::herz, "herz"},
//     {cards::karo, "karo"}
// })

// enum TaskState
// {
//     TS_STOPPED,
//     TS_RUNNING,
//     TS_COMPLETED,
//     TS_INVALID = -1,
// };

// // NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays) - false positive
// NLOHMANN_JSON_SERIALIZE_ENUM(TaskState,
// {
//     {TS_INVALID, nullptr},
//     {TS_STOPPED, "stopped"},
//     {TS_RUNNING, "running"},
//     {TS_COMPLETED, "completed"},
// })







DOCTEST_CLANG_SUPPRESS_WARNING_POP
