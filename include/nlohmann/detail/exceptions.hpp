//     __ _____ _____ _____
//  __|  |   __|     |   | |  JSON for Modern C++
// |  |  |__   |  |  | | | |  version 3.11.3
// |_____|_____|_____|_|___|  https://github.com/nlohmann/json
//
// SPDX-FileCopyrightText: 2013-2023 Niels Lohmann <https://nlohmann.me>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstddef>
#include <exception>
#include <stdexcept>
#include <string>
#include "input/position_t.hpp"

namespace nlohmann {
namespace detail {

class exception : public std::exception
{
public:
    const char *what() const noexcept override { return m.what(); }
    const int id;

protected:
    exception(int id_, const char *what_arg) : id(id_), m(what_arg) {}
    static std::string name(const std::string &ename, int id_);

private:
    std::runtime_error m;
};

class parse_error : public exception
{
public:
    static parse_error create(int id_, const position_t &pos, const std::string &what_arg);
    static parse_error create(int id_, size_t byte_, const std::string &what_arg);

    const size_t byte;

private:
    parse_error(int id_, size_t byte_, const char *what_arg);
    static std::string position_string(const position_t &pos);
};

class invalid_iterator : public exception
{
public:
    static invalid_iterator create(int id_, const std::string &what_arg);

private:
    invalid_iterator(int id_, const char *what_arg) : exception(id_, what_arg) {}
};

class type_error : public exception
{
public:
    static type_error create(int id_, const std::string &what_arg);

private:
    type_error(int id_, const char *what_arg) : exception(id_, what_arg) {}
};

class out_of_range : public exception
{
public:
    static out_of_range create(int id_, const std::string &what_arg);

private:
    out_of_range(int id_, const char *what_arg) : exception(id_, what_arg) {}
};

class other_error : public exception
{
public:
    static other_error create(int id_, const std::string &what_arg);

private:
    other_error(int id_, const char *what_arg) : exception(id_, what_arg) {}
};

} // namespace detail
} // namespace nlohmann
