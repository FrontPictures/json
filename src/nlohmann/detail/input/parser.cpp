#include "parser.hpp"
#include <cmath>
#include "nlohmann/json.hpp"
#include "json_sax.hpp"
#include "nlohmann/detail/string_concat.hpp"
#include "input_adapters.hpp"
#include "lexer.hpp"

nlohmann::detail::parser::parser(const iterator_input_adapter &adapter, const parser_callback_t cb,
                                 const bool allow_exceptions_, const bool skip_comments)
    : callback(cb), m_lexer(new lexer(adapter, skip_comments)), allow_exceptions(allow_exceptions_)
{
    get_token();
}

nlohmann::detail::parser::~parser() = default;

void nlohmann::detail::parser::parse(const bool strict, ordered_json &result)
{
    if (callback) {
        json_sax_dom_callback_parser sdp(result, callback, allow_exceptions);
        sax_parse_internal(&sdp);

        if (strict && (get_token() != lexer_token_type::end_of_input)) {
            sdp.parse_error(m_lexer->get_position(), m_lexer->get_token_string(),
                            parse_error::create(101, m_lexer->get_position(),
                                                exception_message(lexer_token_type::end_of_input,
                                                                  "value")));
        }

        if (sdp.is_errored()) {
            result = value_t::discarded;
            return;
        }

        if (result.is_discarded()) {
            result = nullptr;
        }
    } else {
        json_sax_dom_parser sdp(result, allow_exceptions);
        sax_parse_internal(&sdp);

        if (strict && (get_token() != lexer_token_type::end_of_input)) {
            sdp.parse_error(m_lexer->get_position(), m_lexer->get_token_string(),
                            parse_error::create(101, m_lexer->get_position(),
                                                exception_message(lexer_token_type::end_of_input,
                                                                  "value")));
        }

        if (sdp.is_errored()) {
            result = value_t::discarded;
            return;
        }
    }
}

bool nlohmann::detail::parser::accept(const bool strict)
{
    json_sax_acceptor sax_acceptor;
    return sax_parse(&sax_acceptor, strict);
}

bool nlohmann::detail::parser::sax_parse(json_sax *sax, const bool strict)
{
    const bool result = sax_parse_internal(sax);

    if (result && strict && (get_token() != lexer_token_type::end_of_input)) {
        return sax->parse_error(m_lexer->get_position(), m_lexer->get_token_string(),
                                parse_error::create(101, m_lexer->get_position(),
                                                    exception_message(lexer_token_type::end_of_input,
                                                                      "value")));
    }

    return result;
}

bool nlohmann::detail::parser::sax_parse_internal(json_sax *sax)
{
    std::vector<bool> states;

    bool skip_to_state_evaluation = false;

    while (true) {
        if (!skip_to_state_evaluation) {
            switch (last_token) {
            case lexer_token_type::begin_object: {
                if (!sax->start_object(static_cast<size_t>(-1))) {
                    return false;
                }

                if (get_token() == lexer_token_type::end_object) {
                    if (!sax->end_object()) {
                        return false;
                    }
                    break;
                }

                if (last_token != lexer_token_type::value_string) {
                    return sax
                        ->parse_error(m_lexer->get_position(), m_lexer->get_token_string(),
                                      parse_error::create(101, m_lexer->get_position(),
                                                          exception_message(lexer_token_type::value_string,
                                                                            "object key")));
                }
                if (!sax->key(m_lexer->get_string())) {
                    return false;
                }

                if (get_token() != lexer_token_type::name_separator) {
                    return sax->parse_error(
                        m_lexer->get_position(), m_lexer->get_token_string(),
                        parse_error::create(101, m_lexer->get_position(),
                                            exception_message(lexer_token_type::name_separator,
                                                              "object separator")));
                }

                states.push_back(false);

                get_token();
                continue;
            }

            case lexer_token_type::begin_array: {
                if (!sax->start_array(static_cast<size_t>(-1))) {
                    return false;
                }

                if (get_token() == lexer_token_type::end_array) {
                    if (!sax->end_array()) {
                        return false;
                    }
                    break;
                }

                states.push_back(true);

                continue;
            }

            case lexer_token_type::value_float: {
                const auto res = m_lexer->get_number_float();

                if (!std::isfinite(res)) {
                    return sax
                        ->parse_error(m_lexer->get_position(), m_lexer->get_token_string(),
                                      out_of_range::create(406, concat("number overflow parsing '",
                                                                       m_lexer->get_token_string(),
                                                                       '\'')));
                }

                if (!sax->number_float(res, m_lexer->get_string())) {
                    return false;
                }

                break;
            }

            case lexer_token_type::literal_false: {
                if (!sax->boolean(false)) {
                    return false;
                }
                break;
            }

            case lexer_token_type::literal_null: {
                if (!sax->null()) {
                    return false;
                }
                break;
            }

            case lexer_token_type::literal_true: {
                if (!sax->boolean(true)) {
                    return false;
                }
                break;
            }

            case lexer_token_type::value_integer: {
                if (!sax->number_integer(m_lexer->get_number_integer())) {
                    return false;
                }
                break;
            }

            case lexer_token_type::value_string: {
                if (!sax->string(m_lexer->get_string())) {
                    return false;
                }
                break;
            }

            case lexer_token_type::parse_error: {
                return sax
                    ->parse_error(m_lexer->get_position(), m_lexer->get_token_string(),
                                  parse_error::create(101, m_lexer->get_position(),
                                                      exception_message(lexer_token_type::uninitialized,
                                                                        "value")));
            }
            case lexer_token_type::end_of_input: {
                if (m_lexer->get_position().chars_read_total == 1) {
                    return sax->parse_error(
                        m_lexer->get_position(), m_lexer->get_token_string(),
                        parse_error::create(
                            101, m_lexer->get_position(),
                            "attempting to parse an empty input; check that your input string "
                            "or stream contains the expected JSON"));
                }

                return sax->parse_error(
                    m_lexer->get_position(), m_lexer->get_token_string(),
                    parse_error::create(101, m_lexer->get_position(),
                                        exception_message(lexer_token_type::literal_or_value, "value")));
            }
            case lexer_token_type::uninitialized:
            case lexer_token_type::end_array:
            case lexer_token_type::end_object:
            case lexer_token_type::name_separator:
            case lexer_token_type::value_separator:
            case lexer_token_type::literal_or_value:
            default: {
                return sax->parse_error(
                    m_lexer->get_position(), m_lexer->get_token_string(),
                    parse_error::create(101, m_lexer->get_position(),
                                        exception_message(lexer_token_type::literal_or_value, "value")));
            }
            }
        } else {
            skip_to_state_evaluation = false;
        }

        if (states.empty()) {
            return true;
        }

        if (states.back()) {
            if (get_token() == lexer_token_type::value_separator) {
                get_token();
                continue;
            }

            if (last_token == lexer_token_type::end_array) {
                if (!sax->end_array()) {
                    return false;
                }

                states.pop_back();
                skip_to_state_evaluation = true;
                continue;
            }

            return sax->parse_error(m_lexer->get_position(), m_lexer->get_token_string(),
                                    parse_error::create(101, m_lexer->get_position(),
                                                        exception_message(lexer_token_type::end_array,
                                                                          "array")));
        }

        if (get_token() == lexer_token_type::value_separator) {
            if (get_token() != lexer_token_type::value_string) {
                return sax
                    ->parse_error(m_lexer->get_position(), m_lexer->get_token_string(),
                                  parse_error::create(101, m_lexer->get_position(),
                                                      exception_message(lexer_token_type::value_string,
                                                                        "object key")));
            }

            if (!sax->key(m_lexer->get_string())) {
                return false;
            }

            if (get_token() != lexer_token_type::name_separator) {
                return sax
                    ->parse_error(m_lexer->get_position(), m_lexer->get_token_string(),
                                  parse_error::create(101, m_lexer->get_position(),
                                                      exception_message(lexer_token_type::name_separator,
                                                                        "object separator")));
            }

            get_token();
            continue;
        }

        if (last_token == lexer_token_type::end_object) {
            if (!sax->end_object()) {
                return false;
            }

            states.pop_back();
            skip_to_state_evaluation = true;
            continue;
        }

        return sax->parse_error(m_lexer->get_position(), m_lexer->get_token_string(),
                                parse_error::create(101, m_lexer->get_position(),
                                                    exception_message(lexer_token_type::end_object,
                                                                      "object")));
    }
}

nlohmann::detail::lexer_token_type nlohmann::detail::parser::get_token()
{
    return last_token = m_lexer->scan();
}

std::string nlohmann::detail::parser::exception_message(const lexer_token_type expected,
                                                        const std::string &context)
{
    std::string error_msg = "syntax error ";

    if (!context.empty()) {
        error_msg += concat("while parsing ", context, ' ');
    }

    error_msg += "- ";

    if (last_token == lexer_token_type::parse_error) {
        error_msg += concat(m_lexer->get_error_message(), "; last read: '",
                            m_lexer->get_token_string(), '\'');
    } else {
        error_msg += concat("unexpected ", lexer_token_type_name(last_token));
    }

    if (expected != lexer_token_type::uninitialized) {
        error_msg += concat("; expected ", lexer_token_type_name(expected));
    }

    return error_msg;
}
