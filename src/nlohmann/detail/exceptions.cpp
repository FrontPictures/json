#include "exceptions.hpp"
#include "string_concat.hpp"

std::string nlohmann::detail::exception::name(const std::string &ename, int id_)
{
    return concat("[json.exception.", ename, '.', std::to_string(id_), "] ");
}

nlohmann::detail::parse_error nlohmann::detail::parse_error::create(int id_, const position_t &pos,
                                                                    const std::string &what_arg)
{
    const std::string w = concat(exception::name("parse_error", id_), "parse error",
                                 position_string(pos), ": ", what_arg);
    return {id_, pos.chars_read_total, w.c_str()};
}

nlohmann::detail::parse_error nlohmann::detail::parse_error::create(int id_, size_t byte_,
                                                                    const std::string &what_arg)
{
    const std::string w = concat(exception::name("parse_error", id_), "parse error",
                                 (byte_ != 0 ? (concat(" at byte ", std::to_string(byte_))) : ""),
                                 ": ", what_arg);
    return {id_, byte_, w.c_str()};
}

nlohmann::detail::parse_error::parse_error(int id_, size_t byte_, const char *what_arg)
    : exception(id_, what_arg), byte(byte_)
{}

std::string nlohmann::detail::parse_error::position_string(const position_t &pos)
{
    return concat(" at line ", std::to_string(pos.lines_read + 1), ", column ",
                  std::to_string(pos.chars_read_current_line));
}

nlohmann::detail::invalid_iterator nlohmann::detail::invalid_iterator::create(
    int id_, const std::string &what_arg)
{
    const std::string w = concat(exception::name("invalid_iterator", id_), what_arg);
    return {id_, w.c_str()};
}

nlohmann::detail::type_error nlohmann::detail::type_error::create(int id_,
                                                                  const std::string &what_arg)
{
    const std::string w = concat(exception::name("type_error", id_), what_arg);
    return {id_, w.c_str()};
}

nlohmann::detail::out_of_range nlohmann::detail::out_of_range::create(int id_,
                                                                      const std::string &what_arg)
{
    const std::string w = concat(exception::name("out_of_range", id_), what_arg);
    return {id_, w.c_str()};
}

nlohmann::detail::other_error nlohmann::detail::other_error::create(int id_,
                                                                    const std::string &what_arg)
{
    const std::string w = concat(exception::name("other_error", id_), what_arg);
    return {id_, w.c_str()};
}
