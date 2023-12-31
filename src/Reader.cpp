#include "Reader.hpp"
#include "json.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <future>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <type_traits>
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
        case '\0' ... '\x19':
            error("Unexpected character after `\\`.");
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
                if (value >= 0xD800U && value < 0xDC00U) {
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
                    value = (0x10000 + ((value - 0xD800) << 10) +
                             (nextvalue - 0xDC00));
                }
                next(3); // will be nexted after switch
                // icu::UnicodeString{ch}.toUTF8String(s);
                char str[5]{};
                auto *p = reinterpret_cast<unsigned char *>(str); // NOLINT
                if (value < 0x0080U) {
                    s.push_back(static_cast<char>(value)); // for value == 0
                } else if (value < 0x0800U) {
                    auto second = value & 0b111111U;
                    value >>= 6;
                    auto first = value;
                    p[0] = first + 0b110'00000;
                    p[1] = second + 0b10'000000;
                } else if (value < 0x10000U) {
                    auto third = value & 0b111111U;
                    value >>= 6;
                    auto second = value & 0b111111U;
                    value >>= 6;
                    auto first = value;

                    p[0] = first + 0b1110'0000;
                    p[1] = second + 0b10'000000;
                    p[2] = third + 0b10'000000;
                } else {
                    auto fourth = value & 0b111111U;
                    value >>= 6;
                    auto third = value & 0b111111U;
                    value >>= 6;
                    auto second = value & 0b111111U;
                    value >>= 6;
                    auto first = value & 0b111U; // avoid error

                    p[0] = first + 0b11110'000;
                    p[1] = second + 0b10'000000;
                    p[2] = third + 0b10'000000;
                    p[3] = fourth + 0b10'000000;
                }
                s.append(str); // NOLINT
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
    if (match("0") && !match("0.") && !match("0e")) {
        next();
        return generate_token(Token::Type::NUMBER, 0);
    }
    if (match("-0") && !match("-0.") && !match("-0e")) {
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
    if (curr()->type != Token::Type::EOF_) {
        error("End of file expected");
    }
    return json;
}
// NOLINTBEGIN(*-no-recursion)
Json Parser::parse_value() {
    // curr != tokens_.end()

    switch (curr()->type) {
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
        auto json = Json(std::get<double>(curr()->value));
        next();
        return json;
    }
    case Token::Type::STRING: {
        auto json = Json(std::get<std::string>(curr()->value));
        next();
        return json;
    }
    }
    unreachable();
}

Json Parser::parse_array() {
    // curr != tokens_.end()
    Json json(ArrayType{});
    if (curr(1)->type == Token::Type::END_ARRAY) {
        next(2);
        return json;
    }
    do { // NOLINT
        next();
        json.append(parse_value());
    } while (curr()->type == Token::Type::VALUE_SEPARATOR);
    if (curr()->type == Token::Type::END_ARRAY) {
        next();
        return json;
    }
    error("Expected comma or closing bracket");
}
Json Parser::parse_object() {
    Json json(ObjectType{});
    if (curr(1)->type == Token::Type::END_OBJECT) {
        next(2);
        return json;
    }
    do { // NOLINT
        next();
        if (curr()->type == Token::Type::STRING) {
            auto index = std::get<std::string>(curr()->value);
            if (json.contains(index)) {
                error("Duplicate object key");
            }
            next();
            if (curr()->type == Token::Type::NAME_SEPARATOR) {
                next();
                json[index] = parse_value();
            } else {
                error("Colon expected");
            }
        } else {
            error("Property expected");
        }
    } while (curr()->type == Token::Type::VALUE_SEPARATOR);
    if (curr()->type == Token::Type::END_OBJECT) {
        next();
        return json;
    }
    error("Expected comma or closing bracket");
}
// NOLINTEND(*-no-recursion)

void Parser::error(const char *messgae) const {
    std::string message_ = std::to_string(curr()->lineno) + ":" +
                           std::to_string(curr()->col_offset) + ": " + messgae;
    throw std::runtime_error(message_);
}

Json threaded_parse(std::string_view data) {
    Lexer lexer(data);
    Parser parser;
    std::promise<void> p;
    std::future<void> f = p.get_future();
    std::thread worker([&]() {
        try {
            bool finish = false;
            while (!finish) {
                auto token = lexer.get_next_token();
                if (token.type == Token::Type::EOF_) {
                    finish = true;
                }
                parser.push(std::move(token));
            }
        } catch (...) {
            parser.stop = true;
            parser.cv.notify_all();
            try {
                // 存储任何抛出的异常于 promise
                p.set_exception(std::current_exception());
            } catch(...) {} // set_exception() 亦可能抛出
        }
    });
    Json value;
    std::thread consumer([&]() {
        try {
            value = parser.parse();
            p.set_value();
        } catch (...) {
            try {
                // 存储任何抛出的异常于 promise
                if(!parser.stop){
                    p.set_exception(std::current_exception());
                }
            } catch(...) {} // set_exception() 亦可能抛出
        }
    });
    consumer.join();
    worker.join();

    f.get();

    return value;
}