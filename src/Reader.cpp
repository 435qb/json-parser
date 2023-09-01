#include "Reader.hpp"
#include "json.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>
#include <unicode/unistr.h>
#include <utility>
#include <variant>

[[noreturn]] void unreachable() {
#ifdef __GNUC__ // GCC, Clang, ICC
    __builtin_unreachable();
#elif defined(_MSC_VER) // MSVC
    __assume(false);
#endif
}

std::string Token::get_type() const {
    switch (type) {
    case Type::EOF_:
        return "EOF";
    case Type::BEGIN_ARRAY:
        return "BEGIN_ARRAY";
    case Type::BEGIN_OBJECT:
        return "BEGIN_OBJECT";
    case Type::END_ARRAY:
        return "END_ARRAY";
    case Type::END_OBJECT:
        return "END_OBJECT";
    case Type::NAME_SEPARATOR:
        return "NAME_SEPARATOR";
    case Type::VALUE_SEPARATOR:
        return "VALUE_SEPARATOR";
    case Type::FALSE:
        return "FALSE";
    case Type::TRUE:
        return "TRUE";
    case Type::NULL_:
        return "NULL";
    case Type::NUMBER:
        return "NUMBER";
    case Type::STRING:
        return "STRING";
    }
    unreachable();
}
std::string Token::get_value() const {
    switch (type) {
    case Type::EOF_:
    case Type::BEGIN_ARRAY:
    case Type::BEGIN_OBJECT:
    case Type::END_ARRAY:
    case Type::END_OBJECT:
    case Type::NAME_SEPARATOR:
    case Type::VALUE_SEPARATOR:
    case Type::FALSE:
    case Type::TRUE:
    case Type::NULL_:
        return "";
    case Type::NUMBER:
    case Type::STRING:
        return std::visit(
            []<class T>(T &&arg) {
                if constexpr (std::is_same_v<double, std::remove_cvref_t<T>>) {
                    return std::to_string(arg);
                } else {
                    return arg;
                }
            },
            value);
    }
    unreachable();
}

Token Lexer::generate_token(Token::Type type, std::string value) const {
    return Token{lineno, colnom, type, value};
}
Token Lexer::generate_token(Token::Type type, double value) const {
    return Token{lineno, colnom, type, value};
}
Token Lexer::generate_token(Token::Type type) const {
    return Token{lineno, colnom, type};
}
Token Lexer::get_next_token() {
    skip_ws();
    if (curr_pos == data_.end()) {
        return generate_token(Token::Type::EOF_);
    }
    switch (*curr_pos) {
    case '[':
        next();
        return generate_token(Token::Type::BEGIN_ARRAY);
    case '{':
        next();
        return generate_token(Token::Type::BEGIN_OBJECT);
    case ']':
        next();
        return generate_token(Token::Type::END_ARRAY);
    case '}':
        next();
        return generate_token(Token::Type::END_OBJECT);
    case ':':
        next();
        return generate_token(Token::Type::NAME_SEPARATOR);
    case ',':
        next();
        return generate_token(Token::Type::VALUE_SEPARATOR);
    case 'f':
        next(1);
        if (match("alse")) {
            next(4);
            return generate_token(Token::Type::FALSE);
        }
        error("Value expected");
    case 't':
        next(1);
        if (match("rue")) {
            next(3);
            return generate_token(Token::Type::TRUE);
        }
        error("Value expected");
    case 'n':
        next(1);
        if (match("ull")) {
            next(3);
            return generate_token(Token::Type::NULL_);
        }
        error("Value expected");
    case '\x22': // " quotation mark
    {
        return get_string();
    }
    case '-':
    case '0' ... '9': {
        return get_number();
    }
    default:
        error("Value expected");
    }
}
Token Lexer::get_string() { // NOLINT
    next();
    std::string s;
    while (curr_pos != data_.end()) {
        switch (*curr_pos) {
        case '\x5C': /* \ */
            next();
            if (curr_pos == data_.end()) {
                error("Unexpected character after `\\`.");
            }
            switch (*curr_pos) {
            case '\x5C': /* \ */
            case '\x22': // "
            case '\x2F': // /
                s.push_back(*curr_pos);
                break;
            case 'b': // backspace
                s.push_back('\b');
                break;
            case 'f': // form feed
                s.push_back('\f');
                break;
            case 'n': // line feed
                s.push_back('\n');
                break;
            case 'r': // carriage return
                s.push_back('\r');
                break;
            case 't': // tab
                s.push_back('\t');
                break;
            case 'u': // \uxxxx
            {
                next();
                if (curr_pos + 4 > data_.end()) {
                    error("Invalid unicode sequence in string.");
                }
                unsigned int value{};
                auto result =
                    std::from_chars(curr_pos, curr_pos + 4, value, 16);
                if (result.ptr != curr_pos + 4) {
                    error("Invalid unicode sequence in string.");
                }
                auto ch = static_cast<UChar32>(value);
                if (value >= 0xD800 && value < 0xDC00) {
                    next(4);
                    if (!match("\\u")) {
                        error("Invalid unicode sequence in string.");
                    }
                    next(2);
                    if (curr_pos + 4 > data_.end()) {
                        error("Invalid unicode sequence in string.");
                    }
                    unsigned int nextvalue{};
                    auto nextresult =
                        std::from_chars(curr_pos, curr_pos + 4, nextvalue, 16);
                    if (nextresult.ptr != curr_pos + 4) {
                        error("Invalid unicode sequence in string.");
                    }
                    if (nextvalue < 0xDC00 || nextvalue >= 0xE000) {
                        error("Invalid unicode sequence in string.");
                    }
                    ch = static_cast<UChar32>(0x10000 +
                                              ((value - 0xD800) << 10) +
                                              (nextvalue - 0xDC00));
                }
                next(3); // will be nexted after switch
                icu::UnicodeString{ch}.toUTF8String(s);

                break;
            }
            default:
                error("Invalid escape character in string.");
            }
            break;
        case '\x22': // "
            next();
            return generate_token(Token::Type::STRING, std::move(s));
        default:
            s.push_back(*curr_pos);
            break;
        }
        next();
    }
    error("Unexpected end of string.");
}
Token Lexer::get_number() { // NOLINT
    if (match("0")) {
        next();
        return generate_token(Token::Type::NUMBER, 0);
    }
    if (match("-0")) {
        next(2);
        return generate_token(Token::Type::NUMBER, -0.0);
    }
    double retn{};
    auto result =
        from_chars(curr_pos, data_.end(), retn,
                   std::chars_format::general | std::chars_format::hex);
    if (result.ec == std::errc::invalid_argument) {
        error("invalid float string");
    } else if (result.ec == std::errc::result_out_of_range) {
        error("result out of range");
    }
    const auto *iter = std::find(curr_pos, result.ptr, '.');
    if (iter != result.ptr && (iter[1] < '0' || iter[1] > '9')) {
        error("Unexpected end of number");
    }
    curr_pos = result.ptr;
    return generate_token(Token::Type::NUMBER, retn);
}
void Lexer::error(const char *messgae) const {
    std::string message_ =
        std::to_string(lineno) + ":" + std::to_string(colnom) + ": " + messgae;
    throw std::runtime_error(message_);
}
void Lexer::skip_ws() {
    while (curr_pos != data_.end()) {
        switch (*curr_pos) {
        case '\x20': // Space
        case '\x09': // Horizontal tab
            next();
            break;
        case '\x0A': // Line feed or New line
        case '\x0D': // Carriage return
            colnom = 1;
            ++curr_pos;
            ++lineno;
            break;
        default:
            return;
        }
    }
}
std::vector<Token> Lexer::dump_tokens() {
    std::vector<Token> tokens;
    auto end = false;
    while (!end) {
        auto token = get_next_token();
        end = token.type == Token::Type::EOF_;
        tokens.emplace_back(std::move(token));
    }
    return tokens;
}

Json Parser::parse() {
    auto json = parse_value();
    if (curr_->type != Token::Type::EOF_) {
        error("End of file expected");
    }
    return json;
}
// NOLINTBEGIN(*-no-recursion)
Json Parser::parse_value() {
    // curr != tokens_.end()

    switch (curr_->type) {
    case Token::Type::EOF_:
    case Token::Type::END_ARRAY:
    case Token::Type::END_OBJECT:
    case Token::Type::NAME_SEPARATOR:
    case Token::Type::VALUE_SEPARATOR:
        error("Value expected");
    case Token::Type::BEGIN_ARRAY:
        return parse_array();
    case Token::Type::BEGIN_OBJECT:
        return parse_object();
    case Token::Type::FALSE:
        next();
        return Json(false);
    case Token::Type::TRUE:
        next();
        return Json(true);
    case Token::Type::NULL_:
        next();
        return Json(Null{});
    case Token::Type::NUMBER: {
        auto json = Json(std::get<double>(curr_->value));
        next();
        return json;
    }
    case Token::Type::STRING: {
        auto json = Json(std::get<double>(curr_->value));
        next();
        return json;
    }
    }
    unreachable();
}

Json Parser::parse_array() {
    // curr != tokens_.end()
    Json json(ArrayType{});
    if ((curr_ + 1)->type == Token::Type::END_ARRAY) {
        next(2);
        return json;
    }
    do { // NOLINT
        next();
        json.append(parse_value());
    } while (curr_->type == Token::Type::VALUE_SEPARATOR);
    if (curr_->type == Token::Type::END_ARRAY) {
        next();
        return json;
    }
    error("Expected comma or closing bracket");
}
Json Parser::parse_object() {
    Json json(ObjectType{});
    if ((curr_ + 1)->type == Token::Type::END_OBJECT) {
        next(2);
        return json;
    }
    do { // NOLINT
        next();
        if (curr_->type == Token::Type::STRING) {
            auto index = std::get<std::string>(curr_->value);
            if (json.contains(index)) {
                error("Duplicate object key");
            }
            next();
            if (curr_->type == Token::Type::NAME_SEPARATOR) {
                next();
                json[index] = parse_value();
            } else {
                error("Colon expected");
            }
        } else {
            error("Property expected");
        }
    } while (curr_->type == Token::Type::VALUE_SEPARATOR);
    if (curr_->type == Token::Type::END_OBJECT) {
        next();
        return json;
    }
    error("Expected comma or closing bracket");
}
// NOLINTEND(*-no-recursion)

void Parser::error(const char *messgae) const {
    std::string message_ = std::to_string(curr_->lineno) + ":" +
                           std::to_string(curr_->col_offset) + ": " + messgae;
    throw std::runtime_error(message_);
}
