#include "lexer.hpp"
#include <array>
#include "input_adapters.hpp"

const char *nlohmann::detail::lexer::token_type_name(const lexer_token_type t)
{
    return lexer_token_type_name(t);
}

nlohmann::detail::lexer::lexer(const iterator_input_adapter &adapter,
                               bool ignore_comments_) noexcept
    : ia(new iterator_input_adapter(adapter)), ignore_comments(ignore_comments_),
      decimal_point_char('.')
{}

nlohmann::detail::lexer::~lexer() = default;

int nlohmann::detail::lexer::get_codepoint()
{
    int codepoint = 0;

    const auto factors = {12u, 8u, 4u, 0u};
    for (const auto factor : factors) {
        get();

        if (current >= '0' && current <= '9') {
            codepoint += static_cast<int>((static_cast<unsigned int>(current) - 0x30u) << factor);
        } else if (current >= 'A' && current <= 'F') {
            codepoint += static_cast<int>((static_cast<unsigned int>(current) - 0x37u) << factor);
        } else if (current >= 'a' && current <= 'f') {
            codepoint += static_cast<int>((static_cast<unsigned int>(current) - 0x57u) << factor);
        } else {
            return -1;
        }
    }

    return codepoint;
}

bool nlohmann::detail::lexer::next_byte_in_range(std::initializer_list<uint64_t> ranges)
{
    add(current);

    for (auto range = ranges.begin(); range != ranges.end(); ++range) {
        get();
        if (*range <= current && current <= *(++range)) {
            add(current);
        } else {
            error_message = "invalid string: ill-formed UTF-8 byte";
            return false;
        }
    }

    return true;
}

nlohmann::detail::lexer_token_type nlohmann::detail::lexer::scan_string()
{
    reset();

    while (true) {
        switch (get()) {
        case uint64_t(EOF): {
            error_message = "invalid string: missing closing quote";
            return lexer_token_type::parse_error;
        }

        case '\"': {
            return lexer_token_type::value_string;
        }

        case '\\': {
            switch (get()) {
            case '\"': add('\"'); break;

            case '\\': add('\\'); break;

            case '/': add('/'); break;

            case 'b': add('\b'); break;

            case 'f': add('\f'); break;

            case 'n': add('\n'); break;

            case 'r': add('\r'); break;

            case 't': add('\t'); break;

            case 'u': {
                const int codepoint1 = get_codepoint();
                int codepoint = codepoint1;

                if (codepoint1 == -1) {
                    error_message = "invalid string: '\\u' must be followed by 4 hex digits";
                    return lexer_token_type::parse_error;
                }

                if (0xD800 <= codepoint1 && codepoint1 <= 0xDBFF) {
                    if (get() == '\\' && get() == 'u') {
                        const int codepoint2 = get_codepoint();

                        if (codepoint2 == -1) {
                            error_message
                                = "invalid string: '\\u' must be followed by 4 hex digits";
                            return lexer_token_type::parse_error;
                        }

                        if (0xDC00 <= codepoint2 && codepoint2 <= 0xDFFF) {
                            codepoint = static_cast<int>(

                                (static_cast<unsigned int>(codepoint1) << 10u)

                                + static_cast<unsigned int>(codepoint2)

                                - 0x35FDC00u);
                        } else {
                            error_message = "invalid string: surrogate U+D800..U+DBFF must be "
                                            "followed by U+DC00..U+DFFF";
                            return lexer_token_type::parse_error;
                        }
                    } else {
                        error_message = "invalid string: surrogate U+D800..U+DBFF must be "
                                        "followed by U+DC00..U+DFFF";
                        return lexer_token_type::parse_error;
                    }
                } else {
                    if (0xDC00 <= codepoint1 && codepoint1 <= 0xDFFF) {
                        error_message = "invalid string: surrogate U+DC00..U+DFFF must follow "
                                        "U+D800..U+DBFF";
                        return lexer_token_type::parse_error;
                    }
                }

                if (codepoint < 0x80) {
                    add(static_cast<uint64_t>(codepoint));
                } else if (codepoint <= 0x7FF) {
                    add(static_cast<uint64_t>(0xC0u
                                              | (static_cast<unsigned int>(codepoint) >> 6u)));
                    add(static_cast<uint64_t>(0x80u
                                              | (static_cast<unsigned int>(codepoint) & 0x3Fu)));
                } else if (codepoint <= 0xFFFF) {
                    add(static_cast<uint64_t>(0xE0u
                                              | (static_cast<unsigned int>(codepoint) >> 12u)));
                    add(static_cast<uint64_t>(
                        0x80u | ((static_cast<unsigned int>(codepoint) >> 6u) & 0x3Fu)));
                    add(static_cast<uint64_t>(0x80u
                                              | (static_cast<unsigned int>(codepoint) & 0x3Fu)));
                } else {
                    add(static_cast<uint64_t>(0xF0u
                                              | (static_cast<unsigned int>(codepoint) >> 18u)));
                    add(static_cast<uint64_t>(
                        0x80u | ((static_cast<unsigned int>(codepoint) >> 12u) & 0x3Fu)));
                    add(static_cast<uint64_t>(
                        0x80u | ((static_cast<unsigned int>(codepoint) >> 6u) & 0x3Fu)));
                    add(static_cast<uint64_t>(0x80u
                                              | (static_cast<unsigned int>(codepoint) & 0x3Fu)));
                }

                break;
            }

            default:
                error_message = "invalid string: forbidden character after backslash";
                return lexer_token_type::parse_error;
            }

            break;
        }

        case 0x00: {
            error_message
                = "invalid string: control character U+0000 (NUL) must be escaped to \\u0000";
            return lexer_token_type::parse_error;
        }

        case 0x01: {
            error_message
                = "invalid string: control character U+0001 (SOH) must be escaped to \\u0001";
            return lexer_token_type::parse_error;
        }

        case 0x02: {
            error_message
                = "invalid string: control character U+0002 (STX) must be escaped to \\u0002";
            return lexer_token_type::parse_error;
        }

        case 0x03: {
            error_message
                = "invalid string: control character U+0003 (ETX) must be escaped to \\u0003";
            return lexer_token_type::parse_error;
        }

        case 0x04: {
            error_message
                = "invalid string: control character U+0004 (EOT) must be escaped to \\u0004";
            return lexer_token_type::parse_error;
        }

        case 0x05: {
            error_message
                = "invalid string: control character U+0005 (ENQ) must be escaped to \\u0005";
            return lexer_token_type::parse_error;
        }

        case 0x06: {
            error_message
                = "invalid string: control character U+0006 (ACK) must be escaped to \\u0006";
            return lexer_token_type::parse_error;
        }

        case 0x07: {
            error_message
                = "invalid string: control character U+0007 (BEL) must be escaped to \\u0007";
            return lexer_token_type::parse_error;
        }

        case 0x08: {
            error_message = "invalid string: control character U+0008 (BS) must be escaped to "
                            "\\u0008 or \\b";
            return lexer_token_type::parse_error;
        }

        case 0x09: {
            error_message = "invalid string: control character U+0009 (HT) must be escaped to "
                            "\\u0009 or \\t";
            return lexer_token_type::parse_error;
        }

        case 0x0A: {
            error_message = "invalid string: control character U+000A (LF) must be escaped to "
                            "\\u000A or \\n";
            return lexer_token_type::parse_error;
        }

        case 0x0B: {
            error_message
                = "invalid string: control character U+000B (VT) must be escaped to \\u000B";
            return lexer_token_type::parse_error;
        }

        case 0x0C: {
            error_message = "invalid string: control character U+000C (FF) must be escaped to "
                            "\\u000C or \\f";
            return lexer_token_type::parse_error;
        }

        case 0x0D: {
            error_message = "invalid string: control character U+000D (CR) must be escaped to "
                            "\\u000D or \\r";
            return lexer_token_type::parse_error;
        }

        case 0x0E: {
            error_message
                = "invalid string: control character U+000E (SO) must be escaped to \\u000E";
            return lexer_token_type::parse_error;
        }

        case 0x0F: {
            error_message
                = "invalid string: control character U+000F (SI) must be escaped to \\u000F";
            return lexer_token_type::parse_error;
        }

        case 0x10: {
            error_message
                = "invalid string: control character U+0010 (DLE) must be escaped to \\u0010";
            return lexer_token_type::parse_error;
        }

        case 0x11: {
            error_message
                = "invalid string: control character U+0011 (DC1) must be escaped to \\u0011";
            return lexer_token_type::parse_error;
        }

        case 0x12: {
            error_message
                = "invalid string: control character U+0012 (DC2) must be escaped to \\u0012";
            return lexer_token_type::parse_error;
        }

        case 0x13: {
            error_message
                = "invalid string: control character U+0013 (DC3) must be escaped to \\u0013";
            return lexer_token_type::parse_error;
        }

        case 0x14: {
            error_message
                = "invalid string: control character U+0014 (DC4) must be escaped to \\u0014";
            return lexer_token_type::parse_error;
        }

        case 0x15: {
            error_message
                = "invalid string: control character U+0015 (NAK) must be escaped to \\u0015";
            return lexer_token_type::parse_error;
        }

        case 0x16: {
            error_message
                = "invalid string: control character U+0016 (SYN) must be escaped to \\u0016";
            return lexer_token_type::parse_error;
        }

        case 0x17: {
            error_message
                = "invalid string: control character U+0017 (ETB) must be escaped to \\u0017";
            return lexer_token_type::parse_error;
        }

        case 0x18: {
            error_message
                = "invalid string: control character U+0018 (CAN) must be escaped to \\u0018";
            return lexer_token_type::parse_error;
        }

        case 0x19: {
            error_message
                = "invalid string: control character U+0019 (EM) must be escaped to \\u0019";
            return lexer_token_type::parse_error;
        }

        case 0x1A: {
            error_message
                = "invalid string: control character U+001A (SUB) must be escaped to \\u001A";
            return lexer_token_type::parse_error;
        }

        case 0x1B: {
            error_message
                = "invalid string: control character U+001B (ESC) must be escaped to \\u001B";
            return lexer_token_type::parse_error;
        }

        case 0x1C: {
            error_message
                = "invalid string: control character U+001C (FS) must be escaped to \\u001C";
            return lexer_token_type::parse_error;
        }

        case 0x1D: {
            error_message
                = "invalid string: control character U+001D (GS) must be escaped to \\u001D";
            return lexer_token_type::parse_error;
        }

        case 0x1E: {
            error_message
                = "invalid string: control character U+001E (RS) must be escaped to \\u001E";
            return lexer_token_type::parse_error;
        }

        case 0x1F: {
            error_message
                = "invalid string: control character U+001F (US) must be escaped to \\u001F";
            return lexer_token_type::parse_error;
        }

        case 0x20:
        case 0x21:
        case 0x23:
        case 0x24:
        case 0x25:
        case 0x26:
        case 0x27:
        case 0x28:
        case 0x29:
        case 0x2A:
        case 0x2B:
        case 0x2C:
        case 0x2D:
        case 0x2E:
        case 0x2F:
        case 0x30:
        case 0x31:
        case 0x32:
        case 0x33:
        case 0x34:
        case 0x35:
        case 0x36:
        case 0x37:
        case 0x38:
        case 0x39:
        case 0x3A:
        case 0x3B:
        case 0x3C:
        case 0x3D:
        case 0x3E:
        case 0x3F:
        case 0x40:
        case 0x41:
        case 0x42:
        case 0x43:
        case 0x44:
        case 0x45:
        case 0x46:
        case 0x47:
        case 0x48:
        case 0x49:
        case 0x4A:
        case 0x4B:
        case 0x4C:
        case 0x4D:
        case 0x4E:
        case 0x4F:
        case 0x50:
        case 0x51:
        case 0x52:
        case 0x53:
        case 0x54:
        case 0x55:
        case 0x56:
        case 0x57:
        case 0x58:
        case 0x59:
        case 0x5A:
        case 0x5B:
        case 0x5D:
        case 0x5E:
        case 0x5F:
        case 0x60:
        case 0x61:
        case 0x62:
        case 0x63:
        case 0x64:
        case 0x65:
        case 0x66:
        case 0x67:
        case 0x68:
        case 0x69:
        case 0x6A:
        case 0x6B:
        case 0x6C:
        case 0x6D:
        case 0x6E:
        case 0x6F:
        case 0x70:
        case 0x71:
        case 0x72:
        case 0x73:
        case 0x74:
        case 0x75:
        case 0x76:
        case 0x77:
        case 0x78:
        case 0x79:
        case 0x7A:
        case 0x7B:
        case 0x7C:
        case 0x7D:
        case 0x7E:
        case 0x7F: {
            add(current);
            break;
        }

        case 0xC2:
        case 0xC3:
        case 0xC4:
        case 0xC5:
        case 0xC6:
        case 0xC7:
        case 0xC8:
        case 0xC9:
        case 0xCA:
        case 0xCB:
        case 0xCC:
        case 0xCD:
        case 0xCE:
        case 0xCF:
        case 0xD0:
        case 0xD1:
        case 0xD2:
        case 0xD3:
        case 0xD4:
        case 0xD5:
        case 0xD6:
        case 0xD7:
        case 0xD8:
        case 0xD9:
        case 0xDA:
        case 0xDB:
        case 0xDC:
        case 0xDD:
        case 0xDE:
        case 0xDF: {
            if (!next_byte_in_range({0x80, 0xBF})) {
                return lexer_token_type::parse_error;
            }
            break;
        }

        case 0xE0: {
            if (!(next_byte_in_range({0xA0, 0xBF, 0x80, 0xBF}))) {
                return lexer_token_type::parse_error;
            }
            break;
        }

        case 0xE1:
        case 0xE2:
        case 0xE3:
        case 0xE4:
        case 0xE5:
        case 0xE6:
        case 0xE7:
        case 0xE8:
        case 0xE9:
        case 0xEA:
        case 0xEB:
        case 0xEC:
        case 0xEE:
        case 0xEF: {
            if (!(next_byte_in_range({0x80, 0xBF, 0x80, 0xBF}))) {
                return lexer_token_type::parse_error;
            }
            break;
        }

        case 0xED: {
            if (!(next_byte_in_range({0x80, 0x9F, 0x80, 0xBF}))) {
                return lexer_token_type::parse_error;
            }
            break;
        }

        case 0xF0: {
            if (!(next_byte_in_range({0x90, 0xBF, 0x80, 0xBF, 0x80, 0xBF}))) {
                return lexer_token_type::parse_error;
            }
            break;
        }

        case 0xF1:
        case 0xF2:
        case 0xF3: {
            if (!(next_byte_in_range({0x80, 0xBF, 0x80, 0xBF, 0x80, 0xBF}))) {
                return lexer_token_type::parse_error;
            }
            break;
        }

        case 0xF4: {
            if (!(next_byte_in_range({0x80, 0x8F, 0x80, 0xBF, 0x80, 0xBF}))) {
                return lexer_token_type::parse_error;
            }
            break;
        }

        default: {
            error_message = "invalid string: ill-formed UTF-8 byte";
            return lexer_token_type::parse_error;
        }
        }
    }
}

bool nlohmann::detail::lexer::scan_comment()
{
    switch (get()) {
    case '/': {
        while (true) {
            switch (get()) {
            case '\n':
            case '\r':
            case uint64_t(EOF):
            case '\0': return true;

            default: break;
            }
        }
    }

    case '*': {
        while (true) {
            switch (get()) {
            case uint64_t(EOF):
            case '\0': {
                error_message = "invalid comment; missing closing '*/'";
                return false;
            }

            case '*': {
                switch (get()) {
                case '/': return true;

                default: {
                    unget();
                    continue;
                }
                }
            }

            default: continue;
            }
        }
    }

    default: {
        error_message = "invalid comment; expecting '/' or '*' after '/'";
        return false;
    }
    }
}

void nlohmann::detail::lexer::strtof(float &f, const char *str, char **endptr) noexcept
{
    f = std::strtof(str, endptr);
}

void nlohmann::detail::lexer::strtof(double &f, const char *str, char **endptr) noexcept
{
    f = std::strtod(str, endptr);
}

void nlohmann::detail::lexer::strtof(long double &f, const char *str, char **endptr) noexcept
{
    f = std::strtold(str, endptr);
}

nlohmann::detail::lexer_token_type nlohmann::detail::lexer::scan_number()
{
    reset();

    lexer_token_type number_type = lexer_token_type::value_integer;

    switch (current) {
    case '-': {
        add(current);
        goto scan_number_minus;
    }

    case '0': {
        add(current);
        goto scan_number_zero;
    }

    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
        add(current);
        goto scan_number_any1;
    }
    }

scan_number_minus:

    number_type = lexer_token_type::value_integer;
    switch (get()) {
    case '0': {
        add(current);
        goto scan_number_zero;
    }

    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
        add(current);
        goto scan_number_any1;
    }

    default: {
        error_message = "invalid number; expected digit after '-'";
        return lexer_token_type::parse_error;
    }
    }

scan_number_zero:

    switch (get()) {
    case '.': {
        add(decimal_point_char);
        goto scan_number_decimal1;
    }

    case 'e':
    case 'E': {
        add(current);
        goto scan_number_exponent;
    }

    default: goto scan_number_done;
    }

scan_number_any1:

    switch (get()) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
        add(current);
        goto scan_number_any1;
    }

    case '.': {
        add(decimal_point_char);
        goto scan_number_decimal1;
    }

    case 'e':
    case 'E': {
        add(current);
        goto scan_number_exponent;
    }

    default: goto scan_number_done;
    }

scan_number_decimal1:

    number_type = lexer_token_type::value_float;
    switch (get()) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
        add(current);
        goto scan_number_decimal2;
    }

    default: {
        error_message = "invalid number; expected digit after '.'";
        return lexer_token_type::parse_error;
    }
    }

scan_number_decimal2:

    switch (get()) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
        add(current);
        goto scan_number_decimal2;
    }

    case 'e':
    case 'E': {
        add(current);
        goto scan_number_exponent;
    }

    default: goto scan_number_done;
    }

scan_number_exponent:

    number_type = lexer_token_type::value_float;
    switch (get()) {
    case '+':
    case '-': {
        add(current);
        goto scan_number_sign;
    }

    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
        add(current);
        goto scan_number_any2;
    }

    default: {
        error_message = "invalid number; expected '+', '-', or digit after exponent";
        return lexer_token_type::parse_error;
    }
    }

scan_number_sign:

    switch (get()) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
        add(current);
        goto scan_number_any2;
    }

    default: {
        error_message = "invalid number; expected digit after exponent sign";
        return lexer_token_type::parse_error;
    }
    }

scan_number_any2:

    switch (get()) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
        add(current);
        goto scan_number_any2;
    }

    default: goto scan_number_done;
    }

scan_number_done:

    unget();

    char *endptr = nullptr;
    errno = 0;

    if (number_type == lexer_token_type::value_integer) {
        const auto x = std::strtoll(token_buffer.data(), &endptr, 10);

        if (errno == 0) {
            value_integer = static_cast<int64_t>(x);
            if (value_integer == x) {
                return lexer_token_type::value_integer;
            }
        }
    }

    strtof(value_float, token_buffer.data(), &endptr);

    return lexer_token_type::value_float;
}

nlohmann::detail::lexer_token_type nlohmann::detail::lexer::scan_literal(const char *literal_text,
                                                                          const size_t length,
                                                                          lexer_token_type return_type)
{
    for (size_t i = 1; i < length; ++i) {
        if (char(get()) != literal_text[i]) {
            error_message = "invalid literal";
            return lexer_token_type::parse_error;
        }
    }
    return return_type;
}

void nlohmann::detail::lexer::reset() noexcept
{
    token_buffer.clear();
    token_string.clear();
    token_string.push_back(char(current));
}

uint64_t nlohmann::detail::lexer::get()
{
    ++position.chars_read_total;
    ++position.chars_read_current_line;

    if (next_unget) {
        next_unget = false;
    } else {
        current = ia->get_character();
    }

    if (current != uint64_t(EOF)) {
        token_string.push_back(char(current));
    }

    if (current == '\n') {
        ++position.lines_read;
        position.chars_read_current_line = 0;
    }

    return current;
}

void nlohmann::detail::lexer::unget()
{
    next_unget = true;

    --position.chars_read_total;

    if (position.chars_read_current_line == 0) {
        if (position.lines_read > 0) {
            --position.lines_read;
        }
    } else {
        --position.chars_read_current_line;
    }

    if (current != uint64_t(EOF)) {
        token_string.pop_back();
    }
}

void nlohmann::detail::lexer::add(uint64_t c)
{
    token_buffer.push_back(static_cast<typename std::string::value_type>(c));
}

std::string nlohmann::detail::lexer::get_token_string() const
{
    std::string result;
    for (const auto c : token_string) {
        if (static_cast<unsigned char>(c) <= '\x1F') {
            std::array<char, 9> cs{{}};
            static_cast<void>(
                (std::snprintf)(cs.data(), cs.size(), "<U+%.4X>", static_cast<unsigned char>(c)));
            result += cs.data();
        } else {
            result.push_back(static_cast<std::string::value_type>(c));
        }
    }

    return result;
}

bool nlohmann::detail::lexer::skip_bom()
{
    if (get() == 0xEF) {
        return get() == 0xBB && get() == 0xBF;
    }

    unget();
    return true;
}

void nlohmann::detail::lexer::skip_whitespace()
{
    do {
        get();
    } while (current == ' ' || current == '\t' || current == '\n' || current == '\r');
}

nlohmann::detail::lexer_token_type nlohmann::detail::lexer::scan()
{
    if (position.chars_read_total == 0 && !skip_bom()) {
        error_message = "invalid BOM; must be 0xEF 0xBB 0xBF if given";
        return lexer_token_type::parse_error;
    }

    skip_whitespace();

    while (ignore_comments && current == '/') {
        if (!scan_comment()) {
            return lexer_token_type::parse_error;
        }

        skip_whitespace();
    }

    switch (current) {
    case '[': return lexer_token_type::begin_array;
    case ']': return lexer_token_type::end_array;
    case '{': return lexer_token_type::begin_object;
    case '}': return lexer_token_type::end_object;
    case ':': return lexer_token_type::name_separator;
    case ',': return lexer_token_type::value_separator;

    case 't': {
        std::array<char, 4> true_literal = {{static_cast<char>('t'), static_cast<char>('r'),
                                             static_cast<char>('u'), static_cast<char>('e')}};
        return scan_literal(true_literal.data(), true_literal.size(), lexer_token_type::literal_true);
    }
    case 'f': {
        std::array<char, 5> false_literal = {{static_cast<char>('f'), static_cast<char>('a'),
                                              static_cast<char>('l'), static_cast<char>('s'),
                                              static_cast<char>('e')}};
        return scan_literal(false_literal.data(), false_literal.size(), lexer_token_type::literal_false);
    }
    case 'n': {
        std::array<char, 4> null_literal = {{static_cast<char>('n'), static_cast<char>('u'),
                                             static_cast<char>('l'), static_cast<char>('l')}};
        return scan_literal(null_literal.data(), null_literal.size(), lexer_token_type::literal_null);
    }

    case '\"': return scan_string();

    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': return scan_number();

    case '\0':
    case uint64_t(EOF): return lexer_token_type::end_of_input;

    default: error_message = "invalid literal"; return lexer_token_type::parse_error;
    }
}
