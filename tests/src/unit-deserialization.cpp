//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++ (supporting code)
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// Copyright (c) 2013-2022 Niels Lohmann <http://nlohmann.me>.
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#include "doctest_compatibility.h"

#include "nlohmann/json.hpp"
using nlohmann::json;

#include <iostream>
#include <iterator>
#include <sstream>
#include <valarray>
#include "nlohmann/detail/input/json_sax.hpp"

namespace
{
struct SaxEventLogger : public nlohmann::json_sax
{
    bool null() override
    {
        events.emplace_back("null()");
        return true;
    }

    bool boolean(bool val) override
    {
        events.emplace_back(val ? "boolean(true)" : "boolean(false)");
        return true;
    }

    bool number_integer(json::number_integer_t val) override
    {
        events.push_back("number_integer(" + std::to_string(val) + ")");
        return true;
    }

    // bool number_unsigned(json::number_unsigned_t val) override
    // {
    //     events.push_back("number_integer(" + std::to_string(val) + ")");
    //     return true;
    // }

    bool number_float(json::number_float_t /*val*/, const std::string& s) override
    {
        events.push_back("number_float(" + s + ")");
        return true;
    }

    bool string(std::string& val) override
    {
        events.push_back("string(" + val + ")");
        return true;
    }

    bool binary(json::binary_t& val) override
    {
        std::string binary_contents = "binary(";
        std::string comma_space;
        for (auto b : val)
        {
            binary_contents.append(comma_space);
            binary_contents.append(std::to_string(static_cast<int>(b)));
            comma_space = ", ";
        }
        binary_contents.append(")");
        events.push_back(binary_contents);
        return true;
    }

    bool start_object(std::size_t elements) override
    {
        if (elements == static_cast<std::size_t>(-1))
        {
            events.emplace_back("start_object()");
        }
        else
        {
            events.push_back("start_object(" + std::to_string(elements) + ")");
        }
        return true;
    }

    bool key(std::string& val) override
    {
        events.push_back("key(" + val + ")");
        return true;
    }

    bool end_object() override
    {
        events.emplace_back("end_object()");
        return true;
    }

    bool start_array(std::size_t elements) override
    {
        if (elements == static_cast<std::size_t>(-1))
        {
            events.emplace_back("start_array()");
        }
        else
        {
            events.push_back("start_array(" + std::to_string(elements) + ")");
        }
        return true;
    }

    bool end_array() override
    {
        events.emplace_back("end_array()");
        return true;
    }

    bool parse_error(std::size_t position, const std::string& /*last_token*/, const json::exception& /*ex*/) override
    {
        events.push_back("parse_error(" + std::to_string(position) + ")");
        return false;
    }

    std::vector<std::string> events {};
};

struct SaxEventLoggerExitAfterStartObject : public SaxEventLogger
{
    bool start_object(std::size_t elements) override
    {
        if (elements == static_cast<std::size_t>(-1))
        {
            events.emplace_back("start_object()");
        }
        else
        {
            events.push_back("start_object(" + std::to_string(elements) + ")");
        }
        return false;
    }
};

struct SaxEventLoggerExitAfterKey : public SaxEventLogger
{
    bool key(std::string& val) override
    {
        events.push_back("key(" + val + ")");
        return false;
    }
};

struct SaxEventLoggerExitAfterStartArray : public SaxEventLogger
{
    bool start_array(std::size_t elements) override
    {
        if (elements == static_cast<std::size_t>(-1))
        {
            events.emplace_back("start_array()");
        }
        else
        {
            events.push_back("start_array(" + std::to_string(elements) + ")");
        }
        return false;
    }
};

template <typename T>
class proxy_iterator
{
  public:
    using iterator = typename T::iterator;
    using value_type = typename std::iterator_traits<iterator>::value_type;
    using reference = typename std::iterator_traits<iterator>::reference;
    using pointer = typename std::iterator_traits<iterator>::pointer;
    using difference_type =
        typename std::iterator_traits<iterator>::difference_type;
    using iterator_category = std::input_iterator_tag;

    proxy_iterator() = default;
    explicit proxy_iterator(iterator& it) : m_it(std::addressof(it)) {}

    proxy_iterator& operator++()
    {
        ++*m_it;
        return *this;
    }

    proxy_iterator& operator--()
    {
        --*m_it;
        return *this;
    }

    bool operator==(const proxy_iterator& rhs) const
    {
        return (m_it && rhs.m_it) ? (*m_it == *rhs.m_it) : (m_it == rhs.m_it);
    }

    bool operator!=(const proxy_iterator& rhs) const
    {
        return !(*this == rhs);
    }

    reference operator*() const
    {
        return **m_it;
    }

  private:
    iterator* m_it = nullptr;
};
} // namespace

TEST_CASE("deserialization")
{
    SECTION("successful deserialization")
    {
        SECTION("stream")
        {
            std::stringstream ss1;
            std::stringstream ss2;
            std::stringstream ss3;
            ss1 << R"(["foo",1,2,3,false,{"one":1}])";
            ss2 << R"(["foo",1,2,3,false,{"one":1}])";
            ss3 << R"(["foo",1,2,3,false,{"one":1}])";
            json j = json::parse(ss1.str());
            CHECK(json::accept(ss2.str()));
            CHECK(j == json({"foo", 1, 2, 3, false, {{"one", 1}}}));

            SaxEventLogger l;
            CHECK(json::sax_parse(ss3.str(), &l));
            CHECK(l.events.size() == 11);
            CHECK(l.events == std::vector<std::string>(
            {
                "start_array()", "string(foo)", "number_integer(1)",
                "number_integer(2)", "number_integer(3)", "boolean(false)",
                "start_object()", "key(one)", "number_integer(1)",
                "end_object()", "end_array()"
            }));
        }

        SECTION("string literal")
        {
            const auto* s = R"(["foo",1,2,3,false,{"one":1}])";
            json j = json::parse(s);
            CHECK(json::accept(s));
            CHECK(j == json({"foo", 1, 2, 3, false, {{"one", 1}}}));

            SaxEventLogger l;
            CHECK(json::sax_parse(s, &l));
            CHECK(l.events.size() == 11);
            CHECK(l.events == std::vector<std::string>(
            {
                "start_array()", "string(foo)", "number_integer(1)",
                "number_integer(2)", "number_integer(3)", "boolean(false)",
                "start_object()", "key(one)", "number_integer(1)",
                "end_object()", "end_array()"
            }));
        }

        SECTION("string_t")
        {
            json::string_t const s = R"(["foo",1,2,3,false,{"one":1}])";
            json j = json::parse(s);
            CHECK(json::accept(s));
            CHECK(j == json({"foo", 1, 2, 3, false, {{"one", 1}}}));

            SaxEventLogger l;
            CHECK(json::sax_parse(s, &l));
            CHECK(l.events.size() == 11);
            CHECK(l.events == std::vector<std::string>(
            {
                "start_array()", "string(foo)", "number_integer(1)",
                "number_integer(2)", "number_integer(3)", "boolean(false)",
                "start_object()", "key(one)", "number_integer(1)",
                "end_object()", "end_array()"
            }));
        }



        SECTION("user-defined string literal")
        {
            CHECK("[\"foo\",1,2,3,false,{\"one\":1}]"_json == json({"foo", 1, 2, 3, false, {{"one", 1}}}));
        }
    }

    SECTION("unsuccessful deserialization")
    {




        SECTION("string")
        {
            json::string_t const s = R"(["foo",1,2,3,false,{"one":1})";
            json _;
            CHECK_THROWS_WITH_AS(_ = json::parse(s), "[json.exception.parse_error.101] parse error at line 1, column 29: syntax error while parsing array - unexpected end of input; expected ']'", json::parse_error&);
            CHECK(!json::accept(s));

            json j_error;
            CHECK_NOTHROW(j_error = json::parse(s, nullptr, false));
            CHECK(j_error.is_discarded());

            SaxEventLogger l;
            CHECK(!json::sax_parse(s, &l));
            CHECK(l.events.size() == 11);
            CHECK(l.events == std::vector<std::string>(
            {
                "start_array()", "string(foo)", "number_integer(1)",
                "number_integer(2)", "number_integer(3)", "boolean(false)",
                "start_object()", "key(one)", "number_integer(1)",
                "end_object()", "parse_error(29)"
            }));
        }



        SECTION("user-defined string literal")
        {
            CHECK_THROWS_WITH_AS("[\"foo\",1,2,3,false,{\"one\":1}"_json, "[json.exception.parse_error.101] parse error at line 1, column 29: syntax error while parsing array - unexpected end of input; expected ']'", json::parse_error&);
        }
    }

    SECTION("contiguous containers")
    {
        SECTION("directly")
        {
            SECTION("from std::vector")
            {
                std::vector<uint8_t> const v = {'t', 'r', 'u', 'e'};
                CHECK(json::parse(v) == json(true));
                CHECK(json::accept(v));

                SaxEventLogger l;
                CHECK(json::sax_parse(v, &l));
                CHECK(l.events.size() == 1);
                CHECK(l.events == std::vector<std::string>({"boolean(true)"}));
            }







            //     delete[] v; // NOLINT(cppcoreguidelines-owning-memory)
            // }

            SECTION("from std::string")
            {
                std::string const v = {'t', 'r', 'u', 'e'};
                CHECK(json::parse(v) == json(true));
                CHECK(json::accept(v));

                SaxEventLogger l;
                CHECK(json::sax_parse(v, &l));
                CHECK(l.events.size() == 1);
                CHECK(l.events == std::vector<std::string>({"boolean(true)"}));
            }



            SECTION("empty container")
            {
                std::vector<uint8_t> const v;
                json _;
                CHECK_THROWS_AS(_ = json::parse(v), json::parse_error&);
                CHECK(!json::accept(v));

                SaxEventLogger l;
                CHECK(!json::sax_parse(v, &l));
                CHECK(l.events.size() == 1);
                CHECK(l.events == std::vector<std::string>({"parse_error(1)"}));
            }
        }



        //     }















        //         auto first = str.begin();
        //         auto last = str.end();
        //         json j;
        //         json_sax_dom_parser sax(j, true);


        // these cases are required for 100% line coverage
        SECTION("error cases")
        {
            SECTION("case 1")
            {
                std::vector<std::uint8_t> v = {{'\"', 'a', 'a', 'a', 'a', 'a', 'a', '\\', 'u'}};
                json _;
                CHECK_THROWS_AS(_ = json::parse(v), json::parse_error&);
                CHECK(!json::accept(v));

                json j_error;
                CHECK_NOTHROW(j_error = json::parse(v, nullptr, false));
                CHECK(j_error.is_discarded());

                SaxEventLogger l;
                CHECK(!json::sax_parse(v, &l));
                CHECK(l.events.size() == 1);
                CHECK(l.events == std::vector<std::string>({"parse_error(10)"}));
            }

            SECTION("case 2")
            {
                std::vector<std::uint8_t> v = {{'\"', 'a', 'a', 'a', 'a', 'a', 'a', '\\', 'u', '1'}};
                json _;
                CHECK_THROWS_AS(_ = json::parse(v), json::parse_error&);
                CHECK(!json::accept(v));

                json j_error;
                CHECK_NOTHROW(j_error = json::parse(v, nullptr, false));
                CHECK(j_error.is_discarded());

                SaxEventLogger l;
                CHECK(!json::sax_parse(v, &l));
                CHECK(l.events.size() == 1);
                CHECK(l.events == std::vector<std::string>({"parse_error(11)"}));
            }

            SECTION("case 3")
            {
                std::vector<std::uint8_t> v = {{'\"', 'a', 'a', 'a', 'a', 'a', 'a', '\\', 'u', '1', '1', '1', '1', '1', '1', '1', '1'}};
                json _;
                CHECK_THROWS_AS(_ = json::parse(v), json::parse_error&);
                CHECK(!json::accept(v));

                json j_error;
                CHECK_NOTHROW(j_error = json::parse(v, nullptr, false));
                CHECK(j_error.is_discarded());

                SaxEventLogger l;
                CHECK(!json::sax_parse(v, &l));
                CHECK(l.events.size() == 1);
                CHECK(l.events == std::vector<std::string>({"parse_error(18)"}));
            }

            SECTION("case 4")
            {
                std::vector<std::uint8_t> v = {{'\"', 'a', 'a', 'a', 'a', 'a', 'a', 'u', '1', '1', '1', '1', '1', '1', '1', '1', '\\'}};
                json _;
                CHECK_THROWS_AS(_ = json::parse(v), json::parse_error&);
                CHECK(!json::accept(v));

                json j_error;
                CHECK_NOTHROW(j_error = json::parse(v, nullptr, false));
                CHECK(j_error.is_discarded());

                SaxEventLogger l;
                CHECK(!json::sax_parse(v, &l));
                CHECK(l.events.size() == 1);
                CHECK(l.events == std::vector<std::string>({"parse_error(18)"}));
            }

            SECTION("case 5")
            {
                std::vector<std::uint8_t> v = {{'\"', 0x7F, 0xC1}};
                json _;
                CHECK_THROWS_AS(_ = json::parse(v), json::parse_error&);
                CHECK(!json::accept(v));

                json j_error;
                CHECK_NOTHROW(j_error = json::parse(v, nullptr, false));
                CHECK(j_error.is_discarded());

                SaxEventLogger l;
                CHECK(!json::sax_parse(v, &l));
                CHECK(l.events.size() == 1);
                CHECK(l.events == std::vector<std::string>({"parse_error(3)"}));
            }

            SECTION("case 6")
            {
                std::vector<std::uint8_t> v = {{'\"', 0x7F, 0xDF, 0x7F}};
                json _;
                CHECK_THROWS_WITH_AS(_ = json::parse(v), "[json.exception.parse_error.101] parse error at line 1, column 4: syntax error while parsing value - invalid string: ill-formed UTF-8 byte; last read: '\"\x7f\xdf\x7f'", json::parse_error&);
                CHECK(!json::accept(v));

                json j_error;
                CHECK_NOTHROW(j_error = json::parse(v, nullptr, false));
                CHECK(j_error.is_discarded());

                SaxEventLogger l;
                CHECK(!json::sax_parse(v, &l));
                CHECK(l.events.size() == 1);
                CHECK(l.events == std::vector<std::string>({"parse_error(4)"}));
            }

            SECTION("case 7")
            {
                std::vector<std::uint8_t> v = {{'\"', 0x7F, 0xDF, 0xC0}};
                json _;
                CHECK_THROWS_AS(_ = json::parse(v), json::parse_error&);
                CHECK(!json::accept(v));

                json j_error;
                CHECK_NOTHROW(j_error = json::parse(v, nullptr, false));
                CHECK(j_error.is_discarded());

                SaxEventLogger l;
                CHECK(!json::sax_parse(v, &l));
                CHECK(l.events.size() == 1);
                CHECK(l.events == std::vector<std::string>({"parse_error(4)"}));
            }

            SECTION("case 8")
            {
                std::vector<std::uint8_t> v = {{'\"', 0x7F, 0xE0, 0x9F}};
                json _;
                CHECK_THROWS_AS(_ = json::parse(v), json::parse_error&);
                CHECK(!json::accept(v));

                json j_error;
                CHECK_NOTHROW(j_error = json::parse(v, nullptr, false));
                CHECK(j_error.is_discarded());

                SaxEventLogger l;
                CHECK(!json::sax_parse(v, &l));
                CHECK(l.events.size() == 1);
                CHECK(l.events == std::vector<std::string>({"parse_error(4)"}));
            }

            SECTION("case 9")
            {
                std::vector<std::uint8_t> v = {{'\"', 0x7F, 0xEF, 0xC0}};
                json _;
                CHECK_THROWS_AS(_ = json::parse(v), json::parse_error&);
                CHECK(!json::accept(v));

                json j_error;
                CHECK_NOTHROW(j_error = json::parse(v, nullptr, false));
                CHECK(j_error.is_discarded());

                SaxEventLogger l;
                CHECK(!json::sax_parse(v, &l));
                CHECK(l.events.size() == 1);
                CHECK(l.events == std::vector<std::string>({"parse_error(4)"}));
            }

            SECTION("case 10")
            {
                std::vector<std::uint8_t> v = {{'\"', 0x7F, 0xED, 0x7F}};
                json _;
                CHECK_THROWS_AS(_ = json::parse(v), json::parse_error&);
                CHECK(!json::accept(v));

                json j_error;
                CHECK_NOTHROW(j_error = json::parse(v, nullptr, false));
                CHECK(j_error.is_discarded());

                SaxEventLogger l;
                CHECK(!json::sax_parse(v, &l));
                CHECK(l.events.size() == 1);
                CHECK(l.events == std::vector<std::string>({"parse_error(4)"}));
            }

            SECTION("case 11")
            {
                std::vector<std::uint8_t> v = {{'\"', 0x7F, 0xF0, 0x8F}};
                json _;
                CHECK_THROWS_AS(_ = json::parse(v), json::parse_error&);
                CHECK(!json::accept(v));

                json j_error;
                CHECK_NOTHROW(j_error = json::parse(v, nullptr, false));
                CHECK(j_error.is_discarded());

                SaxEventLogger l;
                CHECK(!json::sax_parse(v, &l));
                CHECK(l.events.size() == 1);
                CHECK(l.events == std::vector<std::string>({"parse_error(4)"}));
            }

            SECTION("case 12")
            {
                std::vector<std::uint8_t> v = {{'\"', 0x7F, 0xF0, 0xC0}};
                json _;
                CHECK_THROWS_AS(_ = json::parse(v), json::parse_error&);
                CHECK(!json::accept(v));

                json j_error;
                CHECK_NOTHROW(j_error = json::parse(v, nullptr, false));
                CHECK(j_error.is_discarded());

                SaxEventLogger l;
                CHECK(!json::sax_parse(v, &l));
                CHECK(l.events.size() == 1);
                CHECK(l.events == std::vector<std::string>({"parse_error(4)"}));
            }

            SECTION("case 13")
            {
                std::vector<std::uint8_t> v = {{'\"', 0x7F, 0xF3, 0x7F}};
                json _;
                CHECK_THROWS_AS(_ = json::parse(v), json::parse_error&);
                CHECK(!json::accept(v));

                json j_error;
                CHECK_NOTHROW(j_error = json::parse(v, nullptr, false));
                CHECK(j_error.is_discarded());

                SaxEventLogger l;
                CHECK(!json::sax_parse(v, &l));
                CHECK(l.events.size() == 1);
                CHECK(l.events == std::vector<std::string>({"parse_error(4)"}));
            }

            SECTION("case 14")
            {
                std::vector<std::uint8_t> v = {{'\"', 0x7F, 0xF3, 0xC0}};
                json _;
                CHECK_THROWS_AS(_ = json::parse(v), json::parse_error&);
                CHECK(!json::accept(v));

                json j_error;
                CHECK_NOTHROW(j_error = json::parse(v, nullptr, false));
                CHECK(j_error.is_discarded());

                SaxEventLogger l;
                CHECK(!json::sax_parse(v, &l));
                CHECK(l.events.size() == 1);
                CHECK(l.events == std::vector<std::string>({"parse_error(4)"}));
            }

            SECTION("case 15")
            {
                std::vector<std::uint8_t> v = {{'\"', 0x7F, 0xF4, 0x7F}};
                json _;
                CHECK_THROWS_AS(_ = json::parse(v), json::parse_error&);
                CHECK(!json::accept(v));

                json j_error;
                CHECK_NOTHROW(j_error = json::parse(v, nullptr, false));
                CHECK(j_error.is_discarded());

                SaxEventLogger l;
                CHECK(!json::sax_parse(v, &l));
                CHECK(l.events.size() == 1);
                CHECK(l.events == std::vector<std::string>({"parse_error(4)"}));
            }

            SECTION("case 16")
            {
                std::vector<std::uint8_t> v = {{'{', '\"', '\"', ':', '1', '1'}};
                json _;
                CHECK_THROWS_AS(_ = json::parse(v), json::parse_error&);
                CHECK(!json::accept(v));

                json j_error;
                CHECK_NOTHROW(j_error = json::parse(v, nullptr, false));
                CHECK(j_error.is_discarded());

                SaxEventLogger l;
                CHECK(!json::sax_parse(v, &l));
                CHECK(l.events.size() == 4);
                CHECK(l.events == std::vector<std::string>(
                {
                    "start_object()", "key()", "number_integer(11)",
                    "parse_error(7)"
                }));
            }
        }
    }

    SECTION("ignoring byte-order marks")
    {
        std::string bom = "\xEF\xBB\xBF";

        SECTION("BOM only")
        {
            json _;
            CHECK_THROWS_WITH_AS(_ = json::parse(bom), "[json.exception.parse_error.101] parse error at line 1, column 4: syntax error while parsing value - unexpected end of input; expected '[', '{', or a literal", json::parse_error&);


            SaxEventLogger l;
            CHECK(!json::sax_parse(bom, &l));
            CHECK(l.events.size() == 1);
            CHECK(l.events == std::vector<std::string>(
            {
                "parse_error(4)"
            }));
        }

        SECTION("BOM and content")
        {
            CHECK(json::parse(bom + "1") == 1);

            // SaxEventLogger l1;
            SaxEventLogger l2;
            CHECK(json::sax_parse(bom + "1", &l2));
            CHECK(l2.events.size() == 1);
            CHECK(l2.events == std::vector<std::string>(
            {
                "number_integer(1)"
            }));
        }

        SECTION("2 byte of BOM")
        {
            json _;
            CHECK_THROWS_WITH_AS(_ = json::parse(bom.substr(0, 2)), "[json.exception.parse_error.101] parse error at line 1, column 3: syntax error while parsing value - invalid BOM; must be 0xEF 0xBB 0xBF if given; last read: '\xEF\xBB'", json::parse_error&);


            // SaxEventLogger l1;
            SaxEventLogger l2;
            CHECK(!json::sax_parse(bom.substr(0, 2), &l2));
            CHECK(l2.events.size() == 1);
            CHECK(l2.events == std::vector<std::string>(
            {
                "parse_error(3)"
            }));
        }

        SECTION("1 byte of BOM")
        {
            json _;
            CHECK_THROWS_WITH_AS(_ = json::parse(bom.substr(0, 1)), "[json.exception.parse_error.101] parse error at line 1, column 2: syntax error while parsing value - invalid BOM; must be 0xEF 0xBB 0xBF if given; last read: '\xEF'", json::parse_error&);


            // SaxEventLogger l1;
            SaxEventLogger l2;
            CHECK(!json::sax_parse(bom.substr(0, 1), &l2));
            CHECK(l2.events.size() == 1);
            CHECK(l2.events == std::vector<std::string>(
            {
                "parse_error(2)"
            }));
        }

        SECTION("variations")
        {
            // calculate variations of each byte of the BOM to make sure
            // that the BOM and only the BOM is skipped
            for (int i0 = -1; i0 < 2; ++i0)
            {
                for (int i1 = -1; i1 < 2; ++i1)
                {
                    for (int i2 = -1; i2 < 2; ++i2)
                    {
                        // debug output for the variations
                        CAPTURE(i0)
                        CAPTURE(i1)
                        CAPTURE(i2)

                        std::string s;
                        s.push_back(static_cast<char>(bom[0] + i0));
                        s.push_back(static_cast<char>(bom[1] + i1));
                        s.push_back(static_cast<char>(bom[2] + i2));

                        if (i0 == 0 && i1 == 0 && i2 == 0)
                        {
                            // without any variation, we skip the BOM
                            CHECK(json::parse(s + "null") == json());

                            SaxEventLogger l;
                            CHECK(json::sax_parse(s + "null", &l));
                            CHECK(l.events.size() == 1);
                            CHECK(l.events == std::vector<std::string>(
                            {
                                "null()"
                            }));
                        }
                        else
                        {
                            // any variation is an error
                            json _;
                            CHECK_THROWS_AS(_ = json::parse(s + "null"), json::parse_error&);

                            SaxEventLogger l;
                            CHECK(!json::sax_parse(s + "null", &l));
                            CHECK(l.events.size() == 1);

                            if (i0 != 0)
                            {
                                CHECK(l.events == std::vector<std::string>(
                                {
                                    "parse_error(1)"
                                }));
                            }
                            else if (i1 != 0)
                            {
                                CHECK(l.events == std::vector<std::string>(
                                {
                                    "parse_error(2)"
                                }));
                            }
                            else
                            {
                                CHECK(l.events == std::vector<std::string>(
                                {
                                    "parse_error(3)"
                                }));
                            }
                        }
                    }
                }
            }
        }

    }

    SECTION("SAX and early abort")
    {
        std::string const s = R"([1, ["string", 43.12], null, {"key1": true, "key2": false}])";

        SaxEventLogger default_logger;
        SaxEventLoggerExitAfterStartObject exit_after_start_object;
        SaxEventLoggerExitAfterKey exit_after_key;
        SaxEventLoggerExitAfterStartArray exit_after_start_array;

        json::sax_parse(s, &default_logger);
        CHECK(default_logger.events.size() == 14);
        CHECK(default_logger.events == std::vector<std::string>(
        {
            "start_array()", "number_integer(1)", "start_array()",
            "string(string)", "number_float(43.12)", "end_array()", "null()",
            "start_object()", "key(key1)", "boolean(true)", "key(key2)",
            "boolean(false)", "end_object()", "end_array()"
        }));

        json::sax_parse(s, &exit_after_start_object);
        CHECK(exit_after_start_object.events.size() == 8);
        CHECK(exit_after_start_object.events == std::vector<std::string>(
        {
            "start_array()", "number_integer(1)", "start_array()",
            "string(string)", "number_float(43.12)", "end_array()", "null()",
            "start_object()"
        }));

        json::sax_parse(s, &exit_after_key);
        CHECK(exit_after_key.events.size() == 9);
        CHECK(exit_after_key.events == std::vector<std::string>(
        {
            "start_array()", "number_integer(1)", "start_array()",
            "string(string)", "number_float(43.12)", "end_array()", "null()",
            "start_object()", "key(key1)"
        }));

        json::sax_parse(s, &exit_after_start_array);
        CHECK(exit_after_start_array.events.size() == 1);
        CHECK(exit_after_start_array.events == std::vector<std::string>(
        {
            "start_array()"
        }));
    }

    SECTION("JSON Lines")
    {
        SECTION("Example file")
        {
            std::stringstream ss;
            ss << R"({"name": "Gilbert", "wins": [["straight", "7♣"], ["one pair", "10♥"]]}
                    {"name": "Alexa", "wins": [["two pair", "4♠"], ["two pair", "9♠"]]}
                    {"name": "May", "wins": []}
                    {"name": "Deloise", "wins": [["three of a kind", "5♣"]]}
)";

            std::string line;
            int object_count = 0;
            while (std::getline(ss, line))
            {
                ++object_count;
                CHECK(json::accept(line));
            }

            CHECK(object_count == 4);
        }

        SECTION("Example file without trailing newline")
        {
            std::stringstream ss;
            ss << R"({"name": "Gilbert", "wins": [["straight", "7♣"], ["one pair", "10♥"]]}
                    {"name": "Alexa", "wins": [["two pair", "4♠"], ["two pair", "9♠"]]}
                    {"name": "May", "wins": []}
                    {"name": "Deloise", "wins": [["three of a kind", "5♣"]]})";

            std::string line;
            int object_count = 0;
            while (std::getline(ss, line))
            {
                ++object_count;
                CHECK(json::accept(line));
            }

            CHECK(object_count == 4);
        }
    }
}
